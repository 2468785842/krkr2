"""Stage a private game copy and observe the real graphical engine, no GUI mocking."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def prepare(source, destination):
    source, destination = source.resolve(), destination.resolve()
    if not source.is_dir() or not list(source.glob("*.xp3")):
        raise ValueError("Source must be a game directory containing XP3 archives")
    if destination.exists() or source == destination or source in destination.parents:
        raise ValueError("Use a new destination outside the source game directory")
    if (source / "AfterStartup.tjs").exists():
        raise ValueError("Game has an AfterStartup hook; compose it explicitly before testing")
    destination.mkdir(parents=True)
    manifest = []
    # Savedata is intentionally not copied. Archives and compatibility patch are.
    for path in sorted(source.iterdir()):
        if path.is_file() and (path.suffix.lower() == ".xp3" or path.name.lower() == "patch.tjs"):
            shutil.copy2(path, destination / path.name)
            manifest.append({"name": path.name, "size": path.stat().st_size, "sha256": digest(path)})
    shutil.copy2(Path(__file__).with_name("AfterStartup.tjs"), destination / "AfterStartup.tjs")
    (destination / "smoke-manifest.json").write_text(json.dumps({"source": str(source), "files": manifest}, indent=2), encoding="utf-8")


def state_at(game, name="smoke-state.txt"):
    path = game / name
    if not path.exists():
        return []
    try:
        raw = path.read_bytes()
        return raw.decode("utf-16" if raw.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig").splitlines()
    except (UnicodeError, OSError):
        return []  # The engine may be replacing the heartbeat file.


def run(engine, game, seconds, launch_style="legacy", target_mode="auto", engine_args=(), probe_engine_args=False):
    engine, game = engine.resolve(strict=True), game.resolve(strict=True)
    if not (game / "smoke-manifest.json").is_file():
        raise ValueError("Run prepare first; never instrument the original game")
    if seconds < 2 or seconds > 600:
        raise ValueError("Duration must be 2..600 seconds")
    if any((game / name).exists() for name in ("smoke-state.txt", "smoke-error.txt", "smoke-frame.bmp", "smoke-arguments.txt")):
        raise ValueError("Use a fresh prepared directory for each run; stale evidence is rejected")
    if target_mode == "directory" or (game / "startup.tjs").exists():
        target = game.as_posix() + "/"
    elif (game / "data.xp3").exists():
        target = str(game / "data.xp3")
    else:
        raise ValueError("Provide a loose startup.tjs or a data.xp3 containing startup.tjs")
    report = {"engine": str(engine), "engine_sha256": digest(engine), "game": str(game),
              "seconds": seconds, "passed": False, "forced_stop": False}
    hook = game / "AfterStartup.tjs"
    report["hook_sha256"] = digest(hook) if hook.exists() else None
    report["manifest_sha256"] = digest(game / "smoke-manifest.json")
    engine_args = list(engine_args)
    if probe_engine_args:
        engine_args.extend(["-krkr-smoke-probe=yes", "-smoke-cli=old", "-smoke-cli=测试=a=b", "-smoke-empty=", "-smoke-flag"])
    report["probe_engine_args"] = probe_engine_args
    command = [str(engine)]
    if launch_style != "legacy":
        command.append("--game" if launch_style == "game" else "--xp3")
    command.append(target)
    if engine_args:
        command.extend(["--", *engine_args])
    report["command"] = command
    started = time.monotonic()
    samples = []
    last_change = started
    # Current desktop runtime accepts the game directory as argv[1].
    with (game / "smoke-process.log").open("wb") as log:
        # Legacy selector accepts a directory only with loose startup.tjs.
        process = subprocess.Popen(command, cwd=engine.parent, stdout=log, stderr=subprocess.STDOUT)
        report["pid"] = process.pid
        try:
            while process.poll() is None and time.monotonic() - started < seconds:
                state = state_at(game)
                if state and (not samples or state != samples[-1]):
                    samples.append(state)
                    last_change = time.monotonic()
                time.sleep(0.25)
            report["alive_at_deadline"] = process.poll() is None
            report["heartbeat_age_seconds"] = round(time.monotonic() - last_change, 2)
            report["passed"] = (report["alive_at_deadline"] and len(samples) >= 2
                                and len(samples[-1]) >= 7 and samples[-1][1] == "heartbeat"
                                and time.monotonic() - last_change <= 3
                                and "kag-present" in samples[-1]
                                and not (game / "smoke-error.txt").exists())
            if probe_engine_args:
                report["engine_arguments"] = state_at(game, "smoke-arguments.txt")
                report["engine_argument_probe_passed"] = report["engine_arguments"] == ["测试=a=b", "", "yes", "1"]
                report["passed"] = report["passed"] and report["engine_argument_probe_passed"]
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
            report["exit_code_hex"] = f"0x{process.returncode & 0xffffffff:08X}"
            report["samples"] = samples
            report["elapsed_seconds"] = round(time.monotonic() - started, 2)
            (game / "smoke-report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    summary = {key: value for key, value in report.items() if key != "samples"}
    summary["sample_count"] = len(samples)
    summary["report"] = str(game / "smoke-report.json")
    print(json.dumps(summary, ensure_ascii=False, indent=2))
    return 0 if report["passed"] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    stage = commands.add_parser("prepare")
    stage.add_argument("source", type=Path)
    stage.add_argument("destination", type=Path)
    launch = commands.add_parser("run")
    launch.add_argument("engine", type=Path)
    launch.add_argument("game", type=Path)
    launch.add_argument("--seconds", type=int, default=30)
    launch.add_argument("--launch-style", choices=("legacy", "game", "xp3"), default="legacy")
    launch.add_argument("--target", choices=("auto", "directory"), default="auto")
    launch.add_argument("--engine-arg", action="append", default=[])
    launch.add_argument("--probe-engine-args", action="store_true",
                        help="Assert Unicode, duplicate, empty and flag values through System.getArgument")
    args = parser.parse_args()
    if args.command == "prepare":
        prepare(args.source, args.destination)
        return 0
    return run(args.engine, args.game, args.seconds, args.launch_style, args.target, args.engine_arg, args.probe_engine_args)


if __name__ == "__main__":
    raise SystemExit(main())
