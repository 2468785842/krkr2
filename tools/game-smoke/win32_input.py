"""Win32 desktop input for an owned test process; no engine event injection."""
import ctypes
from ctypes import wintypes
import os
import sys


class Desktop:
    def __init__(self):
        if sys.platform != "win32":
            raise RuntimeError("The native focus probe requires Windows")
        self.user = ctypes.WinDLL("user32", use_last_error=True)
        self.kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        self.helper = None
        self.keys = set()
        self.last_caption = None
        self.class_name = None
        self.instance = None
        self.enum_callback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
        definitions = {
            "EnumWindows": ([self.enum_callback, wintypes.LPARAM], wintypes.BOOL),
            "GetWindowThreadProcessId": ([wintypes.HWND, ctypes.POINTER(wintypes.DWORD)], wintypes.DWORD),
            "IsWindowVisible": ([wintypes.HWND], wintypes.BOOL),
            "IsIconic": ([wintypes.HWND], wintypes.BOOL),
            "GetWindowRect": ([wintypes.HWND, ctypes.POINTER(wintypes.RECT)], wintypes.BOOL),
            "ClientToScreen": ([wintypes.HWND, ctypes.POINTER(wintypes.POINT)], wintypes.BOOL),
            "WindowFromPoint": ([wintypes.POINT], wintypes.HWND),
            "GetAncestor": ([wintypes.HWND, wintypes.UINT], wintypes.HWND),
            "GetForegroundWindow": ([], wintypes.HWND),
            "GetSystemMetrics": ([ctypes.c_int], ctypes.c_int),
            "CreateWindowExW": ([wintypes.DWORD, wintypes.LPCWSTR, wintypes.LPCWSTR,
                                 wintypes.DWORD, ctypes.c_int, ctypes.c_int, ctypes.c_int,
                                 ctypes.c_int, wintypes.HWND, wintypes.HMENU,
                                 wintypes.HINSTANCE, wintypes.LPVOID], wintypes.HWND),
            "ShowWindow": ([wintypes.HWND, ctypes.c_int], wintypes.BOOL),
            "DestroyWindow": ([wintypes.HWND], wintypes.BOOL),
            "PeekMessageW": ([ctypes.POINTER(wintypes.MSG), wintypes.HWND,
                              wintypes.UINT, wintypes.UINT, wintypes.UINT], wintypes.BOOL),
            "TranslateMessage": ([ctypes.POINTER(wintypes.MSG)], wintypes.BOOL),
            "DispatchMessageW": ([ctypes.POINTER(wintypes.MSG)], wintypes.LPARAM),
            "SetProcessDPIAware": ([], wintypes.BOOL),
            "DefWindowProcW": ([wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM], wintypes.LPARAM),
            "UnregisterClassW": ([wintypes.LPCWSTR, wintypes.HINSTANCE], wintypes.BOOL),
        }
        for name, (arguments, result) in definitions.items():
            function = getattr(self.user, name)
            function.argtypes, function.restype = arguments, result
        self.kernel.GetModuleHandleW.argtypes = [wintypes.LPCWSTR]
        self.kernel.GetModuleHandleW.restype = wintypes.HMODULE
        self.user.SetProcessDPIAware()

        class Mouse(ctypes.Structure):
            _fields_ = [("dx", wintypes.LONG), ("dy", wintypes.LONG),
                        ("data", wintypes.DWORD), ("flags", wintypes.DWORD),
                        ("time", wintypes.DWORD), ("extra", ctypes.c_size_t)]

        class Keyboard(ctypes.Structure):
            _fields_ = [("vk", wintypes.WORD), ("scan", wintypes.WORD),
                        ("flags", wintypes.DWORD), ("time", wintypes.DWORD),
                        ("extra", ctypes.c_size_t)]

        class Hardware(ctypes.Structure):
            _fields_ = [("message", wintypes.DWORD), ("low", wintypes.WORD),
                        ("high", wintypes.WORD)]

        class Payload(ctypes.Union):
            _fields_ = [("mouse", Mouse), ("keyboard", Keyboard), ("hardware", Hardware)]

        class Input(ctypes.Structure):
            _fields_ = [("type", wintypes.DWORD), ("payload", Payload)]

        self.Mouse, self.Keyboard, self.Payload, self.Input = Mouse, Keyboard, Payload, Input
        self.user.SendInput.argtypes = [wintypes.UINT, ctypes.POINTER(Input), ctypes.c_int]
        self.user.SendInput.restype = wintypes.UINT

    def pump(self):
        message = wintypes.MSG()
        while self.user.PeekMessageW(ctypes.byref(message), None, 0, 0, 1):
            self.user.TranslateMessage(ctypes.byref(message))
            self.user.DispatchMessageW(ctypes.byref(message))

    def rectangle(self, window):
        rectangle = wintypes.RECT()
        if not self.user.GetWindowRect(window, ctypes.byref(rectangle)):
            raise ctypes.WinError(ctypes.get_last_error())
        return rectangle

    def window_for_pid(self, pid):
        matches = []

        @self.enum_callback
        def visit(window, unused):
            owner = wintypes.DWORD()
            self.user.GetWindowThreadProcessId(window, ctypes.byref(owner))
            if owner.value == pid and self.user.IsWindowVisible(window) and not self.user.IsIconic(window):
                matches.append(window)
            return True

        if not self.user.EnumWindows(visit, 0):
            raise ctypes.WinError(ctypes.get_last_error())
        if len(matches) > 1:
            raise RuntimeError(f"Expected one visible engine window, found {len(matches)}")
        return matches[0] if matches else None

    def create_helper(self):
        x, y, width, height = (self.user.GetSystemMetrics(index) for index in (76, 77, 78, 79))
        if width < 640 or height < 480:
            raise RuntimeError("Native focus probe needs a desktop of at least 640x480")
        class WindowClass(ctypes.Structure):
            _fields_ = [("style", wintypes.UINT), ("procedure", ctypes.c_void_p),
                        ("class_extra", ctypes.c_int), ("window_extra", ctypes.c_int),
                        ("instance", wintypes.HINSTANCE), ("icon", wintypes.HANDLE),
                        ("cursor", wintypes.HANDLE), ("background", wintypes.HBRUSH),
                        ("menu", wintypes.LPCWSTR), ("name", wintypes.LPCWSTR)]

        self.user.RegisterClassW.argtypes = [ctypes.POINTER(WindowClass)]
        self.user.RegisterClassW.restype = wintypes.WORD
        self.instance = self.kernel.GetModuleHandleW(None)
        self.class_name = f"Krkr2NativeFocusProbe{os.getpid()}"
        window_class = WindowClass(
            0, ctypes.cast(self.user.DefWindowProcW, ctypes.c_void_p).value,
            0, 0, self.instance, None, None, 6, None, self.class_name)
        if not self.user.RegisterClassW(ctypes.byref(window_class)):
            self.class_name = None
            raise ctypes.WinError(ctypes.get_last_error())
        # A regular top-level class uses the native default window procedure;
        # built-in STATIC controls do not provide normal activation semantics.
        # TOPMOST keeps its caption clickable even on small desktops where the
        # foreground engine would otherwise obscure the helper.
        self.helper = self.user.CreateWindowExW(
            8, self.class_name, "KrKr2 focus probe helper", 0x00CF0000,
            x + width - 380, y + height - 230, 360, 200,
            None, None, self.instance, None)
        if not self.helper:
            raise ctypes.WinError(ctypes.get_last_error())
        self.user.ShowWindow(self.helper, 4)  # SW_SHOWNOACTIVATE
        self.pump()
        return self.helper

    def send(self, events):
        array = (self.Input * len(events))(*events)
        sent = self.user.SendInput(len(events), array, ctypes.sizeof(self.Input))
        if sent != len(events):
            raise RuntimeError(f"SendInput sent {sent}/{len(events)}, error {ctypes.get_last_error()}")

    def key(self, key, down):
        self.send([self.Input(1, self.Payload(keyboard=self.Keyboard(key, 0, 0 if down else 2, 0, 0)))])
        if down:
            self.keys.add(key)
        else:
            self.keys.discard(key)

    def click_caption(self, window):
        rectangle = self.rectangle(window)
        origin = wintypes.POINT(0, 0)
        if not self.user.ClientToScreen(window, ctypes.byref(origin)):
            raise ctypes.WinError(ctypes.get_last_error())
        x = (rectangle.left + rectangle.right) // 2
        caption_top = rectangle.top + max(1, origin.x - rectangle.left)
        if origin.y - caption_top < 4:
            raise RuntimeError("Native focus target has no usable caption")
        y = (caption_top + origin.y) // 2
        origin_x, origin_y, width, height = (self.user.GetSystemMetrics(index) for index in (76, 77, 78, 79))
        if not (origin_x <= x < origin_x + width and origin_y <= y < origin_y + height):
            raise RuntimeError("Window caption lies outside the virtual desktop")
        hit = self.user.WindowFromPoint(wintypes.POINT(x, y))
        root = self.user.GetAncestor(hit, 2) if hit else None  # GA_ROOT
        self.last_caption = {"target_hwnd": window, "hit_root": root, "point": [x, y],
                             "rectangle": [rectangle.left, rectangle.top, rectangle.right, rectangle.bottom],
                             "client_origin": [origin.x, origin.y],
                             "desktop": [origin_x, origin_y, width, height],
                             "foreground_before": self.foreground()}
        if root != window:
            raise RuntimeError(f"Native caption obscured: target {window}, hit {root}")
        # MOVE | ABSOLUTE | VIRTUALDESK; caption clicks avoid game callbacks.
        move = self.Mouse(round((x - origin_x) * 65535 / (width - 1)),
                          round((y - origin_y) * 65535 / (height - 1)), 0, 0xC001, 0, 0)
        up = self.Input(0, self.Payload(mouse=self.Mouse(0, 0, 0, 4, 0, 0)))
        try:
            self.send([self.Input(0, self.Payload(mouse=move)),
                       self.Input(0, self.Payload(mouse=self.Mouse(0, 0, 0, 2, 0, 0))), up])
        except RuntimeError:
            # A partial insertion may have sent down without up.
            self.send([up])
            raise
        return self.last_caption

    def foreground(self):
        return self.user.GetForegroundWindow()

    def close(self):
        for key in list(self.keys):
            self.key(key, False)
        if self.helper:
            self.user.DestroyWindow(self.helper)
            self.helper = None
        if self.class_name:
            self.user.UnregisterClassW(self.class_name, self.instance)
            self.class_name = None
