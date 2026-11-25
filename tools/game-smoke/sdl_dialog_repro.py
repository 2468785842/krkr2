"""Exercise native menu/input/message/file UI with X11/XTest, never widget dispatch."""
import argparse
import ctypes as C
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

from interaction_smoke import read_events
from sdl_desktop_repro import X11, capture, geometry, origin


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(engine, output):
    output.mkdir(parents=True)
    game = output / "game"
    game.mkdir()
    fixture = Path(__file__).with_name("native-dialogs") / "startup.tjs"
    shutil.copy2(fixture, game / "startup.tjs")
    (game / "fixture.txt").write_text("native file selector\n", encoding="utf-8")
    original = bytes(range(256)) * 8 + b"\0native\xff"
    (game / "source.bin").write_bytes(original)
    runtime = output / "runtime"
    runtime.mkdir(mode=0o700)
    env = dict(os.environ, SDL_VIDEO_DRIVER="x11", SDL_IM_MODULE="none", GTK_IM_MODULE="simple",
               LC_ALL="C.UTF-8", XDG_RUNTIME_DIR=str(runtime), XDG_CONFIG_HOME=str(output / "config"),
               GSETTINGS_BACKEND="memory", ALSOFT_DRIVERS="null")
    command = [str(engine), "--host=sdl3", "--game", str(game)]
    report = {"command": command, "engine_sha256": digest(engine), "fixture_sha256": digest(fixture),
              "forced_stop": False}
    x11 = X11()
    x11.library.XFetchName.argtypes = [C.c_void_p, C.c_ulong, C.POINTER(C.c_void_p)]
    x11.library.XFetchName.restype = C.c_int
    process = None
    try:
        with (output / "process.log").open("wb") as log:
            process = subprocess.Popen(command, cwd=engine.parent, env=env, stdout=log, stderr=subprocess.STDOUT)

            def wait(label, condition):
                deadline = time.monotonic() + 15
                while time.monotonic() < deadline:
                    result = condition()
                    if result:
                        return result
                    if process.poll() is not None:
                        raise RuntimeError(f"Engine exited {process.returncode} before {label}")
                    time.sleep(0.03)
                raise TimeoutError(label)

            def event(value):
                wait(value, lambda: value in read_events(game))

            def titled(title):
                excluded = []
                while window := x11.process_window(process.pid, excluded):
                    excluded.append(window)
                    name = C.c_void_p()
                    with x11.window_query(window):
                        if x11.library.XFetchName(x11.display, window, C.byref(name)) and name:
                            try:
                                if C.string_at(name).decode("utf-8", errors="replace") == title:
                                    return window
                            finally:
                                x11.library.XFree(name)
                return None

            def request(value):
                temporary = game / "request.next"
                temporary.write_text(value + "\n", encoding="utf-8")
                temporary.replace(game / "request.txt")
                event("command:" + value)

            def dialog(title):
                window = wait("mapped " + title, lambda: titled(title))
                x11.focus(window)
                return window

            def tap(key):
                x11.key(True, key)
                x11.key(False, key)

            def chord(modifiers, key):
                for modifier in modifiers:
                    x11.key(True, modifier)
                tap(key)
                for modifier in reversed(modifiers):
                    x11.key(False, modifier)

            def text(value):
                for char in value:
                    if ord(char) > 127:
                        chord([0xffe3, 0xffe1], ord("u"))
                        for digit in f"{ord(char):x}":
                            tap(ord(digit))
                        tap(0xff0d)
                    else:
                        shift = char.isupper() or char in '~!@#$%^&*()_+{}|:"<>?'
                        if shift:
                            x11.key(True, 0xffe1)
                        tap(ord(char))
                        if shift:
                            x11.key(False, 0xffe1)

            def click(window, x, y):
                x11.motion(window, x, y)
                x11.button(True)
                x11.button(False)

            def shot(window, name):
                # Capture the stable root surface: GTK can resize a newly mapped
                # chooser between querying its size and XGetImage.
                x, y = origin(x11, window)
                width, height = geometry(x11, window)
                capture(x11, x11.root, geometry(x11, x11.root)).crop(
                    (x, y, x + width, y + height)).save(output / f"{name}.png")

            event("ready")
            event("copy:1")
            event("copy-self:0")
            assert (game / "复制.bin").read_bytes() == original
            assert (game / "source.bin").read_bytes() == original
            main = dialog("Native host contract")
            for index, expected in ((1, "menu:toggle:1"), (2, "menu:toggle:0")):
                request(f"menu-{index}")
                menu = dialog("Native menu")
                shot(menu, f"menu-{index}")
                # First row of the native menu list, inset by the dialog border.
                click(menu, 100, 28)
                event(expected)
                wait("menu dismissed", lambda: not titled("Native menu"))
            request("menu-nested")
            menu = dialog("Native menu")
            click(menu, 100, 66)
            nested = dialog("Nested menu")
            shot(nested, "menu-nested")
            click(nested, 100, 28)
            event("menu:nested:1")
            request("menu-cancel")
            menu = dialog("Native menu")
            tap(0xff1b)
            wait("menu cancelled", lambda: not titled("Native menu"))
            request("input-accept")
            entry = dialog("Native input")
            chord([0xffe3], ord("a"))
            value = "native 中文 𠀀"
            text(value)
            shot(entry, "input")
            tap(0xff0d)
            event("input:" + value)
            request("input-cancel")
            dialog("Native input")
            tap(0xff1b)
            event("input:cancel")
            request("message")
            message = dialog("Native message")
            shot(message, "message")
            width, height = geometry(x11, message)
            click(message, width - 45, height - 22)
            event("message:2")
            for action in ("file-cancel", "file-accept"):
                request(action)
                chooser = dialog("Native file")
                shot(chooser, action)
                if action.endswith("cancel"):
                    tap(0xff1b)
                    event("file-cancel:0")
                else:
                    chord([0xffe3], ord("l"))
                    text(str(game / "fixture.txt"))
                    tap(0xff0d)
                    event("file-accept:1")
                    assert any(line.startswith("selected:") and line.endswith("/fixture.txt") for line in read_events(game))
            assert not any(line.startswith(("error:", "menu:disabled:", "menu:hidden:")) for line in read_events(game))
            request("quit")
            report["exit_code"] = process.wait(timeout=10)
            assert report["exit_code"] == 0
            report["passed"] = True
    except Exception as error:
        report.update(passed=False, error=str(error))
        raise
    finally:
        if process is not None and process.poll() is None:
            process.kill()
            process.wait()
            report["forced_stop"] = True
        x11.close()
        report["events"] = read_events(game)
        (output / "report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    run(args.engine.resolve(strict=True), args.output.resolve())
