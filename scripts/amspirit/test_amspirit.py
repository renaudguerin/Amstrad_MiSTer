#!/usr/bin/env python3
"""Focused tests for the AmSpirit HTTP helper's command ordering."""

from __future__ import annotations

import io
import json
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import amspirit


class Response(io.BytesIO):
    def __enter__(self):
        return self

    def __exit__(self, *_exc):
        self.close()


class FakeAmSpirit(amspirit.AmSpirit):
    def __init__(self, *, deadline=0.1, ping=None, command=None, commands=None,
                 applied=None, emu_states=None):
        super().__init__("http://test", deadline)
        self.ping = ping or {"frontend": "lite", "version": "1.16.0", "core": 2491682,
                             "emu": {"applied_cmd_seq": 0, "paused": False,
                                     "frames": 12, "crtc_type": 0}}
        self.command = command or {"ok": True, "cmd_seq": 7}
        self.commands = list(commands or [self.command])
        self.applied = list([7] if applied is None else applied)
        self.emu_states = list(emu_states or [])
        self.waiting_for_seq = False
        self.requests = []
        self.poll_timeouts = []
        self.events = []
        self.posts = 0

    def _req(self, method, path, body=None, ctype=None, timeout=30.0):
        self.requests.append((method, path, body, timeout))
        self.events.append(f"request:{method}:{path}")
        if method == "POST":
            self.posts += 1
            command = self.commands.pop(0) if self.commands else self.command
            return Response(json.dumps(command).encode())
        return Response(json.dumps(self.ping).encode())

    def get_json(self, path, timeout=None):
        self.poll_timeouts.append(timeout)
        if path == "/api/ping" and self.posts and self.waiting_for_seq and self.applied:
            ping = dict(self.ping)
            applied = self.applied.pop(0)
            ping["emu"] = dict(self.ping["emu"], applied_cmd_seq=applied)
            self.events.append(f"poll:applied:{applied}")
            return ping
        if path == "/api/ping" and self.posts and self.emu_states:
            state = self.emu_states.pop(0)
            ping = dict(self.ping)
            ping["emu"] = dict(self.ping["emu"], **state)
            self.events.append(f"poll:emu:{state}")
            return ping
        self.events.append(f"poll:{path}")
        return self.ping

    def wait_applied_cmd(self, seq, path="command"):
        self.events.append(f"wait-applied:{seq}")
        self.waiting_for_seq = True
        try:
            return super().wait_applied_cmd(seq, path)
        finally:
            self.waiting_for_seq = False


class CommandOrderingTests(unittest.TestCase):
    def test_post_json_waits_for_applied_command_sequence(self):
        ams = FakeAmSpirit(applied=[6, 7])

        result = ams.post_json("/api/config", {"paused": True})

        self.assertEqual(result["cmd_seq"], 7)
        self.assertEqual(len(ams.poll_timeouts), 3)

    def test_apply_settings_reads_back_only_after_final_command_ack(self):
        class SettingsAms(FakeAmSpirit):
            def get_json(self, path, timeout=None):
                if path == "/api/config":
                    self.events.append("read:config")
                    return {"crtc_type": 0}
                if path == "/api/render":
                    self.events.append("read:render")
                    return {"monitor": 0}
                return super().get_json(path, timeout)

        ams = SettingsAms(commands=[{"ok": True, "cmd_seq": 7},
                                    {"ok": True, "cmd_seq": 8}],
                          applied=[7, 8])

        with mock.patch.object(amspirit.time, "sleep",
                               side_effect=AssertionError("acknowledged settings must not sleep")):
            amspirit.apply_settings(ams, {"config": {"crtc_type": 0},
                                          "render": {"monitor": 0}})

        self.assertLess(ams.events.index("wait-applied:8"), ams.events.index("read:config"))

    def test_command_wait_bounds_each_http_poll_by_remaining_deadline(self):
        ams = FakeAmSpirit(deadline=0.02, applied=[0] * 20)

        with mock.patch.object(amspirit.time, "sleep", lambda _seconds: None):
            with self.assertRaisesRegex(amspirit.OracleError, "cmd_seq 7.*deadline"):
                ams.post_json("/api/config", {"paused": True})

        poll_timeouts = [timeout for timeout in ams.poll_timeouts if timeout is not None]
        self.assertTrue(poll_timeouts)
        self.assertTrue(all(0 < timeout <= ams.deadline for timeout in poll_timeouts))

    def test_keytype_applied_ack_precedes_existing_autotype_completion_wait(self):
        ams = FakeAmSpirit(applied=[0, 7], emu_states=[
            {"autotyping": True, "autotype_remaining": 2, "paused": False},
            {"autotyping": False, "autotype_remaining": 0, "paused": False},
        ])

        with mock.patch.object(amspirit.time, "sleep", lambda _seconds: None):
            ams.type_keys("RUN")

        applied = ams.events.index("wait-applied:7")
        typing = next(i for i, event in enumerate(ams.events) if event.startswith("poll:emu:"))
        self.assertLess(applied, typing)

    def test_cpr_load_acknowledges_before_hard_reset_is_queued(self):
        with tempfile.TemporaryDirectory() as tmp:
            cpr = Path(tmp) / "game.cpr"
            cpr.write_bytes(b"RIFF0000AMS ")
            ams = FakeAmSpirit(commands=[{"ok": True, "cmd_seq": 6},
                                         {"ok": True, "cmd_seq": 7}],
                               applied=[6, 7])

            ams.load_media(cpr, hard_reset=True)

        media = next(i for i, event in enumerate(ams.events) if event.startswith("request:POST:/api/media"))
        media_applied = ams.events.index("poll:applied:6")
        reset = next(i for i, event in enumerate(ams.events)
                     if event.startswith("request:POST:/api/config"))
        self.assertLess(media, media_applied)
        self.assertLess(media_applied, reset)

    def test_missing_sequence_field_fails_closed(self):
        ams = FakeAmSpirit(command={"ok": True})

        with self.assertRaisesRegex(amspirit.OracleError, "cmd_seq"):
            ams.post_json("/api/config", {"paused": True})

    def test_old_server_without_applied_sequence_is_rejected_before_mutation(self):
        ams = FakeAmSpirit(ping={"frontend": "lite", "version": "1.15.4",
                                 "emu": {"paused": False, "frames": 12}})

        with self.assertRaisesRegex(amspirit.OracleError, "applied_cmd_seq"):
            ams.post_json("/api/config", {"paused": True})

        self.assertFalse(any(method == "POST" for method, *_ in ams.requests))


class SnaPolicyTests(unittest.TestCase):
    def test_sna_load_defaults_to_no_reset_and_accepts_explicit_policy(self):
        with tempfile.TemporaryDirectory() as tmp:
            sna = Path(tmp) / "state.sna"
            sna.write_bytes(b"MV - SNA" + bytes(0x100 - 8))
            ams = FakeAmSpirit()

            result = ams.load_media(sna, crtc="keep")

        self.assertEqual(result, 12)
        method, path, _body, _timeout = next(r for r in ams.requests if r[0] == "POST")
        self.assertEqual(method, "POST")
        self.assertIn("crtc=keep", path)
        self.assertEqual(len([r for r in ams.requests if r[0] == "POST"]), 1)

    def test_keep_requires_known_116_capability_before_mutation(self):
        with tempfile.TemporaryDirectory() as tmp:
            sna = Path(tmp) / "state.sna"
            sna.write_bytes(b"MV - SNA" + bytes(0x100 - 8))
            ams = FakeAmSpirit(ping={"frontend": "lite", "version": "1.15.4",
                                     "emu": {"applied_cmd_seq": 0, "paused": False,
                                             "frames": 12, "crtc_type": 0}})

            with self.assertRaisesRegex(amspirit.OracleError, "CRTC keep.*1.16"):
                ams.load_media(sna, hard_reset=False, crtc="keep")

        self.assertFalse(any(method == "POST" for method, *_ in ams.requests))

    def test_sna_rejects_hard_reset_before_mutation(self):
        with tempfile.TemporaryDirectory() as tmp:
            sna = Path(tmp) / "state.sna"
            sna.write_bytes(b"MV - SNA" + bytes(0x100 - 8))
            ams = FakeAmSpirit()

            with self.assertRaisesRegex(amspirit.OracleError, "SNA.*hard reset"):
                ams.load_media(sna, hard_reset=True)

        self.assertFalse(any(method == "POST" for method, *_ in ams.requests))

    def test_identity_reports_116_ping_shape(self):
        ams = FakeAmSpirit()
        result = amspirit.identity(ams)

        self.assertEqual(result["frontend"], "lite")
        self.assertEqual(result["version"], "1.16.0")
        self.assertEqual(result["core"], 2491682)

    def test_load_cli_defaults_to_sna_policy_and_inferred_reset(self):
        args = amspirit.parse_args(["load", "state.sna"])

        self.assertEqual(args.crtc, "sna")
        self.assertIsNone(args.reset)

    def test_post_media_crtc_mismatch_uses_live_emulator_type(self):
        class SettingsRead:
            deadline = 1.0

            def get_json(self, path):
                if path == "/api/config":
                    # Saved preference can still name the requested CRTC.
                    return {"crtc_type": 0}
                if path == "/api/render":
                    return {}
                raise AssertionError(path)

            def emu(self, timeout=None):
                return {"crtc_type": 1, "frames": 15, "ticks": 123}

        effective = amspirit.read_effective_settings(
            SettingsRead(), {"config": {"crtc_type": 0}, "render": {}})

        self.assertEqual(effective["mismatches"]["crtc_type"],
                         {"requested": 0, "applied": 1})


if __name__ == "__main__":
    unittest.main()
