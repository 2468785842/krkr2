"""Check parser error propagation and recovery against the real game runtime.

Creates only synthetic UTF-16LE scripts. A compiler error must be catchable,
must not execute any part of the rejected script, and must leave the engine
able to compile and run the next script. Linux uses xvfb-run when DISPLAY is
unset; an existing display can be supplied for a device/container run.
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


STARTUP = r'''var parserFailures = 0;
var parserSideEffect = 0;
var parserRecovered = 0;

function parserCheck(condition, name) {
    if (condition) {
        Debug.message("PARSER_REPRO|PASS|" + name);
    } else {
        parserFailures++;
        Debug.message("PARSER_REPRO|FAIL|" + name);
    }
}

Debug.message("PARSER_REPRO|BEGIN");
var caught = false;
try {
    // A recoverable grammar error: Bison may accept after the semicolon.
    Scripts.exec("global.parserSideEffect = 1;\nvar broken = ;\n",
                 "parser-exec-invalid.tjs", 40);
} catch (e) {
    caught = e.message != "";
}
parserCheck(caught, "exec-caught");
parserCheck(parserSideEffect == 0, "exec-no-side-effect");
Scripts.exec("global.parserRecovered += 1;", "parser-recovery.tjs");
parserCheck(parserRecovered == 1, "exec-recovered");

caught = false;
try {
    // EOF aborts while a nested context is still open.
    Scripts.exec("global.parserSideEffect = 2;\nfunction unfinished() {",
                 "parser-abort-invalid.tjs", 100);
} catch (e) {
    caught = e.message != "";
}
parserCheck(caught, "abort-caught");
parserCheck(parserSideEffect == 0, "abort-no-side-effect");
Scripts.exec("global.parserRecovered += 1;", "parser-recovery.tjs");
parserCheck(parserRecovered == 2, "abort-recovered");

caught = false;
try {
    var ignored = Scripts.eval("(global.parserSideEffect = 3,\n)",
                               "parser-eval-invalid.tjs", 200);
} catch (e) {
    caught = e.message != "";
}
parserCheck(caught, "eval-caught");
parserCheck(parserSideEffect == 0, "eval-no-side-effect");
parserCheck(Scripts.eval("20 + 22") == 42, "eval-recovered");

caught = false;
try {
    Scripts.execStorage("parser-storage-invalid.tjs");
} catch (e) {
    caught = e.message != "";
}
parserCheck(caught, "storage-caught");
parserCheck(parserSideEffect == 0, "storage-no-side-effect");
Scripts.execStorage("parser-storage-valid.tjs");
parserCheck(parserRecovered == 3, "storage-recovered");

Debug.message("PARSER_REPRO|END|" + parserFailures);
System.exit(parserFailures ? 1 : 0);
'''

SCRIPTS = {
    "startup.tjs": STARTUP,
    "parser-storage-invalid.tjs": (
        "// No statement in this file may execute.\n"
        "global.parserSideEffect = 4;\n"
        "var broken = ;\n"
    ),
    "parser-storage-valid.tjs": "global.parserRecovered += 1;\n",
}
EXPECTED = [
    "PARSER_REPRO|BEGIN",
    *[
        "PARSER_REPRO|PASS|" + name
        for name in (
            "exec-caught", "exec-no-side-effect", "exec-recovered",
            "abort-caught", "abort-no-side-effect", "abort-recovered",
            "eval-caught", "eval-no-side-effect", "eval-recovered",
            "storage-caught", "storage-no-side-effect", "storage-recovered",
        )
    ],
    "PARSER_REPRO|END|0",
]
DIAGNOSTICS = [
    "parser-exec-invalid.tjs(42)",
    "parser-abort-invalid.tjs(102)",
    "parser-eval-invalid.tjs(202)",
    "parser-storage-invalid.tjs(3)",
]


def prepare(directory):
    fixture = directory / "fixture"
    fixture.mkdir(parents=True, exist_ok=False)
    for name, text in SCRIPTS.items():
        (fixture / name).write_bytes(b"\xff\xfe" + text.encode("utf-16le"))
    return fixture


def run(engine, directory, timeout):
    fixture = prepare(directory)
    command = [str(engine), fixture.as_posix() + "/"]
    if sys.platform.startswith("linux") and not os.environ.get("DISPLAY"):
        xvfb = shutil.which("xvfb-run")
        if not xvfb:
            raise RuntimeError("Set DISPLAY or install xvfb-run for the real runtime")
        command = [xvfb, "-a", "-s", "-screen 0 1280x720x24", *command]
    env = os.environ.copy()
    env.setdefault("LIBGL_ALWAYS_SOFTWARE", "1")
    env.setdefault("ALSOFT_DRIVERS", "null")
    digest = hashlib.sha256()
    with engine.open("rb") as binary:
        for chunk in iter(lambda: binary.read(1024 * 1024), b""):
            digest.update(chunk)
    report = {"engine": str(engine), "engine_sha256": digest.hexdigest(),
              "command": command, "timeout_seconds": timeout, "forced_stop": False}
    log_path = directory / "parser-error-runtime.log"
    with log_path.open("wb") as log:
        process = subprocess.Popen(command, cwd=engine.parent, env=env,
                                   stdout=log, stderr=subprocess.STDOUT,
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
    failures = []
    if report["exit_code"] != 0 or report["forced_stop"]:
        failures.append(f"Runtime exited with {report['exit_code']}; forced_stop={report['forced_stop']}")
    if "PARSER_REPRO|FAIL|" in output:
        failures.append("The TJS fixture reported a failed assertion")
    for marker in EXPECTED:
        if output.count(marker) != 1:
            failures.append(f"Expected exactly one marker: {marker}")
    for diagnostic in DIAGNOSTICS:
        if diagnostic not in output:
            failures.append(f"Missing file/line diagnostic: {diagnostic}")
    report["failures"] = failures
    report["checks"] = 12
    report["diagnostics"] = DIAGNOSTICS
    (directory / "parser-error-report.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if failures:
        print(output[-12000:], file=sys.stderr)
        for failure in failures:
            print(failure, file=sys.stderr)
        return 1
    print("12 runtime parser error/recovery checks and 4 file/line diagnostics passed")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path)
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--timeout", type=float, default=45)
    parser.add_argument("--prepare-only", action="store_true",
                        help="write the device-ready fixture without launching an engine")
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    if args.prepare_only:
        if args.output_dir is None:
            parser.error("--prepare-only requires --output-dir")
        print(prepare(args.output_dir.resolve()))
        return 0
    if args.engine is None:
        parser.error("--engine is required")
    engine = args.engine.resolve(strict=True)
    if args.output_dir is not None:
        parent = args.output_dir.resolve()
        parent.mkdir(parents=True, exist_ok=True)
        directory = Path(tempfile.mkdtemp(prefix="parser-error-", dir=parent))
        print(f"Parser error evidence: {directory}", flush=True)
        return run(engine, directory, args.timeout)
    with tempfile.TemporaryDirectory(prefix="krkr2-parser-error-") as temporary:
        return run(engine, Path(temporary), args.timeout)


if __name__ == "__main__":
    raise SystemExit(main())
