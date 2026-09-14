"""Tests for the SNA snapshot DDR3 save stream pull script (B18).

The slot image fixtures here are built byte by byte from the DDR3 layout
described in rtl/sna_save_stream.v and the SNA format specification, never
captured from the simulator.
"""

from __future__ import annotations

import base64
import io
import re
import struct
import sys
import tempfile
import unittest
from pathlib import Path
from typing import Any, List, Optional
from unittest.mock import patch

# Ensure hardware-loop directory is in sys.path
_hw_loop_dir = str(Path(__file__).resolve().parent)
if _hw_loop_dir not in sys.path:
    sys.path.insert(0, _hw_loop_dir)

from driver import CommandResult, DriverError
from sna_pull import (
    DEFAULT_BASE,
    LEN_WORDS_64K,
    LEN_WORDS_128K,
    NoSnapshotError,
    SlotHeader,
    SnaPullError,
    SnaTransportError,
    SnaTruncatedError,
    SnaWaitTimeoutError,
    TornReadError,
    main,
    parse_slot_header,
    pull_snapshot,
    read_command,
    validate_sna_payload,
    wait_and_pull,
    wait_for_generation,
    write_snapshot_file,
)


def make_slot_image(
    generation: int = 1,
    len_words: int = LEN_WORDS_128K,
    gen_high: int = 0,
    len_high: int = 0,
    word0_override: Optional[int] = None,
    word1_override: Optional[int] = None,
    signature: bytes = b"MV - SNA",
    version: int = 3,
    mem_size_kb: Optional[int] = None,
    fill_byte: int = 0xA5,
) -> bytes:
    """Build a synthetic DDR3 slot image byte by byte matching the RTL layout."""
    if word0_override is not None:
        word0 = word0_override
    else:
        word0 = (generation & 0xFFFFFFFF) | ((gen_high & 0xFFFFFFFF) << 32)

    if word1_override is not None:
        word1 = word1_override
    else:
        word1 = (len_words & 0xFFFFFFFF) | ((len_high & 0xFFFFFFFF) << 32)

    header = struct.pack("<QQ", word0, word1)
    file_bytes = len_words * 4
    payload = bytearray([fill_byte] * file_bytes)

    if len(payload) >= 8:
        payload[0:len(signature)] = signature
    if len(payload) >= 0x11:
        payload[0x10] = version

    if mem_size_kb is None:
        mem_size_kb = 128 if len_words == LEN_WORDS_128K else 64

    if len(payload) >= 0x6D:
        struct.pack_into("<H", payload, 0x6B, mem_size_kb)

    return header + bytes(payload)


class FakeTransport:
    """Scriptable mock transport returning CommandResults based on command size."""

    def __init__(
        self,
        slot_image: Optional[bytes] = None,
        responses: Optional[List[Any]] = None,
    ):
        self.slot_image = slot_image
        self.responses: List[Any] = list(responses) if responses is not None else []
        self.commands: List[str] = []

    def run_cmd(self, cmd: str, timeout: float = 20.0) -> CommandResult:
        self.commands.append(cmd)
        m = re.search(r"s=(\d+)", cmd)
        size = int(m.group(1)) if m else 16

        if self.responses:
            item = self.responses.pop(0)
            if isinstance(item, Exception):
                raise item
            if isinstance(item, CommandResult):
                return item
            data = item[:size] if len(item) > size else item
        elif self.slot_image is not None:
            data = self.slot_image[:size]
        else:
            data = b"\x00" * size

        encoded = base64.b64encode(data).decode("ascii")
        return CommandResult(0, encoded, "")


class TestSlotParsingAndValidation(unittest.TestCase):
    def test_valid_128k_slot_header_unpacks(self):
        raw = struct.pack("<QQ", 42, LEN_WORDS_128K)
        hdr = parse_slot_header(raw)
        self.assertEqual(hdr.generation, 42)
        self.assertEqual(hdr.len_words, LEN_WORDS_128K)
        self.assertEqual(hdr.file_bytes, 0x20100)

    def test_valid_64k_slot_header_unpacks(self):
        raw = struct.pack("<QQ", 7, LEN_WORDS_64K)
        hdr = parse_slot_header(raw)
        self.assertEqual(hdr.generation, 7)
        self.assertEqual(hdr.len_words, LEN_WORDS_64K)
        self.assertEqual(hdr.file_bytes, 0x10100)

    def test_all_ones_generation_rejected_as_no_snapshot(self):
        raw = struct.pack("<QQ", 0xFFFFFFFFFFFFFFFF, LEN_WORDS_128K)
        with self.assertRaises(NoSnapshotError) as ctx:
            parse_slot_header(raw)
        self.assertIn("publication in progress", str(ctx.exception))

    def test_illegal_length_rejected_as_no_snapshot(self):
        for bad_len in (0x0000, 0x1234, 0x8000, 0x8041):
            raw = struct.pack("<QQ", 1, bad_len)
            with self.assertRaises(NoSnapshotError) as ctx:
                parse_slot_header(raw)
            self.assertIn("illegal length", str(ctx.exception))

    def test_nonzero_high_32_bits_word0_rejected_as_no_snapshot(self):
        raw = struct.pack("<QQ", 1 | (1 << 32), LEN_WORDS_128K)
        with self.assertRaises(NoSnapshotError) as ctx:
            parse_slot_header(raw)
        self.assertIn("nonzero high 32 bits in word 0", str(ctx.exception))

    def test_nonzero_high_32_bits_word1_rejected_as_no_snapshot(self):
        raw = struct.pack("<QQ", 1, LEN_WORDS_128K | (1 << 32))
        with self.assertRaises(NoSnapshotError) as ctx:
            parse_slot_header(raw)
        self.assertIn("nonzero high 32 bits in word 1", str(ctx.exception))

    def test_generation_zero_rejected_as_no_snapshot(self):
        raw = struct.pack("<QQ", 0, LEN_WORDS_128K)
        with self.assertRaises(NoSnapshotError) as ctx:
            parse_slot_header(raw)
        self.assertIn("generation 0x00000000 out of valid range", str(ctx.exception))

    def test_bad_signature_rejected_as_no_snapshot(self):
        img = make_slot_image(signature=b"BAD - SNA")
        payload = img[16:]
        with self.assertRaises(NoSnapshotError) as ctx:
            validate_sna_payload(payload, LEN_WORDS_128K)
        self.assertIn("bad SNA signature", str(ctx.exception))

    def test_bad_version_byte_rejected_as_no_snapshot(self):
        img = make_slot_image(version=2)
        payload = img[16:]
        with self.assertRaises(NoSnapshotError) as ctx:
            validate_sna_payload(payload, LEN_WORDS_128K)
        self.assertIn("byte 0x10 is 2", str(ctx.exception))

    def test_memory_size_mismatch_128k_rejected_as_no_snapshot(self):
        img = make_slot_image(len_words=LEN_WORDS_128K, mem_size_kb=64)
        payload = img[16:]
        with self.assertRaises(NoSnapshotError) as ctx:
            validate_sna_payload(payload, LEN_WORDS_128K)
        self.assertIn("memory size 64 KB", str(ctx.exception))

    def test_memory_size_mismatch_64k_rejected_as_no_snapshot(self):
        img = make_slot_image(len_words=LEN_WORDS_64K, mem_size_kb=128)
        payload = img[16:]
        with self.assertRaises(NoSnapshotError) as ctx:
            validate_sna_payload(payload, LEN_WORDS_64K)
        self.assertIn("memory size 128 KB", str(ctx.exception))


class TestSnapshotPull(unittest.TestCase):
    def test_valid_128k_pull_produces_exact_file_bytes(self):
        img = make_slot_image(generation=10, len_words=LEN_WORDS_128K, fill_byte=0x42)
        fake = FakeTransport(slot_image=img)
        hdr, payload = pull_snapshot(fake, retry_delay=0)
        self.assertEqual(hdr.generation, 10)
        self.assertEqual(hdr.len_words, LEN_WORDS_128K)
        self.assertEqual(hdr.file_bytes, 0x20100)
        self.assertEqual(payload, img[16:])
        self.assertEqual(len(payload), 0x20100)

    def test_valid_64k_pull_produces_exact_file_bytes(self):
        img = make_slot_image(generation=25, len_words=LEN_WORDS_64K, fill_byte=0x7E)
        fake = FakeTransport(slot_image=img)
        hdr, payload = pull_snapshot(fake, retry_delay=0)
        self.assertEqual(hdr.generation, 25)
        self.assertEqual(hdr.len_words, LEN_WORDS_64K)
        self.assertEqual(hdr.file_bytes, 0x10100)
        self.assertEqual(payload, img[16:])
        self.assertEqual(len(payload), 0x10100)

    def test_pull_rejects_all_ones_as_no_snapshot(self):
        img = make_slot_image(word0_override=0xFFFFFFFFFFFFFFFF)
        fake = FakeTransport(slot_image=img)
        with self.assertRaises(NoSnapshotError):
            pull_snapshot(fake, retry_delay=0)

    def test_pull_rejects_illegal_length_as_no_snapshot(self):
        img = make_slot_image(len_words=0x3000)
        fake = FakeTransport(slot_image=img)
        with self.assertRaises(NoSnapshotError):
            pull_snapshot(fake, retry_delay=0)

    def test_pull_rejects_nonzero_high_word0_as_no_snapshot(self):
        img = make_slot_image(gen_high=2)
        fake = FakeTransport(slot_image=img)
        with self.assertRaises(NoSnapshotError):
            pull_snapshot(fake, retry_delay=0)

    def test_pull_rejects_nonzero_high_word1_as_no_snapshot(self):
        img = make_slot_image(len_high=3)
        fake = FakeTransport(slot_image=img)
        with self.assertRaises(NoSnapshotError):
            pull_snapshot(fake, retry_delay=0)

    def test_pull_rejects_bad_signature_as_no_snapshot(self):
        img = make_slot_image(signature=b"WRONG!!!")
        fake = FakeTransport(slot_image=img)
        with self.assertRaises(NoSnapshotError):
            pull_snapshot(fake, retry_delay=0)

    def test_pull_rejects_memory_size_mismatch_as_no_snapshot(self):
        img = make_slot_image(len_words=LEN_WORDS_128K, mem_size_kb=64)
        fake = FakeTransport(slot_image=img)
        with self.assertRaises(NoSnapshotError):
            pull_snapshot(fake, retry_delay=0)


class TestTornReadHandling(unittest.TestCase):
    def test_torn_read_generation_change_retries_and_succeeds(self):
        img_g1 = make_slot_image(generation=1, len_words=LEN_WORDS_128K)
        img_g2 = make_slot_image(generation=2, len_words=LEN_WORDS_128K)
        # Attempt 0: hdr1 (gen 1), image (gen 1), hdr2 (gen 2) -> torn!
        # Attempt 1: hdr1 (gen 2), image (gen 2), hdr2 (gen 2) -> coherent!
        responses = [
            img_g1[:16],
            img_g1,
            img_g2[:16],
            img_g2[:16],
            img_g2,
            img_g2[:16],
        ]
        fake = FakeTransport(responses=responses)
        hdr, payload = pull_snapshot(fake, max_retries=3, retry_delay=0)
        self.assertEqual(hdr.generation, 2)
        self.assertEqual(payload, img_g2[16:])
        self.assertEqual(len(fake.responses), 0)

    def test_torn_read_image_header_mismatch_retries_and_succeeds(self):
        img = make_slot_image(generation=5, len_words=LEN_WORDS_64K)
        bad_image = bytearray(img)
        bad_image[:16] = struct.pack("<QQ", 999, LEN_WORDS_64K)
        # Attempt 0: hdr1 (gen 5), image with bad header, hdr2 (gen 5) -> torn!
        # Attempt 1: hdr1 (gen 5), valid image, hdr2 (gen 5) -> coherent!
        responses = [
            img[:16],
            bytes(bad_image),
            img[:16],
            img[:16],
            img,
            img[:16],
        ]
        fake = FakeTransport(responses=responses)
        hdr, payload = pull_snapshot(fake, max_retries=3, retry_delay=0)
        self.assertEqual(hdr.generation, 5)
        self.assertEqual(payload, img[16:])
        self.assertEqual(len(fake.responses), 0)

    def test_torn_read_fails_after_retry_budget(self):
        img_g1 = make_slot_image(generation=1, len_words=LEN_WORDS_128K)
        img_g2 = make_slot_image(generation=2, len_words=LEN_WORDS_128K)
        # 3 attempts all torn by generation change
        responses = [
            img_g1[:16], img_g1, img_g2[:16],
            img_g1[:16], img_g1, img_g2[:16],
            img_g1[:16], img_g1, img_g2[:16],
        ]
        fake = FakeTransport(responses=responses)
        with self.assertRaises(TornReadError) as ctx:
            pull_snapshot(fake, max_retries=3, retry_delay=0)
        self.assertIn("after 3 attempts", str(ctx.exception))


class TestWaitFlow(unittest.TestCase):
    def test_wait_ignores_starting_generation_and_pulls_next(self):
        img_g1 = make_slot_image(generation=1, len_words=LEN_WORDS_128K)
        img_g2 = make_slot_image(generation=2, len_words=LEN_WORDS_128K)
        responses = [
            img_g1[:16],  # baseline check: sees gen 1
            img_g1[:16],  # poll 1: still gen 1 -> wait
            img_g2[:16],  # poll 2: sees gen 2 -> detected!
            img_g2[:16],  # pull hdr1
            img_g2,       # pull image
            img_g2[:16],  # pull hdr2
        ]
        fake = FakeTransport(responses=responses)
        hdr, payload = wait_and_pull(
            fake,
            wait_seconds=5.0,
            poll_interval=0.001,
            retry_delay=0,
        )
        self.assertEqual(hdr.generation, 2)
        self.assertEqual(payload, img_g2[16:])

    def test_wait_from_none_pulls_first_generation(self):
        invalid_hdr = b"\xFF" * 16
        img_g1 = make_slot_image(generation=1, len_words=LEN_WORDS_64K)
        responses = [
            invalid_hdr,  # baseline check: invalid -> baseline is None
            invalid_hdr,  # poll 1: still invalid -> wait
            img_g1[:16],  # poll 2: valid gen 1 appears -> detected!
            img_g1[:16],  # pull hdr1
            img_g1,       # pull image
            img_g1[:16],  # pull hdr2
        ]
        fake = FakeTransport(responses=responses)
        hdr, payload = wait_and_pull(
            fake,
            wait_seconds=5.0,
            poll_interval=0.001,
            retry_delay=0,
        )
        self.assertEqual(hdr.generation, 1)
        self.assertEqual(payload, img_g1[16:])

    def test_wait_with_explicit_after_generation(self):
        img_g3 = make_slot_image(generation=3, len_words=LEN_WORDS_128K)
        img_g4 = make_slot_image(generation=4, len_words=LEN_WORDS_128K)
        responses = [
            img_g3[:16],  # poll 1: gen 3 matches after_generation -> wait
            img_g4[:16],  # poll 2: gen 4 differs -> detected!
            img_g4[:16],  # pull hdr1
            img_g4,       # pull image
            img_g4[:16],  # pull hdr2
        ]
        fake = FakeTransport(responses=responses)
        hdr, payload = wait_and_pull(
            fake,
            wait_seconds=5.0,
            after_generation=3,
            poll_interval=0.001,
            retry_delay=0,
        )
        self.assertEqual(hdr.generation, 4)
        self.assertEqual(payload, img_g4[16:])

    def test_wait_times_out_cleanly(self):
        img = make_slot_image(generation=1, len_words=LEN_WORDS_128K)
        fake = FakeTransport(slot_image=img)
        with self.assertRaises(SnaWaitTimeoutError) as ctx:
            wait_for_generation(fake, wait_seconds=0.05, poll_interval=0.01)
        self.assertIn("timed out after 0.1s waiting for new snapshot", str(ctx.exception))


class TestOutputFileIntegrity(unittest.TestCase):
    def test_failed_pull_leaves_no_output_file_on_no_snapshot(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            out_file = Path(tmpdir) / "test.sna"
            img = make_slot_image(word0_override=0xFFFFFFFFFFFFFFFF)
            fake = FakeTransport(slot_image=img)

            with patch("sna_pull.SSHTransport", return_value=fake):
                with patch("sys.stderr", new_callable=io.StringIO) as mock_stderr:
                    exit_code = main(["--target", "root@mister", "--out", str(out_file)])

            self.assertEqual(exit_code, 1)
            self.assertIn("Error: no snapshot:", mock_stderr.getvalue())
            self.assertFalse(out_file.exists())
            # Ensure no stray temp files left
            self.assertEqual(len(list(Path(tmpdir).iterdir())), 0)

    def test_failed_pull_leaves_no_output_file_on_torn_timeout(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            out_file = Path(tmpdir) / "test.sna"
            img_g1 = make_slot_image(generation=1, len_words=LEN_WORDS_128K)
            img_g2 = make_slot_image(generation=2, len_words=LEN_WORDS_128K)
            responses = []
            for _ in range(5):
                responses.extend([img_g1[:16], img_g1, img_g2[:16]])
            fake = FakeTransport(responses=responses)

            with patch("sna_pull.SSHTransport", return_value=fake):
                with patch("sys.stderr", new_callable=io.StringIO) as mock_stderr:
                    exit_code = main([
                        "--target", "root@mister",
                        "--out", str(out_file),
                    ])

            self.assertEqual(exit_code, 1)
            self.assertIn("Error: snapshot read torn after 5 attempts:", mock_stderr.getvalue())
            self.assertFalse(out_file.exists())
            self.assertEqual(len(list(Path(tmpdir).iterdir())), 0)

    def test_failed_pull_leaves_no_output_file_on_wait_timeout(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            out_file = Path(tmpdir) / "test.sna"
            img = make_slot_image(generation=1, len_words=LEN_WORDS_128K)
            fake = FakeTransport(slot_image=img)

            with patch("sna_pull.SSHTransport", return_value=fake):
                with patch("sys.stderr", new_callable=io.StringIO) as mock_stderr:
                    exit_code = main([
                        "--target", "root@mister",
                        "--out", str(out_file),
                        "--wait", "0.05",
                    ])

            self.assertEqual(exit_code, 1)
            self.assertIn("Error: timed out after 0.1s waiting for new snapshot", mock_stderr.getvalue())
            self.assertFalse(out_file.exists())
            self.assertEqual(len(list(Path(tmpdir).iterdir())), 0)

    def test_successful_pull_writes_exact_file_and_reports(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            out_file = Path(tmpdir) / "snapshot.sna"
            img = make_slot_image(generation=42, len_words=LEN_WORDS_128K, fill_byte=0xBB)
            fake = FakeTransport(slot_image=img)

            with patch("sna_pull.SSHTransport", return_value=fake):
                with patch("sys.stdout", new_callable=io.StringIO) as mock_stdout:
                    exit_code = main(["--target", "root@mister", "--out", str(out_file)])

            self.assertEqual(exit_code, 0)
            self.assertTrue(out_file.exists())
            self.assertEqual(out_file.read_bytes(), img[16:])
            self.assertIn("Generation 42, 131328 bytes written to", mock_stdout.getvalue())
            self.assertEqual(len(list(Path(tmpdir).iterdir())), 1)

    def test_after_generation_without_wait_matching_fails(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            out_file = Path(tmpdir) / "snapshot.sna"
            img = make_slot_image(generation=42, len_words=LEN_WORDS_128K)
            fake = FakeTransport(slot_image=img)

            with patch("sna_pull.SSHTransport", return_value=fake):
                with patch("sys.stderr", new_callable=io.StringIO) as mock_stderr:
                    exit_code = main([
                        "--target", "root@mister",
                        "--out", str(out_file),
                        "--after-generation", "42",
                    ])

            self.assertEqual(exit_code, 1)
            self.assertIn("matches --after-generation", mock_stderr.getvalue())
            self.assertFalse(out_file.exists())


class TestReadCommandAndTransport(unittest.TestCase):
    def test_read_command_syntax(self):
        cmd = read_command(0x3E000000, 16)
        self.assertIn("b=1040187392", cmd)
        self.assertIn("s=16", cmd)
        self.assertIn("/dev/mem", cmd)

    def test_read_command_alignment_check(self):
        with self.assertRaises(SnaPullError) as ctx:
            read_command(0x3E000001, 16)
        self.assertIn("not 16-byte aligned", str(ctx.exception))

    def test_read_command_size_check(self):
        with self.assertRaises(SnaPullError) as ctx:
            read_command(0x3E000000, 0)
        self.assertIn("size must be positive", str(ctx.exception))

    def test_transport_error_on_command_failure(self):
        fake = FakeTransport(responses=[CommandResult(1, "", "mmap: Operation not permitted")])
        with self.assertRaises(SnaTransportError) as ctx:
            pull_snapshot(fake)
        self.assertIn("reading /dev/mem failed (1)", str(ctx.exception))

    def test_transport_error_on_ssh_driver_error(self):
        fake = FakeTransport(responses=[DriverError("SSH connection failed")])
        with self.assertRaises(SnaTransportError) as ctx:
            pull_snapshot(fake)
        self.assertIn("SSH connection failed", str(ctx.exception))

    def test_truncated_data_from_device(self):
        fake = FakeTransport(responses=[b"\x00" * 8])  # returns fewer than 16 bytes
        with self.assertRaises(SnaTruncatedError) as ctx:
            pull_snapshot(fake)
        self.assertIn("read 8 bytes from device, expected 16", str(ctx.exception))


if __name__ == "__main__":
    unittest.main()
