"""Stage a self-contained UI fixture and check evidence from real user/UI actions.

The fixture accepts request files for opening dialogs and exercising close callbacks.
It never fabricates menu clicks or dialog results; operate the real window separately.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time


def read_events(directory):
    path = directory / "interaction-events.txt"
    if not path.exists():
        return []
    try:
        raw = path.read_bytes()
        return raw.decode("utf-16" if raw.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig").splitlines()
    except (OSError, UnicodeError):
        return []


def failures(events):
    required = {"ready", "inform-return:0", "select-cancel-return:0", "select-accept-return:1",
                "menu-click:1", "menu-click:0", "veto-preserved", "close-query:0", "close-query:1", "close-returned"}
    missing = sorted(required - set(events))
    names = [line.partition(":")[2] for line in events if line.startswith("select-accept-name:")]
    if not names or not names[-1].replace("\\", "/").endswith("/fixture.txt"):
        missing.append("selected fixture.txt path")
    return missing + [line for line in events if line.startswith("error:")]


def prepare(destination):
    destination.mkdir(parents=True, exist_ok=False)
    shutil.copy2(Path(__file__).with_name("interaction") / "startup.tjs", destination / "startup.tjs")
    (destination / "fixture.txt").write_text("KrKr2 UI contract fixture\n", encoding="utf-8")
    (destination / "interaction-fixture.json").write_text('{"version":1}', encoding="utf-8")


def run(engine, directory, seconds, launch_style="game", component_input=False, automation=None):
    engine, directory = engine.resolve(strict=True), directory.resolve(strict=True)
    if not (directory / "interaction-fixture.json").is_file():
        raise ValueError("Prepare a private fixture first")
    if any((directory / name).exists() for name in ("interaction-events.txt", "interaction-command.txt", "interaction-report.json")):
        raise ValueError("Run requires a fresh fixture")
    if not 5 <= seconds <= 600:
        raise ValueError("Duration must be 5..600 seconds")
    with engine.open("rb") as binary, (directory / "startup.tjs").open("rb") as fixture:
        report = {"engine": str(engine), "engine_sha256": hashlib.file_digest(binary, "sha256").hexdigest(),
                  "fixture_sha256": hashlib.file_digest(fixture, "sha256").hexdigest(),
                  "forced_stop": False}
    with (directory / "interaction-process.log").open("wb") as log:
        command = [str(engine)]
        if launch_style == "game":
            command.append("--game")
        command.append(directory.as_posix() + "/")
        if component_input:
            command.extend(["--", "-ui-contract-driver=yes"])
        report["component_input"] = component_input
        report["command"] = command
        process = subprocess.Popen(command, cwd=engine.parent, stdout=log, stderr=subprocess.STDOUT)
        report["pid"] = process.pid
        print(f"Fixture PID {process.pid}; requests: {directory / 'interaction-command.txt'}", flush=True)
        deadline = time.monotonic() + seconds
        try:
            if automation is not None:
                automation(process, directory, deadline, report)
            process.wait(timeout=max(0, deadline - time.monotonic()))
        except subprocess.TimeoutExpired:
            report["forced_stop"] = True
        except Exception as error:
            report.setdefault("automation_failures", []).append(f"automation:{type(error).__name__}: {error}")
        finally:
            if process.poll() is None:
                report["forced_stop"] = True
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)
            report["exit_code"] = process.returncode
            report["events"] = read_events(directory)
            report["failures"] = failures(report["events"])
            report["failures"].extend(report.get("automation_failures", []))
            driver_log = directory / "ui-driver-events.txt"
            report["driver_events"] = driver_log.read_text(encoding="utf-8").splitlines() if driver_log.exists() else []
            if component_input:
                if not report["driver_events"]:
                    report["failures"].append("UI component driver did not run")
                report["failures"].extend(line for line in report["driver_events"] if " error:" in line)
            report["passed"] = not report["failures"] and not report["forced_stop"] and process.returncode == 0
            (directory / "interaction-report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0 if report["passed"] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    stage = commands.add_parser("prepare")
    stage.add_argument("destination", type=Path)
    launch = commands.add_parser("run")
    launch.add_argument("engine", type=Path)
    launch.add_argument("directory", type=Path)
    launch.add_argument("--seconds", type=int, default=300)
    launch.add_argument("--component-input", action="store_true")
    launch.add_argument("--launch-style", choices=("game", "positional"), default="game")
    args = parser.parse_args()
    if args.command == "prepare":
        prepare(args.destination)
        return 0
    return run(args.engine, args.directory, args.seconds, args.launch_style, args.component_input)


if __name__ == "__main__":
    raise SystemExit(main())
