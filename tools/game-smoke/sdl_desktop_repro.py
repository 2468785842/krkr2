"""Exercise SDL3 through real X11/Wayland desktops and a self-authored KAG game."""

import argparse
from contextlib import contextmanager
import ctypes as C
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import shlex
import subprocess
import sys
import time

from PIL import Image, ImageChops

from interaction_smoke import read_events

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "sdl3-prototype"))
from verify_x11 import X11, WindowAttributes


FIXTURES = Path(__file__).with_name("sdl-desktop")
BLUE = (32, 64, 96)
GREEN = (48, 192, 80)
F6, F11, ESCAPE, ALT, TAB = 0xffc3, 0xffc8, 0xff1b, 0xffe9, 0xff09


class XImage(C.Structure):
    _fields_ = [(name, C.c_int) for name in (
        "width", "height", "xoffset", "format",
    )] + [("data", C.c_void_p)] + [(name, C.c_int) for name in (
        "byte_order", "bitmap_unit", "bitmap_bit_order", "bitmap_pad",
        "depth", "bytes_per_line", "bits_per_pixel",
    )]


def capture(x11, window, size):
    library = x11.library
    library.XGetImage.argtypes = [
        C.c_void_p, C.c_ulong, C.c_int, C.c_int, C.c_uint, C.c_uint, C.c_ulong, C.c_int,
    ]
    library.XGetImage.restype = C.POINTER(XImage)
    library.XDestroyImage.argtypes = [C.POINTER(XImage)]
    pointer = library.XGetImage(x11.display, window, 0, 0, *size, C.c_ulong(-1).value, 2)
    if not pointer:
        raise RuntimeError("Cannot capture the X11 drawable")
    try:
        image = pointer.contents
        assert image.byte_order == 0 and image.bits_per_pixel == 32
        pixels = C.string_at(image.data, image.bytes_per_line * image.height)
        return Image.frombytes("RGB", size, pixels, "raw", "BGRX", image.bytes_per_line)
    finally:
        library.XDestroyImage(pointer)


def geometry(x11, window):
    attrs = WindowAttributes()
    if not x11.library.XGetWindowAttributes(x11.display, window, C.byref(attrs)):
        raise RuntimeError(f"Cannot read X11 geometry for {window:#x}")
    return attrs.width, attrs.height


def origin(x11, window):
    x, y, child = C.c_int(), C.c_int(), C.c_ulong()
    if not x11.library.XTranslateCoordinates(x11.display, window, x11.root, 0, 0,
                                             C.byref(x), C.byref(y), C.byref(child)):
        raise RuntimeError("Cannot locate window on the X11 desktop")
    return x.value, y.value


def content_box(image, color=BLUE):
    differences = ImageChops.difference(image, Image.new("RGB", image.size, color)).split()
    return ImageChops.lighter(ImageChops.lighter(*differences[:2]), differences[2]).point(
        [255] + [0] * 255
    ).getbbox()


def command(*args):
    return subprocess.check_output(args, text=True, stderr=subprocess.STDOUT, timeout=8)


def digest(path):
    with path.open("rb") as handle:
        return hashlib.file_digest(handle, "sha256").hexdigest()


def wait_for(label, condition, process=None):
    for _ in range(150):
        result = condition()
        if result:
            return result
        if process is not None and process.poll() is not None:
            raise RuntimeError(f"Engine exited {process.returncode} before {label}")
        time.sleep(0.08)
    raise TimeoutError(label)


@contextmanager
def launch(args, log, **opts):
    with log.open("wb") as handle:
        process = subprocess.Popen(args, stdout=handle, stderr=subprocess.STDOUT, **opts)
        try:
            yield process
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=3)


def client_window(pid):
    result = subprocess.run(["wmctrl", "-lp"], capture_output=True, text=True, timeout=8)
    if result.returncode and "Cannot get client list properties" not in result.stderr:
        result.check_returncode()
    for line in result.stdout.splitlines():
        xid, _, owner, *_ = line.split(maxsplit=4)
        if int(owner) == pid:
            return int(xid, 16)
    return None


def tap(x11, keysym):
    x11.key(True, keysym)
    x11.key(False, keysym)


def click(x11, window, x, y):
    # KAG requires pointer motion when a new choice appears under the cursor.
    x11.motion(window, x - 15, y)
    x11.motion(window, x, y)
    time.sleep(0.05)
    x11.button(True)
    time.sleep(0.05)
    x11.button(False)


def activate(window):
    command("xdotool", "windowactivate", "--sync", str(window))


def event(game, process, name, count=1):
    wait_for(name, lambda: read_events(game).count(name) >= count, process)


def snapshot(output, name, x11, window):
    image = capture(x11, window, geometry(x11, window))
    image.save(output / f"{name}.png")
    return image


@contextmanager
def desktop(mode, x11, output, env):
    if mode == "wayland":
        socket = "krkr2-desktop"
        config = output / "weston.ini"
        config.write_text("[shell]\nbinding-modifier=alt\npanel-position=none\nlocking=false\n")
        args = ["weston", "--backend=x11-backend.so", "--use-pixman", "--width=640", "--height=390",
                "--scale=2", f"--socket={socket}", "--idle-time=0", f"--config={config}"]
        with launch(args, output / "compositor.log", env=env) as compositor:
            wait_for("Wayland socket", lambda: (Path(env["XDG_RUNTIME_DIR"]) / socket).is_socket(), compositor)
            def weston_window():
                result = subprocess.run(["xdotool", "search", "--onlyvisible", "--class",
                                         "^Weston Compositor$"], capture_output=True, text=True, timeout=3)
                return int(result.stdout.splitlines()[0]) if result.stdout.strip() else None
            window = wait_for("Weston X11 window", weston_window, compositor)
            x11.focus(window)
            engine_env = dict(env, WAYLAND_DISPLAY=socket, SDL_VIDEO_DRIVER="wayland",
                              GDK_BACKEND="wayland", SDL_VIDEO_WAYLAND_PREFER_LIBDECOR="1",
                              LIBDECOR_FORCE_CSD="1")
            if "LIBDECOR_PLUGIN_DIR" not in env:
                libdir = Path(command("pkg-config", "--variable=libdir", "libdecor-0").strip())
                plugins = output / "libdecor"
                plugins.mkdir()
                shutil.copy2(libdir / "libdecor/plugins-1/libdecor-cairo.so", plugins)
                engine_env["LIBDECOR_PLUGIN_DIR"] = str(plugins)
            engine_env.pop("DISPLAY", None)
            yield window, engine_env
    else:
        ready = output / "wm-ready"
        with launch(["openbox", "--sm-disable", "--startup", shlex.join(["touch", str(ready)])],
                    output / "wm.log", env=env) as wm:
            wait_for("Openbox startup", ready.is_file, wm)
            yield None, dict(env, SDL_VIDEO_DRIVER="x11", GDK_BACKEND="x11")


def lifecycle(x11, output, game, process, window, mode):
    scale = 2 if mode == "wayland" else 1

    def frame():
        if mode == "wayland":
            return capture(x11, window, geometry(x11, window))
        x, y = origin(x11, window)
        width, height = geometry(x11, window)
        return capture(x11, x11.root, geometry(x11, x11.root)).crop((x, y, x + width, y + height))

    def take_frame(name):
        image = frame()
        image.save(output / f"{name}.png")
        return image

    def box():
        return content_box(frame())

    def rendered(width, height):
        rectangle = box()
        if rectangle is not None:
            x, y, right, bottom = rectangle
            if (right - x, bottom - y) == (width, height):
                return rectangle
        return None

    def wait_rendered(label, width, height):
        try:
            return wait_for(label, lambda: rendered(width, height), process)
        except TimeoutError:
            image = take_frame("last-frame")
            raise TimeoutError(f"{label}: expected {width}x{height}, observed {content_box(image)}")

    def window_state():
        return command("xprop", "-id", hex(window), "_NET_WM_STATE")

    size = (320 * scale, 240 * scale)
    initial = wait_rendered("windowed pixels", *size)
    take_frame("windowed")
    x11.motion(window, initial[0] + size[0] // 2, initial[1] + size[1] // 2)
    tap(x11, F11)
    event(game, process, "fullscreen-request:1")
    if mode == "x11":
        wait_for("EWMH fullscreen", lambda: "_NET_WM_STATE_FULLSCREEN" in window_state(), process)
        full_width, full_height = geometry(x11, x11.root)
    else:
        full_width, full_height = geometry(x11, window)
        assert (full_width, full_height) == (1280, 780)
    full_content_width = full_height * 4 // 3
    full = wait_rendered("fullscreen pixels", full_content_width, full_height)
    image = take_frame("fullscreen")
    assert full[0] > 0 and image.getpixel((0, full_height // 2)) == (0, 0, 0)
    tap(x11, F11)
    event(game, process, "fullscreen-request:0")
    if mode == "x11":
        wait_for("EWMH windowed", lambda: "_NET_WM_STATE_FULLSCREEN" not in window_state(), process)
    windowed = wait_rendered("pixels after leaving fullscreen", *size)
    take_frame("windowed-again")

    # Leave a key down across the focus loss. The engine must release it exactly once.
    before = read_events(game)
    x11.key(True)
    event(game, process, "key-down:1", before.count("key-down:1") + 1)
    if mode == "x11":
        command("xdotool", "windowminimize", str(window))
        wait_for("EWMH hidden", lambda: "_NET_WM_STATE_HIDDEN" in window_state(), process)
    else:
        # libdecor's Cairo frame has 32 logical pixel buttons, from the right:
        # close, maximize, minimize. The title bar sits directly above the content.
        click(x11, window, windowed[2] - 80 * scale, windowed[1] - 12 * scale)
        wait_for("minimized native Wayland surface", lambda: not box(), process)
    event(game, process, "key-up:0", before.count("key-up:0") + 1)
    event(game, process, "window:inactive", before.count("window:inactive") + 1)
    event(game, process, "system:inactive", before.count("system:inactive") + 1)
    snapshot(output, "minimized", x11, window if mode == "wayland" else x11.root)
    x11.key(False)
    if mode == "x11":
        activate(window)
        wait_for("EWMH restored", lambda: "_NET_WM_STATE_HIDDEN" not in window_state(), process)
    else:
        x11.key(True, ALT)
        tap(x11, TAB)
        x11.key(False, ALT)
    restored = wait_rendered("restored pixels", *size)
    event(game, process, "window:active", before.count("window:active") + 1)
    event(game, process, "system:active", before.count("system:active") + 1)
    x11.key(True)
    event(game, process, "key-down:1", before.count("key-down:1") + 2)
    x11.key(False)
    event(game, process, "key-up:0", before.count("key-up:0") + 2)

    point = restored[0] + 65 * scale, restored[1] + 165 * scale
    x11.motion(window, *point)
    x11.button(True)
    event(game, process, "mouse-down:65:165:1")
    x11.button(False)
    event(game, process, "mouse-up:65:165:0")
    # Weston composites the cursor over the click location. Sample the same
    # painted rectangle to the left of the cursor hotspot.
    sample = point[0] - 20 * scale, point[1]
    try:
        wait_for("repaint after the restored click", lambda: frame().getpixel(sample) == GREEN, process)
    except TimeoutError:
        take_frame("last-frame")
        raise
    take_frame("restored-click")
    tap(x11, ESCAPE)
    assert process.wait(timeout=8) == 0
    return {"output_scale": scale, "windowed_content": initial, "fullscreen_content": full,
            "restored_content": restored, "mouse_physical_pixels": point,
            "mouse_logical_pixels": [65, 165], "repaint_sample": sample}


def prepare_fcitx(output):
    config = output / "config" / "fcitx5"
    config.mkdir(parents=True)
    (config / "profile").write_text(
        "[Groups/0]\nName=Default\nDefault Layout=us\nDefaultIM=pinyin\n"
        "\n[Groups/0/Items/0]\nName=keyboard-us\nLayout=\n"
        "\n[Groups/0/Items/1]\nName=pinyin\nLayout=\n"
        "\n[GroupOrder]\n0=Default\n",
        encoding="utf-8",
    )
    (config / "conf").mkdir()
    (config / "conf" / "pinyin.conf").write_text("Prediction=False\n", encoding="utf-8")


def ime(x11, output, game, command_line, env):
    env = dict(env, SDL_IM_MODULE="fcitx", XMODIFIERS="@im=fcitx")
    root_size = geometry(x11, x11.root)

    def screen():
        return capture(x11, x11.root, root_size)

    with launch(["fcitx5", "--replace", "--ui", "classicui"], output / "fcitx.log", env=env) as daemon:
        wait_for("Fcitx5 D-Bus service", lambda: "true" in command(
            "gdbus", "call", "--session", "--dest", "org.freedesktop.DBus",
            "--object-path", "/org/freedesktop/DBus",
            "--method", "org.freedesktop.DBus.NameHasOwner", "org.fcitx.Fcitx5",
        ), daemon)
        with launch(command_line, output / "process.log", env=env) as process:
            event(game, process, "ready")
            window = wait_for("IME game X11 window", lambda: client_window(process.pid), process)
            activate(window)
            event(game, process, "window:active")
            subprocess.run(["fcitx5-remote", "-s", "pinyin"], env=env, check=True, timeout=5)
            subprocess.run(["fcitx5-remote", "-o"], env=env, check=True, timeout=5)
            origin_x, origin_y = origin(x11, window)
            checks = []
            for index, (x, y) in enumerate(((80, 60), (220, 110)), start=1):
                if index == 2:
                    tap(x11, F6)
                    event(game, process, "attention:220:110")
                baseline = screen()
                for key in "nihao":
                    tap(x11, ord(key))

                def candidate():
                    current = screen()
                    rectangle = ImageChops.difference(baseline, current).getbbox()
                    if rectangle is None:
                        return None
                    left, top, right, bottom = rectangle
                    if right - left < 30 or bottom - top < 15:
                        return None
                    return rectangle, current

                rectangle, image = wait_for("visible Fcitx5 candidate popup", candidate, process)
                image.save(output / f"candidate-{index}.png")
                left, top, _, _ = rectangle
                expected = (origin_x + x, origin_y + y)
                checks.append({"attention": expected, "popup": rectangle})
                assert abs(left - expected[0]) <= 16 and -4 <= top - expected[1] <= 55, checks
                tap(x11, ord(" "))
                event(game, process, "text:20320", index)
                event(game, process, "text:22909", index)
                screen().save(output / f"committed-{index}.png")
            first, second = (check["popup"] for check in checks)
            assert abs(second[0] - first[0] - 140) <= 5 and abs(second[1] - first[1] - 50) <= 5, checks
            assert [line for line in read_events(game) if line.startswith("text:")] == [
                "text:20320", "text:22909", "text:20320", "text:22909",
            ]
            tap(x11, ESCAPE)
            assert process.wait(timeout=8) == 0
    return {"candidate_checks": checks, "committed_text": "你好你好"}


def stage_kag(game, framework):
    source = framework / "data"
    shutil.copytree(source / "system", game / "system")
    shutil.copy2(source / "startup.tjs", game)
    (game / "scenario").mkdir()
    shutil.copy2(FIXTURES / "first.ks", game / "scenario")
    shutil.copy2(FIXTURES / "AfterStartup.tjs", game)


def play_kag(x11, output, game, command_line, env):
    def play(name, load_save=False):
        (game / "interaction-events.txt").unlink(missing_ok=True)
        with launch(command_line, output / f"{name}.log", env=env) as process:
            event(game, process, "menu:0")
            window = wait_for("KAG X11 window", lambda: client_window(process.pid), process)
            activate(window)

            def text_frame(label):
                def visible():
                    image = capture(x11, window, geometry(x11, window))
                    areas = [(32, 60, 625, 102), (32, 128, 625, 170)]
                    if label == "checkpoint":
                        areas.append((32, 196, 625, 238))
                    for area in areas:
                        if sum(image.crop(area).convert("L").histogram()[192:]) < 100:
                            return None
                    return image
                image = wait_for(f"drawn KAG text ({label})", visible, process)
                image.save(output / f"{name}-{label}.png")
                return image.crop((32, 60, 625, 102)).tobytes()

            menu = text_frame("menu")
            click(x11, window, 90, 148 if load_save else 80)
            event(game, process, "checkpoint:42")
            checkpoint = text_frame("checkpoint")
            assert menu != checkpoint
            click(x11, window, 90, 216 if load_save else 148)
            assert process.wait(timeout=8) == 0
            expected = ["menu:0", "checkpoint:42"] + ([] if load_save else ["saved"]) + ["exit"]
            assert read_events(game) == expected
            return checkpoint, expected

    saved_frame, saved_events = play("save")
    saves = [path for path in game.rglob("*") if re.fullmatch(r"data1\.(kdt|bmp)", path.name)]
    assert len(saves) == 1 and saves[0].stat().st_size > 0, saves
    save_hash = digest(saves[0])
    loaded_frame, loaded_events = play("load", load_save=True)
    assert saved_frame == loaded_frame and digest(saves[0]) == save_hash
    return {"save_sha256": save_hash, "save_size": saves[0].stat().st_size,
            "save_events": saved_events, "load_events": loaded_events}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("x11", "wayland", "ime", "kag"))
    parser.add_argument("engine", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--kag-framework", type=Path)
    args = parser.parse_args()
    engine = args.engine.resolve(strict=True)
    output = args.output.resolve()
    output.mkdir(parents=True)
    runtime = output / "runtime"
    runtime.mkdir(mode=0o700)
    game = output / "game"
    game.mkdir()
    fixture = FIXTURES / ("first.ks" if args.mode == "kag" else "startup.tjs")
    report = {"mode": args.mode, "engine_sha256": digest(engine), "fixture_sha256": digest(fixture)}
    if args.mode == "ime":
        prepare_fcitx(output)
    if args.mode == "kag":
        if not args.kag_framework:
            parser.error("kag mode requires --kag-framework")
        stage_kag(game, args.kag_framework)
        report["setup_sha256"] = digest(FIXTURES / "AfterStartup.tjs")
    else:
        shutil.copy2(fixture, game)
    env = dict(os.environ, XDG_RUNTIME_DIR=str(runtime), XDG_CONFIG_HOME=str(output / "config"),
               XDG_CACHE_HOME=str(output / "cache"),
               LC_ALL="C.UTF-8", SDL_IM_MODULE="none", XMODIFIERS="@im=none",
               LIBGL_ALWAYS_SOFTWARE="1", ALSOFT_DRIVERS="null")
    command_line = [str(engine), "--host=sdl3", "--game", str(game)]
    report["command"] = command_line
    x11 = X11()
    try:
        with desktop(args.mode, x11, output, env) as (compositor_window, engine_env):
            if args.mode == "kag":
                report.update(play_kag(x11, output, game, command_line, engine_env))
            elif args.mode == "ime":
                report.update(ime(x11, output, game, command_line, engine_env))
            else:
                with launch(command_line, output / "process.log", env=engine_env, cwd=engine.parent) as process:
                    event(game, process, "ready")
                    window = compositor_window
                    if window is None:
                        window = wait_for("SDL3 X11 window", lambda: client_window(process.pid), process)
                        activate(window)
                    report.update(lifecycle(x11, output, game, process, window, args.mode))
            report["events"] = read_events(game)
        report["passed"] = True
        report["exit_code"] = 0
        report["forced_stop"] = False
    except Exception as error:
        report.update(passed=False, error=str(error), events=read_events(game))
        raise
    finally:
        x11.close()
        (output / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
