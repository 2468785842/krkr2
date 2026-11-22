"""Run the real Linux krkr2 entry point and observe its SDL3 X11 window."""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

from interaction_smoke import read_events

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "sdl3-prototype"))
from verify_x11 import X11


def pixel(x11, window, x, y):
    library = x11.library
    ulong, integer, pointer = ctypes.c_ulong, ctypes.c_int, ctypes.c_void_p
    library.XGetImage.argtypes = [
        pointer, ulong, integer, integer, ctypes.c_uint, ctypes.c_uint, ulong, integer,
    ]
    library.XGetImage.restype = pointer
    library.XGetPixel.argtypes = [pointer, integer, integer]
    library.XGetPixel.restype = ulong
    library.XDestroyImage.argtypes = [pointer]
    library.XDestroyImage.restype = integer
    image = library.XGetImage(x11.display, window, x, y, 1, 1, ulong(-1).value, 2)
    if not image:
        raise RuntimeError("Cannot read X11 window pixel")
    try:
        return library.XGetPixel(image, 0, 0) & 0xffffff
    finally:
        library.XDestroyImage(image)


def run(engine, output):
    output.mkdir(parents=True)
    game = output / "game"
    game.mkdir()
    fixture = game / "startup.tjs"
    shutil.copy2(Path(__file__).with_name("sdl-host") / "startup.tjs", fixture)
    command = [str(engine), "--host=sdl3", "--game", str(game)]
    report = {
        "command": command,
        "engine_sha256": hashlib.sha256(engine.read_bytes()).hexdigest(),
        "fixture_sha256": hashlib.sha256(fixture.read_bytes()).hexdigest(),
        "pixel_checks": {},
        "forced_stop": False,
    }
    x11 = X11()
    try:
        with (output / "process.log").open("wb") as log:
            env = dict(os.environ, SDL_VIDEO_DRIVER="x11", ALSOFT_DRIVERS="null")
            process = subprocess.Popen(command, cwd=engine.parent, env=env,
                                       stdout=log, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + 30

            def wait(condition, label):
                while time.monotonic() < deadline:
                    if process.poll() is not None:
                        raise RuntimeError(f"Engine exited {process.returncode} before {label}")
                    result = condition()
                    if result:
                        return result
                    time.sleep(0.05)
                raise TimeoutError(label)

            def event(name):
                wait(lambda: name in read_events(game), name)

            def frame(name, expected):
                def matches():
                    report["pixel_checks"][name] = [
                        f"{pixel(x11, window, x, y):06x}" for x, y in ((80, 60), (240, 60), (80, 180))
                    ]
                    return report["pixel_checks"][name] == expected
                wait(matches, f"frame {name}: {expected}")

            try:
                event("ready")
                window = wait(lambda: x11.process_window(process.pid), "mapped SDL3 window")
                x11.focus(window)
                frame("startup", ["204060", "204060", "204060"])
                x11.request_close(window)
                event("close:1")
                # New input and changed pixels after the veto prove that the
                # application kept its window and was still pumping events.
                x11.key(True)
                event("key-down:65:1")
                frame("after-veto-key", ["d03020", "204060", "204060"])
                x11.key(False)
                event("key-up:65:0")
                x11.motion(window, 240, 60)
                x11.button(True)
                event("mouse-down:240:60:1")
                frame("pointer", ["d03020", "30c050", "204060"])
                x11.button(False)
                event("mouse-up:240:60:0")
                x11.request_close(window)
                if process.wait(timeout=max(1, deadline - time.monotonic())) != 0:
                    raise RuntimeError(f"Engine exited with {process.returncode}")
                if [line for line in read_events(game) if line.startswith("close:")] != ["close:1", "close:2"]:
                    raise RuntimeError("Expected exactly two close queries")
            finally:
                if process.poll() is None:
                    report["forced_stop"] = True
                    process.kill()
                    process.wait()
                report["exit_code"] = process.returncode
    finally:
        x11.close()
        report["events"] = read_events(game)
        (output / "report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
        print(json.dumps(report, indent=2), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    run(args.engine.resolve(strict=True), args.output.resolve())
