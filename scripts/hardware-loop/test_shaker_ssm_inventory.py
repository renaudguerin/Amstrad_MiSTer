"""Guard on the SHAKER SSM marker inventory, a load-bearing planning premise.

SHAKER builds its per-test SSM codes at run time by patching an `ED 00 ED 00`
template, so the only markers visible statically are the two unpatched templates
per module. That is recorded in
`docs/shaker-ssm-marker-inventory-2026-09-12.md`, and these tests pin it: a disc
whose codes appear as literal bytes would mean the emitter changed, and a disc
carrying `#FFFE` would mean the scripts started using CSL-side naming. Either
would make that document stale, and both fail here loudly instead.

Skips when the user-owned, untracked disc images are absent.
"""

from __future__ import annotations

import unittest
from pathlib import Path

from shaker_ssm_inventory import DiskError, inventory, ssm_byte_allowed

SHAKER_DIR = Path(__file__).resolve().parents[2] / "docs" / "references" / "Shaker_CSL"
DISCS = ("shaker26.dsk", "shaker27.dsk")

CODE_SYNC = 0x0000
CODE_SCREENSHOT = 0xFFFE
CODE_SNAPSHOT = 0xFFFF


class TestAllowedBytes(unittest.TestCase):
    def test_byte_set_matches_the_rtl_matcher(self):
        # Same ranges as rtl/ssm_marker.v, including FE/FF, which the standard
        # reserves for its own codes and therefore uses itself.
        for byte in (0x00, 0x3F, 0x7F, 0x9F, 0xA4, 0xA7, 0xAC, 0xAF,
                     0xB4, 0xB7, 0xBC, 0xBF, 0xC0, 0xFD, 0xFE, 0xFF):
            self.assertTrue(ssm_byte_allowed(byte), f"0x{byte:02X}")
        for byte in (0x40, 0x7E, 0xA0, 0xA8, 0xB0, 0xB8):
            self.assertFalse(ssm_byte_allowed(byte), f"0x{byte:02X}")


@unittest.skipUnless(
    all((SHAKER_DIR / name).is_file() for name in DISCS),
    "SHAKER disc images are user-owned and untracked",
)
class TestBundledDiscInventory(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.reports = {name: inventory(SHAKER_DIR / name) for name in DISCS}

    def test_every_module_reassembles_to_its_declared_length(self):
        # The AMSDOS header declares the length, so this is a free correctness
        # check on the extents walk. An earlier revision of the tool failed it.
        for name, report in self.reports.items():
            self.assertTrue(report["modules"], name)
            for module in report["modules"]:
                self.assertEqual(module["declared"], module["extracted"],
                                 f"{name}/{module['name']}")

    def test_five_modules_per_disc(self):
        for name, report in self.reports.items():
            self.assertEqual(len(report["modules"]), 5, name)

    def test_no_static_screenshot_or_snapshot_markers_exist(self):
        # #FFFE only means "name this capture from the CSL screenshot_name",
        # and the bundled scripts never use it, so its absence is expected.
        # Its appearance would mean the scripts changed approach.
        for name, report in self.reports.items():
            totals = report["totals"]
            self.assertEqual(totals.get(CODE_SCREENSHOT, 0), 0,
                             f"{name} now emits #FFFE; the scripts changed approach")
            self.assertEqual(totals.get(CODE_SNAPSHOT, 0), 0, name)

    def test_two_unpatched_template_sites_per_module(self):
        for name, report in self.reports.items():
            for module in report["modules"]:
                # Two `ED 00 ED 00` sites: the patched per-test emitter's
                # template and the gated sync emitter.
                self.assertEqual(module["codes"].get(CODE_SYNC, 0), 2,
                                 f"{name}/{module['name']}")

    def test_the_two_template_sites_sit_together(self):
        # In SHAKE26B they are &A109, the per-test emitter's template, and
        # &A128, a gated #0000 sync emitter, 31 bytes apart. Sites that spread
        # out across a module would mean markers are inlined per test after
        # all, which would change how the codes must be read.
        for name, report in self.reports.items():
            for module in report["modules"]:
                sites = [int(a, 16) for a in module["addresses"]["0000"]]
                self.assertEqual(len(sites), 2, f"{name}/{module['name']}")
                self.assertLess(max(sites) - min(sites), 256,
                                f"{name}/{module['name']} sync sites spread out")

    def test_only_2_7_carries_the_sikoview_pair(self):
        self.assertEqual(self.reports["shaker26.dsk"]["totals"].get(0xFFFD, 0), 0)
        self.assertEqual(self.reports["shaker27.dsk"]["totals"].get(0xFFFD, 0), 4)
        self.assertEqual(self.reports["shaker27.dsk"]["totals"].get(0xFFFC, 0), 4)


class TestDiskErrors(unittest.TestCase):
    def test_a_non_disk_image_is_refused(self):
        with self.assertRaises(DiskError):
            from shaker_ssm_inventory import read_tracks
            read_tracks(b"not a disk image" * 32)


if __name__ == "__main__":
    unittest.main()
