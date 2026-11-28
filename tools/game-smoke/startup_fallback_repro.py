"""Verify startup fallback after Initialize partially executes and throws.

Uses only a synthetic startup.tjs and system/Initialize.tjs in a fresh game
folder. The first Initialize attempt leaves a side effect then throws; the host
fallback must log that original error, retry Initialize once, and allow the
AfterStartup hook to observe both attempts and exit normally.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile


PREFIX = "STARTUP_FALLBACK_REPRO|"
FIRST_EXCEPTION = "STARTUP_FALLBACK_REPRO_FIRST_EXCEPTION"

STARTUP = r'''global.initializeAttempts = 0;
global.startupPartialEffect = "startup-entered";
Scripts.execStorage("system/Initialize.tjs");
'''

INITIALIZE = rf'''global.initializeAttempts++;
Debug.message("{PREFIX}INITIALIZE|" + global.initializeAttempts);
if (global.initializeAttempts == 1) throw "{FIRST_EXCEPTION}";
'''

AFTER_STARTUP = rf'''var ok = global.initializeAttempts == 2 &&
        global.startupPartialEffect == "startup-entered";
Debug.message("{PREFIX}AFTER|" + global.initializeAttempts + "|" + (ok ? "pass" : "fail"));
System.exit();
'''


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(engine, parent, timeout):
    parent.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="startup-fallback-", dir=parent))
    (root / "system").mkdir()
    scripts = {
        root / "startup.tjs": STARTUP,
        root / "system" / "Initialize.tjs": INITIALIZE,
        root / "AfterStartup.tjs": AFTER_STARTUP,
    }
    for path, source in scripts.items():
        path.write_bytes(b"\xff\xfe" + source.encode("utf-16le"))

    command = [str(engine), "--game=" + root.as_posix(), "--set=showfps=0"]
    env = os.environ.copy()
    env.setdefault("LIBGL_ALWAYS_SOFTWARE", "1")
    env.setdefault("ALSOFT_DRIVERS", "null")
    if sys.platform.startswith("linux") and not env.get("DISPLAY"):
        xvfb = shutil.which("xvfb-run")
        if not xvfb:
            raise RuntimeError("Set DISPLAY or install xvfb-run for the real runtime")
        command = [xvfb, "-a", "-s", "-screen 0 1280x720x24", *command]

    report = {
        "engine": str(engine),
        "engine_sha256": digest(engine),
        "scripts": {str(path.relative_to(root)): digest(path) for path in scripts},
        "command": command,
        "timeout_seconds": timeout,
        "forced_stop": False,
    }
    log_path = root / "process.log"
    with log_path.open("wb") as output:
        process = subprocess.Popen(command, cwd=engine.parent, env=env,
                                   stdout=output, stderr=subprocess.STDOUT,
                                   start_new_session=(os.name == "posix"))
        try:
            report["exit_code"] = process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            report["forced_stop"] = True
            if os.name == "posix":
                try:
                    os.killpg(process.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
            else:
                process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                if os.name == "posix":
                    try:
                        os.killpg(process.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                else:
                    process.kill()
                process.wait(timeout=5)
            report["exit_code"] = process.returncode

    output = log_path.read_text(encoding="utf-8", errors="replace")
    expected = {
        "first_initialize": f"{PREFIX}INITIALIZE|1",
        "fallback_initialize": f"{PREFIX}INITIALIZE|2",
        "after_startup": f"{PREFIX}AFTER|2|pass",
        "startup_completed": "Startup script ended.",
        "original_error_logged": f"Startup script failed before Initialize fallback: {FIRST_EXCEPTION}",
    }
    failures = []
    if report["exit_code"] != 0 or report["forced_stop"]:
        failures.append(f"Runtime exited with {report['exit_code']}; forced_stop={report['forced_stop']}")
    for name, marker in expected.items():
        count = output.count(marker)
        if count != 1:
            failures.append(f"Expected exactly one {name} marker, got {count}: {marker}")
    report["events"] = {name: output.count(marker) for name, marker in expected.items()}
    report["failures"] = failures
    report["passed"] = not failures
    (root / "report.json").write_text(
        json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Startup fallback evidence: {root}", flush=True)
    print(json.dumps(report, ensure_ascii=False))
    if failures:
        print(output[-12000:], file=sys.stderr)
        for failure in failures:
            print(failure, file=sys.stderr)
    return 0 if report["passed"] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, default=Path("out/startup-fallback"))
    parser.add_argument("--timeout", type=float, default=45)
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    return run(args.engine.resolve(strict=True), args.output_dir.resolve(), args.timeout)


if __name__ == "__main__":
    raise SystemExit(main())
