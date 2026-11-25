"""Validate the SDL-only Windows runtime through native windows and SendInput.

Uses a new self-authored fixture; never enables the engine's component driver.
Requires an interactive Windows desktop and Pillow for actual window screenshots.
"""
import argparse
import ctypes
from ctypes import wintypes
import json
from pathlib import Path
import sys
import time

import interaction_smoke
from win32_input import Desktop


class NativeProbe:
    def __call__(self, process, directory, deadline, report):
        from PIL import ImageGrab

        desktop = Desktop()
        user = desktop.user
        user.SendMessageTimeoutW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM,
            wintypes.LPARAM, wintypes.UINT, wintypes.UINT, ctypes.POINTER(ctypes.c_size_t)]
        user.SendMessageTimeoutW.restype = wintypes.LPARAM
        user.EnumChildWindows.argtypes = [wintypes.HWND, desktop.enum_callback, wintypes.LPARAM]
        user.EnumChildWindows.restype = wintypes.BOOL
        user.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
        user.SetForegroundWindow.argtypes = [wintypes.HWND]
        user.SetForegroundWindow.restype = wintypes.BOOL
        user.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
        report['native_input'] = 'Win32 SendInput'
        report['screenshots'] = []
        report['foreground_steps'] = []

        def wait(condition, label, seconds=15):
            end = min(deadline, time.monotonic() + seconds)
            while time.monotonic() < end:
                desktop.pump()
                if process.poll() is not None:
                    raise RuntimeError(f'Engine exited {process.returncode} before {label}')
                value = condition()
                if value:
                    return value
                time.sleep(0.05)
            raise TimeoutError(label)

        def event(text, count=1):
            wait(lambda: interaction_smoke.read_events(directory).count(text) >= count, text)

        def window(title=None, class_name=None):
            matches = []
            @desktop.enum_callback
            def visit(handle, unused):
                pid = wintypes.DWORD()
                user.GetWindowThreadProcessId(handle, ctypes.byref(pid))
                if pid.value != process.pid or not user.IsWindowVisible(handle):
                    return True
                text, kind = ctypes.create_unicode_buffer(1024), ctypes.create_unicode_buffer(128)
                user.GetWindowTextW(handle, text, len(text))
                user.GetClassNameW(handle, kind, len(kind))
                if (title is None or text.value == title) and (class_name is None or kind.value == class_name):
                    matches.append(handle)
                return True
            if not user.EnumWindows(visit, 0):
                raise ctypes.WinError(ctypes.get_last_error())
            return matches[0] if len(matches) == 1 else None

        def dialog_ready(dialog):
            edits = []
            controls = []
            @desktop.enum_callback
            def visit(handle, unused):
                kind = ctypes.create_unicode_buffer(128)
                text = ctypes.create_unicode_buffer(1024)
                user.GetClassNameW(handle, kind, len(kind))
                user.GetWindowTextW(handle, text, len(text))
                if kind.value == 'Edit':
                    result = ctypes.c_size_t()
                    user.SendMessageTimeoutW(handle, 0x0d, len(text),
                        ctypes.addressof(text), 2, 1000, ctypes.byref(result))
                controls.append({'class':kind.value,'text':text.value})
                # Windows can hide known extensions in the native filename
                # control. The returned storage path is still checked against
                # the full fixture.txt path by interaction_smoke.
                if kind.value == 'Edit' and text.value in {'fixture.txt', 'fixture'}:
                    edits.append(handle)
                return True
            user.EnumChildWindows(dialog, visit, 0)
            report['dialog_controls'] = controls
            return bool(edits)

        def tap(key):
            desktop.key(key, True)
            desktop.key(key, False)

        def foreground(handle):
            desktop.click_caption(handle)
            report['foreground_steps'].append(dict(desktop.last_caption))
            wait(lambda: desktop.foreground() == handle, f'foreground ownership {handle}')

        def request(text):
            temporary = directory / 'native-command.tmp'
            temporary.write_text(text + '\n', encoding='utf-8')
            temporary.replace(directory / 'interaction-command.txt')
            event('command:' + text)

        def capture(handle, name):
            rect = desktop.rectangle(handle)
            path = directory / name
            ImageGrab.grab(bbox=(rect.left, rect.top, rect.right, rect.bottom), all_screens=True).save(path)
            report['screenshots'].append(str(path))

        try:
            event('ready')
            main = wait(lambda: window(title='KrKr2 window contract'), 'game window')
            foreground(main)
            desktop.request_installed_us_layout(main)
            request('keyboard-probe')
            event('keyboard-probe-ready')
            desktop.key(65, True)
            event('input:key-down:65:0:1:0')
            desktop.key(65, False)
            event('input:key-up:65:0:0:0')
            capture(main, 'native-window.png')

            # Native focus changes must release a held key before new input.
            desktop.key(65, True)
            event('input:key-down:65:0:1:0', 2)
            helper = desktop.create_helper()
            if not user.SetForegroundWindow(helper):
                raise RuntimeError("Cannot focus owned native helper")
            wait(lambda: desktop.foreground() == helper, "helper foreground")
            event('input:key-up:65:0:0:0', 2)
            desktop.key(65, False)
            foreground(main)
            desktop.key(65, True)
            event('input:key-down:65:0:1:0', 3)
            desktop.key(65, False)
            event('input:key-up:65:0:0:0', 3)
            desktop.minimize(main)
            wait(lambda: desktop.iconic(main), 'native minimized')
            desktop.restore(main)
            wait(lambda: not desktop.iconic(main), 'native restored')
            foreground(main)
            desktop.key(65, True)
            event('input:key-down:65:0:1:0', 4)
            desktop.key(65, False)
            event('input:key-up:65:0:0:0', 4)
            capture(main, 'native-restored.png')

            for command, result in [('menu-open', 'menu-click:1'), ('menu-open-again', 'menu-click:0')]:
                request(command)
                wait(lambda: window(class_name='#32768'), 'native popup menu')
                tap(0x28)
                tap(0x0d)
                event(result)
            for command, title, key, result in [
                ('inform', 'KrKr2 contract message', 0x0d, 'inform-return:0'),
                ('select-cancel', 'KrKr2 contract file', 0x1b, 'select-cancel-return:0'),
                ('select-accept', 'KrKr2 contract file', 0x0d, 'select-accept-return:1'),
            ]:
                request(command)
                dialog = wait(lambda: window(title=title, class_name='#32770'), title)
                wait(lambda: desktop.foreground() == dialog, 'native dialog foreground')
                if command.startswith('select-'):
                    try:
                        wait(lambda: dialog_ready(dialog), 'file dialog controls ready')
                    except TimeoutError:
                        capture(dialog, 'dialog-waiting.png')
                        raise
                # Window mapping precedes the native opening animation.
                until = time.monotonic() + 0.4
                while time.monotonic() < until:
                    desktop.pump()
                    time.sleep(0.02)
                capture(dialog, command + '.png')
                tap(key)
                event(result)
            request('veto')
            event('veto-preserved')
            request('accept')
            event('close-returned')
        finally:
            report['last_foreground'] = desktop.foreground()
            desktop.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('engine', type=Path)
    parser.add_argument('directory', type=Path)
    args = parser.parse_args()
    if sys.platform != 'win32':
        parser.error('Windows interactive desktop required')
    interaction_smoke.prepare(args.directory)
    return interaction_smoke.run(args.engine, args.directory, 150, automation=NativeProbe())


if __name__ == '__main__':
    raise SystemExit(main())
