"""Verify real Win32 focus loss with native A-key input and GLView touches."""
import argparse
from collections import Counter
import json
from pathlib import Path
import shutil
import sys
import time

import interaction_smoke
from interaction_repro import Reproducer
from win32_input import Desktop
from frame_pixels import focus_frame_failures


HELD_DOWN = ["focus:held:mouse-down:0:0:1", "focus:held:key-down:65:1:1"]
CANCELLED = ["focus:held:mouse-up:0:0:0", "focus:held:key-up:65:0:0"]
RECOVERED = ["focus:recovered:key-down:65:1:0", "focus:recovered:key-up:65:0:0",
             "focus:recovered:mouse-down:0:0:1", "focus:recovered:mouse-up:0:0:0"]
REQUIRED = ["ready", "state:held", "state:unfocused", "state:unfocused-check",
            "state:recovered-ready", "state:recovered-empty", "fixture-finished"]


def failures(events, minimized=False):
    errors = [event for event in events if event.startswith("error:")]
    lifecycle = [event for event in events if event.startswith("lifecycle:")]
    if minimized:
        # SystemImpl replaces pending activation events and checks the current
        # state at delivery. A stopped frame pump may coalesce the background
        # event, but restore must deliver activate exactly once.
        if lifecycle not in (["lifecycle:activate"],
                             ["lifecycle:deactivate", "lifecycle:activate"]):
            errors.append(f"Minimize/restore lifecycle mismatch: {lifecycle!r}")
    else:
        errors.extend(lifecycle)
    held = [event for event in events if event.startswith("focus:held:")]
    # GLFW can synthesize A-up before its focus notification. Both up callbacks
    # must see fully released state; their order is not the contract under test.
    if held[:2] != HELD_DOWN or Counter(held[2:]) != Counter(CANCELLED):
        errors.append(f"Held/cancelled callbacks mismatch: {held!r}")
    recovered = [event for event in events if event.startswith("focus:recovered:")]
    if recovered != RECOVERED:
        errors.append(f"Recovered callbacks mismatch: {recovered!r}")
    errors.extend(event for event in events if event.startswith("focus:") and
                  not event.startswith(("focus:held:", "focus:recovered:")))
    required = [event for event in REQUIRED if not minimized or event != "state:unfocused-check"]
    for event in required:
        if events.count(event) != 1:
            errors.append(f"Expected exactly one {event}")
    return errors


class FocusProbe(Reproducer):
    def __init__(self, engine, rounds, minimized=False, us_keyboard_layout=False):
        super().__init__(engine, rounds)
        self.minimized = minimized
        self.us_keyboard_layout = us_keyboard_layout

    def wait(self, condition, label, seconds=20):
        def pumped():
            self.desktop.pump()
            return condition()
        return super().wait(pumped, label, seconds)

    def foreground(self, window, label):
        if self.minimized:
            self.desktop.raise_without_activation(window)
        point = self.desktop.click_caption(window)
        self.record("native-caption-input", target=label, **point)
        self.wait(lambda: self.desktop.foreground() == window, label)
        self.record("native-foreground", target=label, hwnd=window)

    def key(self, window, down):
        if self.desktop.foreground() != window:
            raise RuntimeError("Native key target lost foreground")
        self.desktop.key(65, down)
        self.record("native-key", hwnd=window, key=65, down=down,
                    keyboard_layout=self.desktop.keyboard_layout(window))

    def __call__(self, process, directory, deadline, report):
        self.process, self.root, self.deadline = process, directory, deadline
        self.started = time.monotonic()
        self.desktop = Desktop()
        try:
            self.wait(lambda: "ready" in interaction_smoke.read_events(directory), "ready")
            self.preserve_binary(report)
            windows = []

            def mapped():
                window = self.desktop.window_for_pid(process.pid)
                if window:
                    windows.append(window)
                return bool(window)

            self.wait(mapped, "mapped engine window")
            engine_window = windows[-1]
            helper = None if self.minimized else self.desktop.create_helper()
            report["native_focus"] = {"engine_hwnd": engine_window, "helper_hwnd": helper,
                                      "keyboard": "Win32 SendInput", "touches": "GLView",
                                      "minimize_restore": self.minimized}
            self.foreground(engine_window, "engine-initial")
            if self.us_keyboard_layout:
                previous_layout = self.desktop.keyboard_layout(engine_window)
                requested = self.desktop.request_installed_us_layout(engine_window)
                self.wait(lambda: self.desktop.keyboard_layout(engine_window) == requested,
                          "owned-engine-english-layout")
                self.record("owned-engine-keyboard-layout", before=previous_layout, after=requested)
                report["native_focus"]["requested_keyboard_layout"] = requested
            before = self.command("held")
            self.script_event("state:held", before)
            self.capture("native-focus-ready.png")
            self.ui("touch-begin 1 0.5 0.5")
            self.ui("touch-move 1 0.6 0.5")
            self.script_event(HELD_DOWN[0], before)
            self.key(engine_window, True)
            self.script_event(HELD_DOWN[1], before)

            if self.minimized:
                self.desktop.minimize(engine_window)
                self.wait(lambda: self.desktop.iconic(engine_window), "engine-minimized")
                self.record("native-minimized", hwnd=engine_window, iconic=True)
                # The script Timer and component driver depend on the stopped
                # Director pump. Publish a request without waiting for its ack;
                # it must remain pending until the OS window is restored.
                self.write_request("interaction-command.txt", "unfocused")
                end = time.monotonic() + 1
                while time.monotonic() < end:
                    self.desktop.pump()
                    if not self.desktop.iconic(engine_window):
                        raise RuntimeError("Engine restored before requested")
                    if "command:unfocused" in interaction_smoke.read_events(directory):
                        raise RuntimeError("Script Timer ran while engine was minimized")
                    if process.poll() is not None:
                        raise RuntimeError("Engine exited while minimized")
                    time.sleep(0.05)
                self.record("minimized-request-pending", seconds=1)
                self.desktop.restore(engine_window)
                self.wait(lambda: not self.desktop.iconic(engine_window), "engine-restored")
                self.record("native-restored", hwnd=engine_window, iconic=False)
                self.script_event("lifecycle:activate", before)
                self.script_event("state:unfocused", before)
                # IsIconic clears before the native restore animation finishes.
                # Let that transition finish before clicking another caption;
                # actual foreground ownership is still required below.
                self.settle()

            if not self.minimized:
                self.foreground(helper, "helper-unfocused")
            for event in CANCELLED:
                self.script_event(event, before)
            if not self.minimized:
                before = self.command("unfocused")
                self.script_event("state:unfocused", before)
                self.key(helper, False)  # native up goes to the helper
                self.ui("key-down A")
                self.ui("key-up A")
                self.ui("touch-move 1 0.65 0.5")
                self.ui("touch-begin 2 0.5 0.5")
                self.ui("touch-move 2 0.6 0.5")
                self.ui("touch-end 2 0.6 0.5")
                before = self.command("unfocused-check")
                self.script_event("state:unfocused-check", before)
                if self.desktop.foreground() != helper:
                    raise RuntimeError("Helper lost foreground during unfocused checks")

            before = self.command("recover")
            self.script_event("state:recovered-ready", before)
            self.foreground(engine_window, "engine-recovered")
            if self.minimized:
                self.key(engine_window, False)  # late native release after restore
            self.ui("touch-end 1 0.65 0.5")  # late contact from before focus loss
            self.ui("key-up A")
            before = self.command("recovered-check")
            self.script_event("state:recovered-empty", before)
            self.key(engine_window, True)
            self.script_event(RECOVERED[0], before)
            self.key(engine_window, False)
            self.script_event(RECOVERED[1], before)
            self.ui("touch-begin 3 0.5 0.5")
            self.ui("touch-move 3 0.6 0.5")
            self.script_event(RECOVERED[2], before)
            self.ui("touch-end 3 0.6 0.5")
            self.script_event(RECOVERED[3], before)
            self.capture("native-focus-recovered.png")
            self.command("finish")
        finally:
            report.setdefault("native_focus", {})["last_caption_input"] = self.desktop.last_caption
            report["native_focus"]["foreground_at_end"] = self.desktop.foreground()
            try:
                self.desktop.close()
            finally:
                (directory / "repro-trace.json").write_text(
                    json.dumps(self.trace, indent=2), encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine", type=Path)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--minimize", action="store_true",
                        help="Exercise native minimize/restore and the stopped script Timer")
    parser.add_argument("--us-keyboard-layout", action="store_true",
                        help="Select an already installed US layout in the owned engine thread")
    args = parser.parse_args()
    if sys.platform != "win32":
        parser.error("Native focus input requires Windows")
    engine = args.engine.resolve(strict=True)
    interaction_smoke.prepare(args.directory)
    shutil.copy2(Path(__file__).with_name("focus") / "startup.tjs", args.directory / "startup.tjs")
    return interaction_smoke.run(engine, args.directory, 180, component_input=True,
                                 automation=FocusProbe(engine, 1, args.minimize, args.us_keyboard_layout),
                                 evaluate=lambda events: failures(events, args.minimize) +
                                 (focus_frame_failures(args.directory) if args.minimize else []))


if __name__ == "__main__":
    raise SystemExit(main())
