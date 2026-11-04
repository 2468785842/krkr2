"""Check touch ownership across two real TJS Windows via GLView dispatch."""
import argparse
import json
from pathlib import Path
import time

import interaction_smoke
from interaction_repro import Reproducer


EXPECTED = [
    "pointer:a:down:0", "pointer:a:up:0",  # switching cancels A's drag
    "pointer:b:down:0", "pointer:b:up:0",  # old A touch cannot release B
    "pointer:b:down:0", "pointer:b:click", "pointer:b:up:0",
    "pointer:b:down:0",  # invalidation removes callbacks to B
    "pointer:a:down:0", "pointer:a:up:0",  # old B touch cannot release A
    "pointer:a:down:0", "pointer:a:click", "pointer:a:up:0",
]


def failures(events):
    errors = [line for line in events if line.startswith("error:")]
    actual = [line for line in events if line.startswith("pointer:")]
    if actual != EXPECTED:
        errors.append(f"Window pointer callback mismatch: expected {EXPECTED!r}, got {actual!r}")
    for event in ("ready", "pointer-windows-ready", "pointer-window-b",
                  "pointer-window-b-held", "pointer-window-b-invalidated",
                  "pointer-window-a-held", "fixture-finished"):
        if events.count(event) != 1:
            errors.append(f"Expected exactly one {event}")
    return errors


class WindowPointerProbe(Reproducer):
    def __call__(self, process, directory, deadline, report):
        self.process, self.root, self.deadline = process, directory, deadline
        self.started = time.monotonic()
        try:
            self.wait(lambda: "ready" in interaction_smoke.read_events(directory), "ready")
            self.preserve_binary(report)
            before = self.command("pointer-windows")
            self.script_event("pointer-windows-ready", before)
            self.capture("windows-ready.png")

            before = self.command("pointer-a-drag")
            self.ui("touch-begin 1 0.5 0.5")
            self.ui("touch-move 1 0.6 0.5")
            self.script_event("pointer:a:down:0", before)
            switched = self.command("pointer-switch-b")
            self.script_event("pointer-window-b", switched)
            self.script_event("pointer:a:up:0", before)

            before = self.command("pointer-b-drag")
            self.ui("touch-begin 2 0.5 0.5")
            self.ui("touch-move 2 0.6 0.5")
            self.script_event("pointer:b:down:0", before)
            self.ui("touch-end 1 0.6 0.5")  # discarded A gesture, B still held
            before = self.command("pointer-b-held")
            self.script_event("pointer-window-b-held", before)
            self.ui("touch-cancel 2 0.6 0.5")
            self.script_event("pointer:b:up:0", before)
            before = self.command("pointer-b-tap")
            self.ui("touch-begin 3 0.5 0.5")
            self.ui("touch-end 3 0.5 0.5")
            self.script_event("pointer:b:up:0", before)

            before = self.command("pointer-b-invalidate-drag")
            self.ui("touch-begin 4 0.5 0.5")
            self.ui("touch-move 4 0.6 0.5")
            self.script_event("pointer:b:down:0", before)
            invalidated = self.command("pointer-invalidate-b")
            self.script_event("pointer-window-b-invalidated", invalidated)

            before = self.command("pointer-a-return-drag")
            self.ui("touch-begin 5 0.5 0.5")
            self.ui("touch-move 5 0.6 0.5")
            self.script_event("pointer:a:down:0", before)
            self.ui("touch-end 4 0.6 0.5")  # invalid B's old contact
            before = self.command("pointer-a-held")
            self.script_event("pointer-window-a-held", before)
            self.ui("touch-end 5 0.6 0.5")
            self.script_event("pointer:a:up:0", before)
            before = self.command("pointer-a-return-tap")
            self.ui("touch-begin 6 0.5 0.5")
            self.ui("touch-end 6 0.5 0.5")
            self.script_event("pointer:a:up:0", before)
            self.capture("windows-complete.png")
            self.command("finish")
        finally:
            (directory / "repro-trace.json").write_text(
                json.dumps(self.trace, indent=2), encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine", type=Path)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    engine = args.engine.resolve(strict=True)
    interaction_smoke.prepare(args.directory)
    return interaction_smoke.run(engine, args.directory, 180, component_input=True,
                                 automation=WindowPointerProbe(engine, 1), evaluate=failures)


if __name__ == "__main__":
    raise SystemExit(main())
