"""Verify Win32 committed Unicode and Cocos UTF8 dispatch to real TJS callbacks."""
import argparse
import json
from pathlib import Path
import shutil
import sys
import time

import interaction_smoke
from focus_repro import FocusProbe
from win32_input import Desktop
from frame_pixels import focus_frame_failures


NATIVE_TEXT = "A\u4e2d\u3042\u00e9e\u0301\U0001f642"
NATIVE = [65, 20013, 12354, 233, 101, 769, 55357, 56898]
RECOVERED = [90, 55357, 56898]
EXPECTED = ([f"text:native:1:{unit}" for unit in NATIVE] +
            ["text:component:1:65", "text:component:1:20013"] +
            [f"text:recovered:1:{unit}" for unit in RECOVERED])
MARKERS = ["ready", "phase:native", "phase:component", "phase:component-done",
           "phase:unfocused", "phase:unfocused-done", "phase:recovered", "fixture-finished"]


def failures(events):
    errors = [event for event in events if event.startswith("error:")]
    text = [event for event in events if event.startswith("text:")]
    if text != EXPECTED:
        errors.append(f"Committed text mismatch: expected {EXPECTED!r}, got {text!r}")
    for marker in MARKERS:
        if events.count(marker) != 1:
            errors.append(f"Expected exactly one {marker}")
    return errors


class TextProbe(FocusProbe):
    def native_text(self, window, text):
        if self.desktop.foreground() != window:
            raise RuntimeError("Native text target lost foreground")
        self.desktop.unicode_text(text)
        self.record("native-unicode", hwnd=window, scalars=[ord(char) for char in text])

    def phase(self, name):
        before = self.command(name)
        self.script_event("phase:" + name, before)
        return before

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

            self.wait(mapped, "mapped text window")
            window = windows[-1]
            helper = self.desktop.create_helper()
            report["text_input"] = {"engine_hwnd": window, "helper_hwnd": helper,
                                    "native": "Win32 SendInput VK_PACKET",
                                    "component": "Cocos IMEDispatcher UTF8"}
            self.foreground(window, "text-engine-initial")
            before = self.phase("native")
            self.native_text(window, NATIVE_TEXT)
            self.script_event("text:native:1:56898", before)
            self.capture("native-focus-ready.png")

            before = self.phase("component")
            # The decoder preserves NUL, but TVPWindowLayer::OnKeyPress ignores
            # unit zero. The following CJK unit must still reach the script.
            self.ui("text-hex 4100e4b8ad")
            self.script_event("text:component:1:20013", before)
            for invalid in ("41eda080", "41c080", "41f4908080", "41e4b8"):
                self.ui("text-hex " + invalid)  # reject the entire commit
            self.phase("component-done")

            self.foreground(helper, "text-helper-unfocused")
            self.phase("unfocused")
            self.ui("text-hex 41e4b8adf09f9982")
            self.phase("unfocused-done")
            if self.desktop.foreground() != helper:
                raise RuntimeError("Text helper lost foreground")
            self.foreground(window, "text-engine-recovered")
            before = self.phase("recovered")
            self.native_text(window, "Z\U0001f642")
            self.script_event("text:recovered:1:56898", before)
            self.capture("native-focus-recovered.png")
            self.command("finish")
        finally:
            report.setdefault("text_input", {})["last_caption_input"] = self.desktop.last_caption
            try:
                self.desktop.close()
            finally:
                (directory / "repro-trace.json").write_text(json.dumps(self.trace, indent=2), encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine", type=Path)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    if sys.platform != "win32":
        parser.error("Native text input requires Windows")
    engine = args.engine.resolve(strict=True)
    interaction_smoke.prepare(args.directory)
    shutil.copy2(Path(__file__).with_name("text") / "startup.tjs", args.directory / "startup.tjs")
    return interaction_smoke.run(engine, args.directory, 180, component_input=True,
                                 automation=TextProbe(engine, 1),
                                 evaluate=lambda events: failures(events) + focus_frame_failures(args.directory))


if __name__ == "__main__":
    raise SystemExit(main())
