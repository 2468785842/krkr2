"""Native X11 checks for the OS-input verifier; run under xvfb-run."""
import ctypes as C
import os
import unittest
from unittest import mock

from verify_x11 import X11


class WindowReadinessTest(unittest.TestCase):
    def test_pid_property_does_not_make_an_unmapped_window_ready(self):
        x11 = X11()
        try:
            window = x11.library.XCreateSimpleWindow(x11.display, x11.root, 0, 0, 100, 100, 0, 0, 0)
            x11.library.XChangeProperty.argtypes = [C.c_void_p, C.c_ulong, C.c_ulong, C.c_ulong,
                                                   C.c_int, C.c_int, C.c_void_p, C.c_int]
            x11.library.XUnmapWindow.argtypes = [C.c_void_p, C.c_ulong]
            pid = C.c_ulong(os.getpid())
            x11.library.XChangeProperty(x11.display, window, x11.atom("_NET_WM_PID"),
                                       6, 32, 0, C.byref(pid), 1)
            x11.sync()
            self.assertIsNone(x11.process_window(os.getpid()))
            x11.library.XMapWindow(x11.display, window)
            x11.sync()
            self.assertEqual(x11.process_window(os.getpid()), window)
            x11.library.XUnmapWindow(x11.display, window)
            x11.sync()
            self.assertIsNone(x11.process_window(os.getpid()))
        finally:
            x11.close()


class DestroyedWindowTest(unittest.TestCase):
    def setUp(self):
        self.x11 = X11()
        lib = self.x11.library
        lib.XDestroyWindow.argtypes = [C.c_void_p, C.c_ulong]
        lib.XChangeProperty.argtypes = [C.c_void_p, C.c_ulong, C.c_ulong, C.c_ulong,
                                       C.c_int, C.c_int, C.c_void_p, C.c_int]

    def tearDown(self):
        self.x11.close()

    def candidate(self):
        x = self.x11
        window = x.library.XCreateSimpleWindow(x.display, x.root, 0, 0, 100, 100, 0, 0, 0)
        pid = C.c_ulong(os.getpid())
        x.library.XChangeProperty(x.display, window, x.atom("_NET_WM_PID"),
                                 6, 32, 0, C.byref(pid), 1)
        x.library.XMapWindow(x.display, window)
        x.sync()
        return window

    def check_destruction(self, after_property):
        victim, survivor = self.candidate(), self.candidate()
        original = self.x11.library.XGetWindowProperty
        destroyed = []

        def property(*args):
            if args[1] != victim:
                return original(*args)
            if after_property:
                result = original(*args)
            self.x11.library.XDestroyWindow(self.x11.display, victim)
            destroyed.append(victim)
            if not after_property:
                result = original(*args)
            return result

        # The root snapshot contains victim. Destroy it on the actual X server
        # exactly before the next native request; no Xlib error is fabricated.
        with mock.patch.object(self.x11.library, "XGetWindowProperty", side_effect=property):
            self.assertEqual(self.x11.process_window(os.getpid()), survivor)
        self.assertEqual(destroyed, [victim])
        self.assertEqual(self.x11.process_window(os.getpid()), survivor)

    def test_destroyed_between_tree_snapshot_and_property(self):
        self.check_destruction(after_property=False)

    def test_destroyed_between_property_and_attributes(self):
        self.check_destruction(after_property=True)

    def test_unexpected_native_error_is_not_ignored(self):
        victim = self.candidate()
        original = self.x11.library.XGetWindowProperty

        def bad_atom(*args):
            if args[1] == victim:
                args = list(args)
                args[2] = 0  # Invalid property atom: real BadAtom, not BadWindow.
            return original(*args)

        with mock.patch.object(self.x11.library, "XGetWindowProperty", side_effect=bad_atom):
            with self.assertRaisesRegex(RuntimeError, "X11.*error.*5"):
                self.x11.process_window(os.getpid())
        self.assertEqual(self.x11.process_window(os.getpid()), victim)


if __name__ == "__main__":
    unittest.main()
