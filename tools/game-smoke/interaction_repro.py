"""Repeat real UI contract actions, optionally attaching CDB for crash evidence.

The native message box needs actual Windows input. Component input never supplies
its return value. The existing smoke controller owns the engine and judges success.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time

import interaction_smoke


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def cdb_commands(engine, directory):
    dump = str(directory / "crash.dmp").replace("\\", "/")
    handler = ('.echo KRKR_UI_ACCESS_VIOLATION; .exr -1; r; kv; '
               f'.dump /m /u \\"{dump}\\"; gn')
    return (f'sxe -c "{handler}" -c2 "{handler}" av; .reload /f {engine.name}; lmvm {engine.stem}; '
            '.echo KRKR_UI_DEBUGGER_READY; g')


class Reproducer:
    def __init__(self, engine, rounds, cdb=None, keyboard_probe=False, lifecycle_probe=False):
        if not 1 <= rounds <= 20:
            raise ValueError("Rounds must be 1..20")
        self.engine, self.rounds, self.cdb = engine, rounds, cdb
        self.keyboard_probe = keyboard_probe
        self.lifecycle_probe = lifecycle_probe
        self.sequence = 0
        self.trace = []

    def record(self, action, **details):
        self.trace.append({"elapsed": round(time.monotonic() - self.started, 3),
                           "action": action, **details})

    def wait(self, condition, label, seconds=20):
        end = min(self.deadline, time.monotonic() + seconds)
        while time.monotonic() < end:
            if condition():
                self.record("observed", event=label)
                return
            errors = [line for line in interaction_smoke.read_events(self.root) if line.startswith("error:")]
            if errors:
                raise RuntimeError("; ".join(errors))
            if self.process.poll() is not None:
                raise RuntimeError(f"Engine exited {self.process.returncode} while waiting for {label}")
            time.sleep(0.1)
        raise TimeoutError(label)

    def write_request(self, filename, value):
        temporary = self.root / (filename + ".pending")
        temporary.write_text(value + "\n", encoding="utf-8")
        # Publish the complete request; the game must not observe a partial line.
        self.wait(lambda: self.replace_request(temporary, self.root / filename), filename)
        self.record("request", file=filename, value=value)

    @staticmethod
    def replace_request(source, target):
        try:
            source.replace(target)
            return True
        except PermissionError:
            return False  # A brief read by the fixture can hold the file open.

    def command(self, command):
        previous = Counter(interaction_smoke.read_events(self.root))
        self.write_request("interaction-command.txt", command)
        self.script_event("command:" + command, previous)
        return previous

    def script_event(self, event, previous, seconds=20):
        self.wait(lambda: interaction_smoke.read_events(self.root).count(event) > previous[event],
                  event, seconds)

    def ui(self, action, seconds=20):
        self.sequence += 1
        prefix = f"{self.sequence} "
        self.write_request("ui-action.txt", prefix + action)

        def acknowledged():
            path = self.root / "ui-driver-events.txt"
            lines = path.read_text(encoding="utf-8").splitlines() if path.exists() else []
            errors = [line for line in lines if " error:" in line]
            if errors:
                raise RuntimeError("; ".join(errors))
            required = {prefix + "complete " + action.split()[0]}
            if action == "capture":
                required.add(prefix + "capture-saved")
            return required.issubset(lines)

        self.wait(acknowledged, prefix + action, seconds)

    def settle(self):
        # Let the existing UI enter/leave animation finish. Actual return values
        # and input acknowledgements, rather than this delay, decide success.
        remaining = self.deadline - time.monotonic()
        if remaining < 1:
            raise TimeoutError("UI transition exceeded overall deadline")
        time.sleep(1)

    def capture(self, name):
        self.ui("capture")
        shutil.copy2(self.root / "ui-frame.png", self.root / name)
        self.record("screenshot", file=name, sha256=digest(self.root / name))

    def preserve_binary(self, report):
        evidence = self.root / "evidence"
        evidence.mkdir()
        identities = {}
        for source in [self.engine, self.engine.with_suffix(".pdb")]:
            if not source.is_file():
                if source.suffix == ".pdb" and self.cdb is None:
                    continue
                raise FileNotFoundError(source)
            destination = evidence / source.name
            shutil.copy2(source, destination)
            copied_hash = digest(destination)
            if copied_hash != digest(source):
                raise RuntimeError(f"Binary changed during evidence capture: {source}")
            identities[source.name] = copied_hash
        if identities[self.engine.name] != report["engine_sha256"]:
            raise RuntimeError("Engine changed after launch")
        report["evidence"] = {"directory": str(evidence), "sha256": identities}
        return evidence

    def __call__(self, process, directory, deadline, report):
        self.process, self.root, self.deadline = process, directory, deadline
        self.started = time.monotonic()
        debugger = None
        debugger_log = None
        report["reproducer"] = {"rounds_requested": self.rounds, "rounds_completed": 0,
                                "debugger": str(self.cdb) if self.cdb else None,
                                "keyboard_probe": self.keyboard_probe,
                                "lifecycle_probe": self.lifecycle_probe}
        try:
            self.wait(lambda: "ready" in interaction_smoke.read_events(directory), "ready")
            evidence = self.preserve_binary(report)
            if self.cdb:
                commands = cdb_commands(self.engine, directory)
                debugger_log = (directory / "cdb.log").open("wb")
                debugger = subprocess.Popen([str(self.cdb), "-p", str(process.pid), "-G",
                                             "-y", str(evidence), "-c", commands],
                                            stdout=debugger_log, stderr=subprocess.STDOUT)
                self.record("debugger-start", pid=debugger.pid, commands=commands)

                def debugger_ready():
                    if debugger.poll() is not None:
                        raise RuntimeError("CDB exited before attaching")
                    lines = (directory / "cdb.log").read_text(encoding="utf-8", errors="replace").splitlines()
                    return any(line.strip() == "KRKR_UI_DEBUGGER_READY" for line in lines)

                self.wait(debugger_ready, "debugger-ready", seconds=120)
            self.ui("tree")
            if debugger is not None and debugger.poll() is not None:
                raise RuntimeError("CDB exited before the interaction sequence")
            if self.keyboard_probe:
                before = self.command("keyboard-probe")
                self.script_event("keyboard-probe-ready", before)
                self.ui("key-down A")
                self.script_event("input:key-down:65:0:1:0", before)
                self.ui("key-up A")
                self.script_event("input:key-up:65:0:0:0", before)
                before = self.command("keyboard-probe-end")
                self.script_event("keyboard-probe-done", before)
                self.capture("keyboard-probe.png")
            if self.lifecycle_probe:
                before = self.command("lifecycle-probe")
                self.script_event("lifecycle-probe-ready", before)
                self.ui("deactivate")
                self.script_event("lifecycle:deactivate", before)
                self.ui("activate")
                self.script_event("lifecycle:activate", before)
                before = self.command("lifecycle-probe-end")
                self.script_event("lifecycle-probe-done", before)
                self.capture("lifecycle-probe.png")
            before = self.command("inform")
            print("Confirm the real 'KrKr2 contract message' dialog to continue.", flush=True)
            self.script_event("inform-return:0", before, seconds=180)
            for index in range(1, self.rounds + 1):
                self.record("round-start", round=index)
                before = self.command("select-cancel")
                self.settle()
                self.capture(f"round-{index:02d}-file-selector.png")
                self.ui("click-name cancel")
                self.script_event("select-cancel-return:0", before)
                self.settle()
                before = self.command("select-accept")
                self.settle()
                self.ui("click-name ok")
                self.script_event("select-accept-return:1", before)
                self.settle()
                before = self.command("menu-open")
                self.script_event("menu-opened", before)
                self.settle()
                self.capture(f"round-{index:02d}-menu.png")
                self.ui("click-text Contract toggle")
                self.script_event("menu-click:1", before)
                self.settle()
                before = self.command("menu-open-again")
                self.script_event("menu-opened", before)
                self.settle()
                self.capture(f"round-{index:02d}-menu-checked.png")
                self.ui("click-text Contract toggle")
                self.script_event("menu-click:0", before)
                # The checked-item callback keeps its form open. Dismiss it
                # through the real back button before starting another round.
                self.ui("click-name left")
                self.settle()
                report["reproducer"]["rounds_completed"] = index
            before = self.command("veto")
            self.script_event("veto-preserved", before)
            self.capture("close-veto.png")
            before = self.command("accept")
            self.script_event("close-returned", before)
            process.wait(timeout=max(0, deadline - time.monotonic()))
        finally:
            if debugger is not None:
                if process.poll() is None:
                    report["forced_stop"] = True
                    process.terminate()
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait(timeout=5)
                try:
                    debugger.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    debugger.terminate()
                    debugger.wait(timeout=5)
                report["debugger_exit_code"] = debugger.returncode
                if debugger.returncode != 0:
                    report.setdefault("automation_failures", []).append(f"CDB exited {debugger.returncode}")
            if debugger_log is not None:
                debugger_log.close()
                log = (directory / "cdb.log").read_text(encoding="utf-8", errors="replace")
                if "KRKR_UI_ACCESS_VIOLATION" in [line.strip() for line in log.splitlines()]:
                    report.setdefault("automation_failures", []).append("CDB observed an access violation")
                report["crash_dumps"] = [str(path) for path in sorted(directory.glob("crash*.dmp"))]
            (directory / "repro-trace.json").write_text(json.dumps(self.trace, ensure_ascii=False, indent=2), encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine", type=Path)
    parser.add_argument("destination", type=Path, help="New private fixture directory")
    parser.add_argument("--rounds", type=int, default=3)
    parser.add_argument("--keyboard-probe", action="store_true", help="Check queued A-key callbacks and System.getKeyState semantics")
    parser.add_argument("--lifecycle-probe", action="store_true", help="Check engine activation notifications without stopping the backend pump")
    parser.add_argument("--seconds", type=int, default=300)
    parser.add_argument("--cdb", type=Path, help="Existing CDB executable; attach and capture access violations")
    args = parser.parse_args()
    engine = args.engine.resolve(strict=True)
    cdb = args.cdb.resolve(strict=True) if args.cdb else None
    driver = Reproducer(engine, args.rounds, cdb, args.keyboard_probe, args.lifecycle_probe)
    if not 5 <= args.seconds <= 600:
        parser.error("Duration must be 5..600 seconds")
    interaction_smoke.prepare(args.destination)
    return interaction_smoke.run(engine, args.destination, args.seconds, component_input=True, automation=driver)


if __name__ == "__main__":
    raise SystemExit(main())
