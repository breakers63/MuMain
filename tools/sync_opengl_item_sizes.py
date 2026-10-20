"""Synchronize inventory footprints with the supplied Paris OpenGL client.

Only width and height change. Names, stats, skills and custom item definitions
remain intact. The reference's explicit scroll override takes precedence over BMD.
"""
import argparse
import json
import re
from pathlib import Path


def reference_sizes(client, source):
    path = client / 'Data/Local/Eng/Item_eng.bmd'
    data = path.read_bytes()
    count, record_size = 8192, 84
    if len(data) != count * record_size + 4:
        raise ValueError('Unsupported OpenGL item catalogue size')
    sizes = {}
    for item_type in range(count):
        record = data[item_type * record_size:(item_type + 1) * record_size]
        decoded = bytes(value ^ (0xFC, 0xCF, 0xAB)[i % 3] for i, value in enumerate(record))
        width, height = decoded[38:40]
        if decoded[0] and 1 <= width <= 8 and 1 <= height <= 8:
            sizes[item_type] = (width, height)
    rule = (source / 'GameLogic/Items/ScrollInventorySize.cpp').read_text(encoding='utf-8-sig')
    values = re.search(r'ScrollTypes\[\]\s*=\s*\{([^}]+)', rule)
    if not values:
        raise ValueError('Explicit OpenGL scroll-size policy missing')
    for item_type in map(int, re.findall(r'\d+', values.group(1))):
        sizes[item_type] = (1, 1)
    return sizes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', type=Path, required=True)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--items', type=Path, required=True)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    sizes = reference_sizes(args.client, args.source)
    differences = []
    for path in sorted(args.items.glob('Group*.json')):
        document = json.loads(path.read_text(encoding='utf-8'))
        changed = False
        for item in document['items']:
            item_type = document['group'] * 512 + item['number']
            target = sizes.get(item_type)
            if target is None or (item.get('width'), item.get('height')) == target:
                continue
            differences.append((item_type, item.get('width'), item.get('height'), *target))
            item['width'], item['height'] = target
            changed = True
        if changed and not args.check:
            path.write_text(json.dumps(document, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f'{len(sizes)} reference footprints; {len(differences)} differences.')
    if args.check and differences:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
