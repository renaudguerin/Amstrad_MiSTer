#!/usr/bin/env python3
"""Bounded timed keyboard replay through uinput on MiSTer.

No dependencies beyond Linux and Python 3. No input capture, controller axes,
core loading, mapping changes, or frame-determinism claim.
"""
import argparse
import fcntl
import json
import math
import os
from pathlib import Path
import signal
import struct
import time

EVENT = struct.Struct('@llHHi')  # native Linux input_event, including 32-bit MiSTer
NAME = b'B17 replay keyboard'
MAX_SECONDS = 600


def finite(value, low, high):
    return type(value) in (float, int) and math.isfinite(value) and low <= value <= high


def validate(data):
    if not isinstance(data, dict) or type(data.get('version')) is not int or data.get('version') != 1:
        raise ValueError('expected version 1 replay schedule')
    duration = data.get('duration')
    events = data.get('events')
    if not finite(duration, 0, MAX_SECONDS) or not isinstance(events, list) or len(events) > 100000:
        raise ValueError('invalid duration or events (maximum 600 seconds / 100000 events)')
    previous = 0
    held = set()
    for event in events:
        if not isinstance(event, list) or len(event) != 3:
            raise ValueError('events must be [seconds, Linux keycode, 0 or 1]')
        at, code, value = event
        if not finite(at, previous, duration) or type(code) is not int or not 1 <= code <= 255:
            raise ValueError('invalid event time or keyboard code')
        if type(value) is not int or value not in (0, 1):
            raise ValueError('only press/release values 1/0 are supported')
        if (value == 1) == (code in held):
            raise ValueError('unbalanced key transition')
        if value:
            held.add(code)
        else:
            held.remove(code)
        previous = at
    return events, duration


def play(events, duration, emit, now=time.monotonic, sleep=time.sleep):
    """Absolute deadlines avoid per-event drift; release via the same device."""
    start, held = now(), set()
    try:
        for at, code, value in events:
            sleep(max(0, start + at - now()))
            # Track before writing: an interrupted write may already reach the kernel.
            if value:
                held.add(code)
            emit(code, value)
            if not value:
                held.discard(code)
        sleep(max(0, start + duration - now()))
    finally:
        # Try every release even if one fails. Closing uinput remains a last resort.
        failure = None
        for code in sorted(held):
            try:
                emit(code, 0)
            except OSError as exc:
                failure = exc
        if failure:
            raise failure


class Keyboard:
    def __enter__(self):
        self.fd = os.open('/dev/uinput', os.O_WRONLY)
        try:
            fcntl.ioctl(self.fd, 0x40045564, 1)  # UI_SET_EVBIT EV_KEY
            for code in range(1, 256):
                fcntl.ioctl(self.fd, 0x40045565, code)  # UI_SET_KEYBIT
            # Legacy uinput_user_dev works with MiSTer's kernel. BUS_USB, private
            # VID/PID 0000:b017. No impersonation of the physical keyboard.
            desc = struct.pack('80sHHHHI', NAME, 3, 0, 0xb017, 1, 0) + bytes(64 * 4 * 4)
            os.write(self.fd, desc)
            fcntl.ioctl(self.fd, 0x5501)  # UI_DEV_CREATE
        except BaseException:
            os.close(self.fd)
            raise
        return self

    def emit(self, code, value):
        os.write(self.fd, EVENT.pack(0, 0, 1, code, value))
        os.write(self.fd, EVENT.pack(0, 0, 0, 0, 0))  # SYN_REPORT

    def __exit__(self, *args):
        try:
            # Give Main time to consume the final release before removal.
            time.sleep(.1)
            fcntl.ioctl(self.fd, 0x5502)  # UI_DEV_DESTROY
        finally:
            os.close(self.fd)


def interrupted(signum, frame):
    raise KeyboardInterrupt('signal %s' % signum)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('file', type=Path)
    parser.add_argument('--settle', type=float, default=1, help='uinput discovery delay before time zero')
    args = parser.parse_args()
    for sig in (signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, interrupted)
    events, duration = validate(json.loads(args.file.read_text()))
    if not finite(args.settle, .1, 10):
        parser.error('--settle must be 0.1..10 seconds')
    with Keyboard() as keyboard:
        time.sleep(args.settle)
        play(events, duration, keyboard.emit)
    print('Replayed %d transitions; owned keys released' % len(events))


if __name__ == '__main__':
    main()
