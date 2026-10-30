import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools" / "ParisLauncher"))
from manifest_validation import validate_manifest


class ManifestValidationTests(unittest.TestCase):
    def entry(self, path, client="modern", aliases=None):
        return {"Path": path, "Client": client, "Size": 1, "Sha256": "a" * 64, "ClassicAliases": aliases or []}

    def test_case_only_duplicate_is_rejected(self):
        with self.assertRaises(ValueError):
            validate_manifest({"Version": "test", "Files": [self.entry("Data/Local/Movereq.bmd"), self.entry("Data/Local/movereq.bmd")]})

    def test_alias_collision_is_rejected(self):
        with self.assertRaises(ValueError):
            validate_manifest({"Version": "test", "Files": [self.entry("Data/a", "common", ["Classic/Data/A"]), self.entry("Classic/Data/a", "classic")]})

    def test_distinct_clients_and_valid_shared_aliases_are_accepted(self):
        validate_manifest({"Version": "test", "Files": [self.entry("Data/a", "common", ["Classic/Data/a"]), self.entry("Data/b"), self.entry("Classic/Data/b", "classic")]})

    def test_invalid_hash_and_client_are_rejected(self):
        for key, value in [("Sha256", "g" * 64), ("Client", "unknown")]:
            entry = self.entry("Data/a")
            entry[key] = value
            with self.assertRaises(ValueError):
                validate_manifest({"Version": "test", "Files": [entry]})


if __name__ == "__main__":
    unittest.main()
