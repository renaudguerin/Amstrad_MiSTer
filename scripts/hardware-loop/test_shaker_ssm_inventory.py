"""Guard on the SHAKER SSM marker inventory, a load-bearing planning premise.

`docs/shaker-ssm-marker-inventory-2026-09-12.md` concludes that the published
discs emit no `#FFFE` at all, which is why phase 2 of the CSL/SSM plan has no
consumer yet. If a future disc, or a build compiled with screenshot markers,
ever lands in `docs/references/Shaker_CSL/`, this suite fails and says so
rather than letting the plan quietly go stale.

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

    def test_no_screenshot_or_snapshot_markers_exist(self):
        # This is the premise phase 2 rests on. If it ever fails, read
        # docs/shaker-ssm-marker-inventory-2026-09-12.md and revise the plan:
        # a disc that emits #FFFE turns phase 2 from speculative into needed.
        for name, report in self.reports.items():
            totals = report["totals"]
            self.assertEqual(totals.get(CODE_SCREENSHOT, 0), 0,
                             f"{name} now emits #FFFE; phase 2 has a consumer")
            self.assertEqual(totals.get(CODE_SNAPSHOT, 0), 0, name)

    def test_two_sync_markers_per_module(self):
        for name, report in self.reports.items():
            for module in report["modules"]:
                self.assertEqual(module["codes"].get(CODE_SYNC, 0), 2,
                                 f"{name}/{module['name']}")

    def test_sync_markers_are_module_level_not_per_screen(self):
        # Both sites in a module sit within a few dozen bytes of each other,
        # which is what makes #0000 a coarse handshake rather than a
        # per-screen signal. A disc where they spread out would mean the
        # opposite, and would make wait_ssm0000 a real capture mechanism.
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
