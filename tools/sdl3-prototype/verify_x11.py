"""Verify the prototype with X11 focus, XTest input and WM close messages.

Run in a private Xvfb display. This Linux-only probe does not use SDL_PushEvent
and does not claim to exercise a physical keyboard or the Cocos game backend.
"""
import argparse
from contextlib import contextmanager
import ctypes as C
import json
import os
from pathlib import Path
import subprocess
import time

from verify import digest


class ClientData(C.Union):
    _fields_ = [("bytes", C.c_char * 20), ("shorts", C.c_short * 10),
                ("longs", C.c_long * 5)]


class ClientMessage(C.Structure):
    _fields_ = [("type", C.c_int), ("serial", C.c_ulong), ("send_event", C.c_int),
                ("display", C.c_void_p), ("window", C.c_ulong),
                ("message_type", C.c_ulong), ("format", C.c_int), ("data", ClientData)]


class XEvent(C.Union):
    _fields_ = [("client", ClientMessage), ("padding", C.c_long * 24)]


class XErrorEvent(C.Structure):
    _fields_ = [("type", C.c_int), ("display", C.c_void_p), ("resourceid", C.c_ulong),
                ("serial", C.c_ulong), ("error_code", C.c_ubyte),
                ("request_code", C.c_ubyte), ("minor_code", C.c_ubyte)]


ErrorHandler = C.CFUNCTYPE(C.c_int, C.c_void_p, C.POINTER(XErrorEvent))


class WindowAttributes(C.Structure):
    _fields_ = [("x", C.c_int), ("y", C.c_int), ("width", C.c_int), ("height", C.c_int),
                ("border_width", C.c_int), ("depth", C.c_int), ("visual", C.c_void_p),
                ("root", C.c_ulong), ("window_class", C.c_int), ("bit_gravity", C.c_int),
                ("win_gravity", C.c_int), ("backing_store", C.c_int),
                ("backing_planes", C.c_ulong), ("backing_pixel", C.c_ulong),
                ("save_under", C.c_int), ("colormap", C.c_ulong),
                ("map_installed", C.c_int), ("map_state", C.c_int),
                ("all_event_masks", C.c_long), ("your_event_mask", C.c_long),
                ("do_not_propagate_mask", C.c_long), ("override_redirect", C.c_int),
                ("screen", C.c_void_p)]


class X11:
    def __init__(self):
        self.library = C.CDLL("libX11.so.6")
        self.test = C.CDLL("libXtst.so.6")
        pointer, window, integer = C.c_void_p, C.c_ulong, C.c_int
        signatures = [
            ("XOpenDisplay", pointer, [C.c_char_p]),
            ("XDefaultRootWindow", window, [pointer]),
            ("XCreateSimpleWindow", window, [pointer, window, integer, integer,
                                           C.c_uint, C.c_uint, C.c_uint, window, window]),
            ("XMapWindow", integer, [pointer, window]),
            ("XGetWindowAttributes", integer, [pointer, window, C.POINTER(WindowAttributes)]),
            ("XSetInputFocus", integer, [pointer, window, integer, window]),
            ("XResizeWindow", integer, [pointer, window, C.c_uint, C.c_uint]),
            ("XTranslateCoordinates", integer, [pointer, window, window, integer, integer,
                                               C.POINTER(integer), C.POINTER(integer), C.POINTER(window)]),
            ("XSync", integer, [pointer, integer]),
            ("XSetErrorHandler", pointer, [pointer]),
            ("XKeysymToKeycode", C.c_ubyte, [pointer, window]),
            ("XInternAtom", window, [pointer, C.c_char_p, integer]),
            ("XQueryTree", integer, [pointer, window, C.POINTER(window), C.POINTER(window),
                                    C.POINTER(C.POINTER(window)), C.POINTER(C.c_uint)]),
            ("XGetWindowProperty", integer, [pointer, window, window, C.c_long, C.c_long,
                                            integer, window, C.POINTER(window), C.POINTER(integer),
                                            C.POINTER(window), C.POINTER(window), C.POINTER(pointer)]),
            ("XFree", integer, [pointer]),
            ("XSendEvent", integer, [pointer, window, integer, C.c_long, pointer]),
            ("XCloseDisplay", integer, [pointer]),
        ]
        for name, result, arguments in signatures:
            function = getattr(self.library, name)
            function.restype, function.argtypes = result, arguments
        self.test.XTestFakeKeyEvent.argtypes = [pointer, C.c_uint, integer, window]
        self.test.XTestFakeKeyEvent.restype = integer
        self.test.XTestFakeMotionEvent.argtypes = [pointer, integer, integer, integer, window]
        self.test.XTestFakeMotionEvent.restype = integer
        self.test.XTestFakeButtonEvent.argtypes = [pointer, C.c_uint, integer, window]
        self.test.XTestFakeButtonEvent.restype = integer
        self.display = self.library.XOpenDisplay(None)
        if not self.display:
            raise RuntimeError("Cannot open DISPLAY; run with xvfb-run -a")
        self.root = self.library.XDefaultRootWindow(self.display)

    def close(self):
        self.library.XCloseDisplay(self.display)

    def sync(self):
        self.library.XSync(self.display, 0)

    def atom(self, name):
        return self.library.XInternAtom(self.display, name.encode(), 0)

    @contextmanager
    def window_query(self, window):
        # XQueryTree is a snapshot: a sibling (e.g. SDL's startup helper) may
        # disappear before its property/attributes reply. Xlib's default error
        # handler exits the process, before Python can inspect the return code.
        # This single-threaded probe scopes the process-wide handler to one
        # candidate, drains replies, and restores it before the callback dies.
        self.sync()
        errors = []

        @ErrorHandler
        def handler(display, event):
            e = event.contents
            errors.append((display, e.error_code, e.request_code, e.resourceid))
            return 0

        previous = self.library.XSetErrorHandler(C.cast(handler, C.c_void_p))
        try:
            yield
        finally:
            try:
                self.sync()
            finally:
                self.library.XSetErrorHandler(previous)
        for display, code, request, resource in errors:
            if not (display == self.display and code == 3 and resource == window
                    and request in (3, 20)):  # BadWindow; GetWindowAttributes/GetProperty
                raise RuntimeError(f"X11 query error {code}, request {request}, resource {resource:#x}")

    def process_window(self, pid):
        root, parent, count = C.c_ulong(), C.c_ulong(), C.c_uint()
        children = C.POINTER(C.c_ulong)()
        if not self.library.XQueryTree(self.display, self.root, C.byref(root), C.byref(parent),
                                      C.byref(children), C.byref(count)):
            raise RuntimeError("XQueryTree failed")
        try:
            for index in range(count.value):
                window = children[index]
                with self.window_query(window):
                    kind, bits, size, remaining, data = C.c_ulong(), C.c_int(), C.c_ulong(), C.c_ulong(), C.c_void_p()
                    # XA_CARDINAL is 6. Xlib exposes format-32 elements as unsigned long.
                    status = self.library.XGetWindowProperty(self.display, window,
                        self.atom("_NET_WM_PID"), 0, 1, 0, 6, C.byref(kind), C.byref(bits),
                        C.byref(size), C.byref(remaining), C.byref(data))
                    try:
                        if status == 0 and kind.value == 6 and bits.value == 32 and size.value == 1 and data:
                            if C.cast(data, C.POINTER(C.c_ulong))[0] == pid:
                                attributes = WindowAttributes()
                                if (self.library.XGetWindowAttributes(self.display, window, C.byref(attributes))
                                        and attributes.map_state == 2):  # IsViewable
                                    return window
                    finally:
                        if data:
                            self.library.XFree(data)
        finally:
            if children:
                self.library.XFree(children)
        return None

    def focus(self, window):
        self.library.XSetInputFocus(self.display, window, 1, 0)
        self.sync()

    def key(self, down, keysym=ord("a")):
        keycode = self.library.XKeysymToKeycode(self.display, keysym)
        if not keycode or not self.test.XTestFakeKeyEvent(self.display, keycode, down, 0):
            raise RuntimeError("XTest keyboard injection failed")
        self.sync()

    def request_close(self, window):
        event = XEvent()
        event.client.type = 33  # ClientMessage
        event.client.display = self.display
        event.client.window = window
        event.client.message_type = self.atom("WM_PROTOCOLS")
        event.client.format = 32
        event.client.data.longs[0] = self.atom("WM_DELETE_WINDOW")
        if not self.library.XSendEvent(self.display, window, 0, 0, C.byref(event)):
            raise RuntimeError("WM_DELETE_WINDOW injection failed")
        self.sync()

    def motion(self, window, x, y):
        root_x, root_y, child = C.c_int(), C.c_int(), C.c_ulong()
        if not self.library.XTranslateCoordinates(self.display, window, self.root, x, y,
                C.byref(root_x), C.byref(root_y), C.byref(child)):
            raise RuntimeError("Cannot translate pointer coordinates to X11 root")
        if not self.test.XTestFakeMotionEvent(self.display, -1, root_x.value, root_y.value, 0):
            raise RuntimeError("XTest pointer motion failed")
        self.sync()

    def button(self, down, button=1):
        if not self.test.XTestFakeButtonEvent(self.display, button, down, 0):
            raise RuntimeError("XTest pointer button failed")
        self.sync()

    def resize(self, window, width, height):
        self.library.XResizeWindow(self.display, window, width, height)
        self.sync()


def probe(engine, script, output, report, pointer=False):
    x11 = X11()
    process = None
    try:
        helper = x11.library.XCreateSimpleWindow(x11.display, x11.root, 700, 0, 100, 100, 0, 0, 0)
        x11.library.XMapWindow(x11.display, helper)
        x11.sync()
        environment = dict(os.environ, SDL_VIDEO_DRIVER="x11")
        with (output / "process.log").open("wb") as log:
            process = subprocess.Popen([str(engine), str(script), str(output / "run")],
                                       cwd=engine.parent, env=environment, stdout=log, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + 20

            def events():
                path = output / "run/events.txt"
                return path.read_text(encoding="utf-8").splitlines() if path.exists() else []

            def wait_for(check):
                while True:
                    result = check()
                    if result:
                        return result
                    if process.poll() is not None:
                        raise RuntimeError(f"Prototype exited early: {process.returncode}; events={events()}")
                    if time.monotonic() >= deadline:
                        raise TimeoutError(f"X11 probe deadline expired; events={events()}")
                    time.sleep(0.02)

            window = wait_for(lambda: x11.process_window(process.pid))
            x11.focus(window)
            if pointer:
                from pointer_probe import drive
                drive(x11, window, helper, output / "run", events, wait_for)
            else:
                drive_keys(x11, window, helper, events, wait_for)
            x11.request_close(window)
            wait_for(lambda: "close-accepted:0" in events())
            # Let the event pump settle after a veto, before sending another
            # request: SDL's automatic QUIT used to accept this same OS close.
            settle = time.monotonic() + 0.2
            while time.monotonic() < settle:
                if process.poll() is not None or events().count("close-accepted:1"):
                    raise RuntimeError("First OS close bypassed the script veto")
                time.sleep(0.02)
            x11.request_close(window)
            result = process.wait(timeout=max(0.01, deadline - time.monotonic()))
            observed = json.loads((output / "run/report.json").read_text(encoding="utf-8"))
            sequence = events()
            expected = {"completed": True, "self_test": False, "video_driver": "x11",
                        "focus_losses": 1, "close_vetoes": 1}
            if result != 0 or any(observed.get(key) != value for key, value in expected.items()):
                raise RuntimeError(f"Unexpected exit or event counts: {result}, {observed}")
            if pointer:
                from pointer_probe import verify
                report["pointer_events"] = verify(output / "run", observed)
                expected_sequence = ["focus:0", "focus:1", "close-accepted:0", "close-accepted:1"]
            else:
                expected_sequence = ["key:17:1:1:0:4", "key:65:1:1:0:4", "key:65:0:0:0:4",
                                     "key:17:0:0:0:0", "focus:0", "focus:1",
                                     "key:65:1:1:0:0", "key:65:0:0:0:0", "close-accepted:0", "close-accepted:1"]
                if observed["key_downs"] != 3 or observed["key_ups"] != 3:
                    raise RuntimeError(f"Unexpected keyboard counts: {observed}")
            # An initially unfocused X11 window can first receive focus gain.
            if sequence and sequence[0] == "focus:1":
                sequence = sequence[1:]
            if sequence != expected_sequence:
                raise RuntimeError(f"Unexpected OS event sequence: {events()}")
            if digest(engine) != report["engine_sha256"] or digest(script) != report["script_sha256"]:
                raise RuntimeError("Executable or script changed during verification")
            report.update(observations=observed, events=events(), passed=True)
    finally:
        if process is not None:
            if process.poll() is None:
                report["forced_stop"] = True
                process.kill()
                process.wait()
            report["exit_code"] = process.returncode
        x11.close()


def drive_keys(x11, window, helper, events, wait_for):
    x11.key(True, 0xffe3)  # XK_Control_L
    wait_for(lambda: "key:17:1:1:0:4" in events())
    x11.key(True)
    wait_for(lambda: "key:65:1:1:0:4" in events())
    x11.focus(helper)
    wait_for(lambda: "focus:0" in events())
    x11.key(False)
    x11.key(False, 0xffe3)
    previous_gains = events().count("focus:1")
    x11.focus(window)
    wait_for(lambda: events().count("focus:1") == previous_gains + 1)
    x11.key(True)
    wait_for(lambda: "key:65:1:1:0:0" in events())
    x11.key(False)
    wait_for(lambda: "key:65:0:0:0:0" in events())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--script", type=Path)
    parser.add_argument("--pointer", action="store_true", help="Check native pointer, capture and resize")
    args = parser.parse_args()
    engine = args.engine.resolve(strict=True)
    script = (args.script or Path(__file__).with_name(
        "pointer_fixture.tjs" if args.pointer else "fixture.tjs")).resolve(strict=True)
    output = args.destination.resolve()
    if output.exists():
        parser.error("Destination must be new")
    output.mkdir(parents=True)
    report = {"engine": str(engine), "engine_sha256": digest(engine), "script_sha256": digest(script),
              "passed": False, "forced_stop": False,
              "input_scope": "X11 focus/resize, XTest devices, WM_DELETE_WINDOW; not physical hardware"}
    try:
        probe(engine, script, output, report, args.pointer)
    except (OSError, RuntimeError, ValueError, TimeoutError, subprocess.TimeoutExpired) as error:
        report["failure"] = str(error)
    (output / "verification.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
