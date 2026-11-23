"""Exercise SDL3 virtual controllers through the native game host."""

import argparse
from itertools import count
import json
import os
from pathlib import Path
import shutil
import sys

from interaction_smoke import read_events
from sdl_desktop_repro import (
    ESCAPE, FIXTURES, GREEN, X11, activate, capture, client_window, desktop,
    digest, event, geometry, launch, snapshot, tap, wait_for,
)


SOUTH, EAST, WEST, DPAD_LEFT, DPAD_RIGHT = 0, 1, 2, 13, 14
LEFT_X, LEFT_TRIGGER = 0, 4
F2, F3, F8 = 0xffbf, 0xffc0, 0xffc5
PAD1, PAD2, PAD4, PAD5, PADLEFT = 448, 449, 451, 452, 437


def drive_gamepads(x11, game, output, process, window, env):
    sequence = count(1)
    request = game / "sdl-pad-input.txt"
    pending = request.with_suffix(".next")
    ack = game / "sdl-pad-ack.txt"

    def send(action, *args):
        number = str(next(sequence))
        pending.write_text(" ".join((number, action, *map(str, args))) + "\n", encoding="ascii")
        pending.replace(request)
        wait_for(f"SDL virtual gamepad request {number}",
                 lambda: ack.exists() and ack.read_text(encoding="ascii").strip() == number,
                 process)

    def probe(held):
        marker = f"pad-state:{int(held)}"
        previous = read_events(game).count(marker)
        tap(x11, F8)
        event(game, process, marker, previous + 1)

    send("attach", 0)
    send("attach", 1)
    send("button", 0, SOUTH, 1)
    event(game, process, f"down:{PAD1}:1:1")
    wait_for("gamepad-triggered repaint",
             lambda: capture(x11, window, geometry(x11, window)).getpixel((80, 60)) == GREEN,
             process)
    snapshot(output, "gamepad-repaint", x11, window)
    send("button", 1, SOUTH, 1)
    send("detach", 0)
    probe(True)
    send("button", 1, SOUTH, 0)
    event(game, process, f"up:{PAD1}:0:0")

    send("button", 1, DPAD_LEFT, 1)
    event(game, process, f"down:{PADLEFT}:1:1")
    send("axis", 1, LEFT_X, -20000)
    send("button", 1, DPAD_LEFT, 0)
    send("axis", 1, LEFT_X, -14000)
    probe(True)
    send("axis", 1, LEFT_X, -11000)
    event(game, process, f"up:{PADLEFT}:0:0")
    # SDL's virtual joystick uses the full signed range for physical triggers.
    send("axis", 1, LEFT_TRIGGER, 24000)
    event(game, process, f"down:{PAD5}:1:1")
    send("axis", 1, LEFT_TRIGGER, -32768)
    event(game, process, f"up:{PAD5}:0:0")

    with launch([sys.executable, str(FIXTURES / "drop_source.py")],
                output / "focus-peer.log", env=env) as peer:
        other = wait_for("native focus peer", lambda: client_window(peer.pid), peer)
        activate(other)
        event(game, process, "inactive")
        before = read_events(game)
        activate(window)
        event(game, process, "active", before.count("active") + 1)
        send("button", 1, EAST, 1)
        event(game, process, f"down:{PAD2}:1:1")
        before = read_events(game)
        activate(other)
        event(game, process, "inactive", before.count("inactive") + 1)
        event(game, process, f"up:{PAD2}:0:0")
        send("button", 1, WEST, 1)
        activate(window)
        probe(False)
        assert f"down:{PAD4}:1:1" not in read_events(game)
        send("button", 1, EAST, 0)
        send("button", 1, WEST, 0)
        send("button", 1, WEST, 1)
        event(game, process, f"down:{PAD4}:1:1")
        send("button", 1, WEST, 0)
        event(game, process, f"up:{PAD4}:0:0")

    tap(x11, F2)
    event(game, process, "mouse-keys:1")
    x11.motion(window, 70, 95)
    event(game, process, "move:70:95")
    before = len(read_events(game))
    send("button", 1, DPAD_RIGHT, 1)

    def moved_right():
        moves = [line for line in read_events(game)[before:] if line.startswith("move:")]
        return any(int(line.split(":")[1]) >= 80 and line.endswith(":95") for line in moves)

    wait_for("gamepad-driven pointer movement", moved_right, process)
    send("button", 1, DPAD_RIGHT, 0)
    probe(False)
    x11.motion(window, 100, 95)
    event(game, process, "move:100:95")
    send("button", 1, SOUTH, 1)
    event(game, process, "pointer-down:0:100:95")
    send("button", 1, SOUTH, 0)
    event(game, process, "pointer-up:0:100:95")
    event(game, process, "click:100:95")

    send("button", 1, SOUTH, 1)
    event(game, process, "pointer-down:0:100:95", 2)
    send("detach", 1)
    event(game, process, "pointer-up:0:100:95", 2)
    probe(False)
    tap(x11, F3)
    event(game, process, "mouse-keys:0")

    events = read_events(game)
    assert events.count("click:100:95") == 1, events
    keys = [line for line in events if line.startswith(("down:", "up:"))]
    assert keys == [f"{edge}:{key}:{state}:{state}"
                    for key in (PAD1, PADLEFT, PAD5, PAD2, PAD4)
                    for edge, state in (("down", 1), ("up", 0))], keys


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
    fixture = FIXTURES / "gamepad.tjs"
    shutil.copy2(fixture, game / "startup.tjs")
    (game / "interaction-fixture.json").write_text('{"fixture":"sdl3-gamepad"}\n', encoding="utf-8")
    env = dict(os.environ, XDG_RUNTIME_DIR=str(runtime), XDG_CONFIG_HOME=str(output / "config"),
               XDG_CACHE_HOME=str(output / "cache"), LC_ALL="C.UTF-8", SDL_IM_MODULE="none",
               ALSOFT_DRIVERS="null")
    command = [str(engine), "--host=sdl3", "--game", str(game), "--", "-sdl-gamepad-contract=yes"]
    report = {"command": command, "engine_sha256": digest(engine), "fixture_sha256": digest(fixture),
              "gamepad_input": "SDL virtual joystick", "window_driver": "x11"}
    x11 = X11()
    try:
        with desktop("x11", x11, output, env) as (_, engine_env):
            with launch(command, output / "process.log", env=engine_env) as process:
                event(game, process, "ready")
                window = wait_for("SDL3 gamepad window", lambda: client_window(process.pid), process)
                activate(window)
                drive_gamepads(x11, game, output, process, window, engine_env)
                tap(x11, ESCAPE)
                assert process.wait(timeout=8) == 0
        report.update(passed=True, forced_stop=False, exit_code=0)
    except Exception as error:
        report.update(passed=False, error=str(error))
        raise
    finally:
        x11.close()
        report["events"] = read_events(game)
        (output / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
