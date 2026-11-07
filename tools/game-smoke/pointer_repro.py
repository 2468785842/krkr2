"""Probe real GLView touch dispatch and TJS Window callbacks (no OS device claim)."""
import argparse
import json
from pathlib import Path
import time

import interaction_smoke
from interaction_repro import Reproducer


EXPECTED = [
    "pointer:down:0", "pointer:up:0",  # drag cancelled without click
    "pointer:down:1", "pointer:click", "pointer:up:1",  # two-finger short tap
    "pointer:down:0", "pointer:up:0",  # hide cancels drag
    "pointer:down:0", "pointer:click", "pointer:up:0",  # new gesture works
]


def failures(events):
    errors = [line for line in events if line.startswith("error:")]
    actual = [line for line in events if line.startswith("pointer:")]
    if actual != EXPECTED:
        errors.append(f"Pointer callback mismatch: expected {EXPECTED!r}, got {actual!r}")
    for event in ("pointer-probe-ready", "pointer-hidden", "pointer-shown", "fixture-finished"):
        if events.count(event) != 1:
            errors.append(f"Expected exactly one {event}")
    return errors


class PointerProbe(Reproducer):
    def __call__(self, process, directory, deadline, report):
        self.process, self.root, self.deadline = process, directory, deadline
        self.started = time.monotonic()
        try:
            self.wait(lambda: "ready" in interaction_smoke.read_events(directory), "ready")
            self.preserve_binary(report)
            before = self.command("pointer-probe")
            self.script_event("pointer-probe-ready", before)
            self.capture("pointer-ready.png")

            before = self.command("pointer-cancel-start")
            self.ui("touch-begin 1 0.5 0.5")
            self.ui("touch-move 1 0.6 0.5")
            self.script_event("pointer:down:0", before)
            self.ui("touch-cancel 1 0.6 0.5")
            self.script_event("pointer:up:0", before)

            before = self.command("pointer-right-start")
            self.ui("touch-begin 1 0.5 0.5")
            self.ui("touch-begin 2 0.55 0.5")
            self.ui("touch-end 2 0.55 0.5")
            self.ui("touch-end 1 0.5 0.5")
            self.script_event("pointer:up:1", before)

            before = self.command("pointer-hide-start")
            self.ui("touch-begin 1 0.5 0.5")
            self.ui("touch-move 1 0.6 0.5")
            self.script_event("pointer:down:0", before)
            self.command("pointer-hide")
            self.script_event("pointer:up:0", before)
            before = self.command("pointer-show")
            self.script_event("pointer-shown", before)
            self.ui("touch-end 1 0.6 0.5")  # late old touch must not click

            before = self.command("pointer-new-start")
            self.ui("touch-begin 1 0.5 0.5")
            self.ui("touch-end 1 0.5 0.5")
            self.script_event("pointer:up:0", before)
            self.capture("pointer-complete.png")
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
                                 automation=PointerProbe(engine, 1), evaluate=failures)


if __name__ == "__main__":
    raise SystemExit(main())
