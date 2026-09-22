import unittest
from unittest import mock
import input_replay as replay


class Clock:
    def __init__(self):
        self.now = 0
    def time(self):
        return self.now
    def sleep(self, delay):
        self.now += delay


class ReplayTests(unittest.TestCase):
    def test_absolute_deadlines_do_not_accumulate_write_latency(self):
        clock, sent = Clock(), []
        def emit(code, value):
            sent.append((round(clock.now, 3), code, value))
            clock.now += .03
        replay.play([[0, 106, 1], [.1, 106, 0], [.2, 29, 1]], .3,
                    emit, clock.time, clock.sleep)
        self.assertEqual(sent, [(0, 106, 1), (.1, 106, 0), (.2, 29, 1), (.3, 29, 0)])

    def test_interrupt_releases_key_even_if_press_write_raises(self):
        sent = []
        def emit(code, value):
            sent.append((code, value))
            if value == 1:
                raise KeyboardInterrupt
        with self.assertRaises(KeyboardInterrupt):
            replay.play([[0, 29, 1]], 1, emit)
        self.assertEqual(sent, [(29, 1), (29, 0)])

    def test_bad_schedule_rejected(self):
        for events in ([[1, 29, 1], [.5, 29, 0]], [[float('nan'), 29, 1]],
                       [[0, 29, 2]], [[0, 768, 1]], [[3, 29, 1]]):
            with self.subTest(events=events), self.assertRaises(ValueError):
                replay.validate({'version': 1, 'duration': 2, 'events': events})

    def test_setup_ioctl_failure_closes_descriptor(self):
        with mock.patch.object(replay.os, 'open', return_value=17), \
             mock.patch.object(replay.os, 'close') as close, \
             mock.patch.object(replay.fcntl, 'ioctl', side_effect=OSError('setup failed')):
            with self.assertRaises(OSError):
                with replay.Keyboard():
                    self.fail('setup should fail')
            close.assert_called_once_with(17)

    def test_one_failed_release_does_not_skip_other_held_keys(self):
        sent = []
        def emit(code, value):
            sent.append((code, value))
            if code == 29 and value == 0:
                raise OSError('write failed')
        clock = Clock()
        with self.assertRaises(OSError):
            replay.play([[0, 29, 1], [0, 106, 1]], 1, emit, clock.time, clock.sleep)
        self.assertEqual(sent, [(29, 1), (106, 1), (29, 0), (106, 0)])

if __name__ == '__main__':
    unittest.main()
