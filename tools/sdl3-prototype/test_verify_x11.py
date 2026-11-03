"""Native X11 checks for the OS-input verifier; run under xvfb-run."""
import ctypes as C
import os
import unittest

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


if __name__ == "__main__":
    unittest.main()
