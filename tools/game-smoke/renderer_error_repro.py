"""Check renderer construction failure and shutdown using a synthetic TJS scene."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile


STARTUP = r'''global.results = [];
try {
    global.window = new Window();
    global.layer = new Layer(window, null);
    results.add("unexpected-success");
} catch(e) {
    results.add("caught:" + e.message);
}
results.save(System.exePath + "renderer-results.txt");
'''

AFTER_STARTUP = r'''results.add("exit-requested");
results.save(System.exePath + "renderer-results.txt");
System.exit();
'''



def run(engine, parent, timeout):
    parent.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="renderer-error-", dir=parent))
    script = root / "startup.tjs"
    script.write_text(STARTUP, encoding="utf-8")
    after_startup = root / "AfterStartup.tjs"
    after_startup.write_text(AFTER_STARTUP, encoding="utf-8")
    preference = root / "Kirikiroid2Preference.xml"
    preference.write_text(
        '<GlobalPreference><Item key="renderer" value="__renderer_error_fixture__"/>'
        '</GlobalPreference>', encoding="utf-8")
    command = [str(engine), "--game=" + str(root), "--set=showfps=0"]
    report = {
        "engine_sha256": hashlib.sha256(engine.read_bytes()).hexdigest(),
        "script_sha256": hashlib.sha256(script.read_bytes()).hexdigest(),
        "after_startup_sha256": hashlib.sha256(after_startup.read_bytes()).hexdigest(),
        "preferences_sha256": hashlib.sha256(preference.read_bytes()).hexdigest(),
        "command": command, "forced_stop": False,
    }
    with (root / "process.log").open("wb") as output:
        process = subprocess.Popen(command, cwd=engine.parent, stdout=output,
                                   stderr=subprocess.STDOUT)
        try:
            report["exit_code"] = process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
            report.update(exit_code=process.returncode, forced_stop=True)
    events_path = root / "renderer-results.txt"
    raw = events_path.read_bytes() if events_path.exists() else b""
    encoding = "utf-16" if raw.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig"
    report["events"] = raw.decode(encoding, errors="replace").splitlines()
    report["passed"] = (
        report["exit_code"] == 0 and not report["forced_stop"] and
        report["events"] == ["caught:unsupported renderer __renderer_error_fixture__",
                             "exit-requested"])
    (root / "report.json").write_text(
        json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Renderer error evidence: {root}", flush=True)
    print(json.dumps(report, ensure_ascii=False))
    return 0 if report["passed"] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, default=Path("out/renderer-error"))
    parser.add_argument("--timeout", type=float, default=45)
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    return run(args.engine.resolve(strict=True), args.output_dir.resolve(), args.timeout)


if __name__ == "__main__":
    raise SystemExit(main())
