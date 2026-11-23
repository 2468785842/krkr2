"""Exercise native Xdnd, native mouse keys and SDL touch event delivery."""

import argparse
import json
import os
from pathlib import Path
import shutil
import sys
import time

from interaction_smoke import read_events
from sdl_desktop_repro import (
    ESCAPE, FIXTURES, X11, activate, client_window, command, desktop, digest,
    event, launch, snapshot, tap, wait_for,
)


F2, F3, F4, F5, RIGHT, RETURN = 0xffbf, 0xffc0, 0xffc1, 0xffc2, 0xff53, 0xff0d


def drag_files(x11, output, game, window, process, env):
    names = ("harbor map.txt", "灯塔.txt")
    files = [output / name for name in names]
    for path in files:
        path.write_text("SDL3 file drag fixture\n", encoding="utf-8")
    script = FIXTURES / "drop_source.py"
    with launch([sys.executable, str(script), *(str(path) for path in files)],
                output / "drop-source.log", env=env) as source:
        source_window = wait_for("GTK drag source", lambda: client_window(source.pid), source)
        command("wmctrl", "-ir", hex(source_window), "-e", "0,48,96,-1,-1")
        activate(source_window)
        x11.motion(source_window, 55, 40)
        x11.button(True)
        try:
            x11.motion(source_window, 110, 40)
            time.sleep(0.15)
            x11.motion(window, 130, 100)
            time.sleep(0.2)
        finally:
            x11.button(False)
        event(game, process, "drop:2")
        delivered = [item for item in read_events(game) if item.startswith("file:")]
        assert len(delivered) == 2 and all(
            item.endswith("/" + name + ":1") for item, name in zip(delivered, names)
        ), delivered
        exercise_mouse_keys(x11, game, window, source_window, process)
    return delivered


def exercise_mouse_keys(x11, game, window, peer, process):
    activate(window)
    tap(x11, F2)
    event(game, process, "mouse-keys:1")
    x11.motion(window, 40, 100)
    event(game, process, "move:40:100")

    def position_advanced():
        moves = [line for line in read_events(game) if line.startswith("move:")]
        x, y = map(int, moves[-1].split(":")[1:])
        return (x, y) if x >= 50 and y == 100 else None

    x11.key(True, RIGHT)
    wait_for("keyboard-driven pointer movement", position_advanced, process)
    x11.key(False, RIGHT)
    time.sleep(0.12)
    x, y = position_advanced()
    for keysym, button in ((RETURN, 0), (ESCAPE, 1)):
        x11.key(True, keysym)
        event(game, process, f"down:{button}:{x}:{y}")
        x11.key(False, keysym)
        event(game, process, f"up:{button}:{x}:{y}")
    event(game, process, f"click:{x}:{y}")

    before = read_events(game)
    x11.key(True, RETURN)
    event(game, process, f"down:0:{x}:{y}", before.count(f"down:0:{x}:{y}") + 1)
    activate(peer)
    event(game, process, "inactive", before.count("inactive") + 1)
    event(game, process, f"up:0:{x}:{y}", before.count(f"up:0:{x}:{y}") + 1)
    x11.key(False, RETURN)
    activate(window)
    tap(x11, F3)
    event(game, process, "mouse-keys:0")
    assert read_events(game).count(f"click:{x}:{y}") == before.count(f"click:{x}:{y}")


def exercise_touch(x11, game, process):
    tap(x11, F4)
    event(game, process, "touch:1")
    event(game, process, "resize:640:360")
    before = read_events(game)
    request = game / "sdl-touch-input.next"
    request.write_text(
        "down 7 99 0.05 0.25\n"  # The left black bar must reject a new finger.
        "down 7 4294967300 0.3125 0.25\n"
        "down 7 4294967301 0.6875 0.5\n"
        "move 7 4294967300 0.40625 0.375\n"
        "move 7 4294967301 0.75 0.625\n"
        "cancel 7 4294967300 0.40625 0.375\n"
        "up 7 4294967300 0.40625 0.375\n"
        "up 7 4294967301 0.75 0.625\n"
        "down 7 4294967302 0.3125 0.25\n",
        encoding="ascii",
    )
    request.replace(game / "sdl-touch-input.txt")
    event(game, process, "finger-down:3:80:60:1:3:80:60")
    tap(x11, F5)
    event(game, process, "touch:0:1")
    event(game, process, "finger-up:3")
    touch_events = read_events(game)[len(before):]
    fingers = [line for line in touch_events if line.startswith("finger-")]
    assert fingers == [
        "finger-down:1:80:60:1:1:80:60", "finger-down:2:240:120:2:2:240:120",
        "finger-move:1:120:90", "finger-move:2:266:150", "finger-up:1",
        "finger-up:2", "finger-down:3:80:60:1:3:80:60", "finger-up:3",
    ], fingers
    assert all(line in touch_events for line in ("scaling:1", "rotation:1", "multi:2")), touch_events
    assert not any(line.startswith(("down:", "up:", "click:")) for line in touch_events)
    return touch_events


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    engine = args.engine.resolve(strict=True)
    output = args.output.resolve()
    game = output / "game"
    game.mkdir(parents=True)
    runtime = output / "runtime"
    runtime.mkdir(mode=0o700)
    fixture = FIXTURES / "input.tjs"
    shutil.copy2(fixture, game / "startup.tjs")
    (game / "interaction-fixture.json").write_text('{"fixture":"sdl3-game-input"}\n', encoding="utf-8")
    env = dict(os.environ, XDG_RUNTIME_DIR=str(runtime), XDG_CONFIG_HOME=str(output / "config"),
               XDG_CACHE_HOME=str(output / "cache"), LC_ALL="C.UTF-8", SDL_IM_MODULE="none",
               ALSOFT_DRIVERS="null")
    command_line = [str(engine), "--host=sdl3", "--game", str(game), "--", "-sdl-touch-contract=yes"]
    report = {"command": command_line, "engine_sha256": digest(engine), "fixture_sha256": digest(fixture),
              "touch_input": "SDL_PushEvent"}
    x11 = X11()
    try:
        with desktop("x11", x11, output, env) as (_, engine_env):
            with launch(command_line, output / "process.log", env=engine_env) as process:
                event(game, process, "ready")
                window = wait_for("SDL3 input window", lambda: client_window(process.pid), process)
                report["dropped_files"] = drag_files(x11, output, game, window, process, engine_env)
                snapshot(output, "game-window", x11, window)
                report["touch_events"] = exercise_touch(x11, game, process)
                tap(x11, ESCAPE)
                assert process.wait(timeout=8) == 0
        report.update(passed=True, forced_stop=False, exit_code=0)
    except Exception as error:
        report.update(passed=False, error=str(error))
        raise
    finally:
        x11.close()
        report["events"] = read_events(game)
        (output / "report.json").write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n",
                                            encoding="utf-8")
    print(json.dumps(report, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
