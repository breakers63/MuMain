"""Export user-supplied Pegasus sky faces and its colour pipeline to native assets.

Usage: python tools/export_pegasus_graphics.py --source PATH --output src/bin/Data
The colour-only operations of post.fs are composed into the existing 32-cube
grading format. Sharpening remains a separate spatial operation in the renderer.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path

import numpy as np
from PIL import Image


def read_cube(path):
    size = 0
    values = []
    for line in path.read_text().splitlines():
        parts = line.split()
        if not parts or parts[0].startswith('#'):
            continue
        if parts[0] == 'LUT_3D_SIZE':
            size = int(parts[1])
        elif len(parts) == 3:
            try:
                values.append([float(v) for v in parts])
            except ValueError:
                pass
    if size < 2 or len(values) != size ** 3:
        raise ValueError('Incomplete Pegasus colour cube')
    return np.array(values).reshape(size, size, size, 3)


def sample_cube(cube, colors):
    scaled = np.clip(colors, 0, 1) * (len(cube) - 1)
    lo = np.floor(scaled).astype(int)
    hi = np.minimum(lo + 1, len(cube) - 1)
    t = scaled - lo
    result = np.zeros_like(colors)
    # .cube entries run red fastest, then green, then blue.
    for b in range(2):
        for g in range(2):
            for r in range(2):
                index = np.stack([hi[..., c] if side else lo[..., c]
                                  for c, side in enumerate((r, g, b))], axis=-1)
                weight = np.prod(np.stack([t[..., c] if side else 1 - t[..., c]
                                           for c, side in enumerate((r, g, b))], axis=-1), axis=-1)
                result += cube[index[..., 2], index[..., 1], index[..., 0]] * weight[..., None]
    return result


def pegasus_color(colors, cube, exposure, contrast, saturation, lut_strength):
    linear = np.maximum(colors, 0) ** 2.2
    linear = linear * exposure / (1 + linear * (exposure - 1))
    color = np.clip(linear, 0, 1) ** (1 / 2.2)
    low, high = color ** contrast, (1 - color) ** contrast
    color = low / np.maximum(low + high, .00001)
    luma = np.sum(color * [.2126, .7152, .0722], axis=-1, keepdims=True)
    color = np.clip(luma + (color - luma) * saturation, 0, 1)
    return color + (sample_cube(cube, color) - color) * lut_strength


def export_tables(cube, output):
    size = 32
    b, g, r = np.meshgrid(np.arange(size), np.arange(size), np.arange(size), indexing='ij')
    colors = np.stack([r, g, b], axis=-1) / (size - 1)
    profiles = {'modern': (1.05, 1.09, 1.05, .12), 'ultra': (1.10, 1.18, 1.10, .20)}
    output.mkdir(parents=True, exist_ok=True)
    for name, parameters in profiles.items():
        graded = pegasus_color(colors, cube, *parameters)
        # Strip layout: rows green, columns blue slice followed by red.
        rgb = np.clip(np.rint(graded.transpose(1, 0, 2, 3).reshape(size, size * size, 3) * 255), 0, 255).astype('uint8')
        rgba = np.concatenate([rgb, np.full((size, size * size, 1), 255, dtype='uint8')], axis=-1)
        Image.fromarray(rgba).save(output / f'{name}.png')


def export_skies(source, output):
    count = 0
    for path in sorted(source.glob('World*/*.bmp')):
        target = output / path.parent.name / (path.stem + '.OZT')
        target.parent.mkdir(parents=True, exist_ok=True)
        stream = io.BytesIO()
        with Image.open(path) as image:
            image.convert('RGBA').save(stream, format='TGA')
        target.write_bytes(bytes(4) + stream.getvalue())
        count += 1
    return count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    client = args.source / 'Client/Data'
    cube_path = client / 'Shaders/PegasusPost/look2.cube'
    cube = read_cube(cube_path)
    output = args.output / 'Grading/Pegasus'
    export_tables(cube, output)
    (output / 'look2.cube').write_bytes(cube_path.read_bytes())
    count = export_skies(client / 'Custom/Sky', args.output / 'Effect/ParisSky')
    manifest = {'source_cube_sha256': hashlib.sha256(cube_path.read_bytes()).hexdigest(),
                'source_cube_size': len(cube), 'export_cube_size': 32,
                'sky_faces': count, 'ultra': {'exposure': 1.10, 'contrast': 1.18, 'saturation': 1.10, 'lut': .20}}
    (output / 'export.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Exported {count} sky faces and 2 Pegasus grading tables.')


if __name__ == '__main__':
    main()
