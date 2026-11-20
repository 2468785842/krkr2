"""Run real software/OpenGL Layer operations in isolated self-authored scenes."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def events(root):
    path = root / "imageops-events.txt"
    if not path.exists():
        return []
    raw = path.read_bytes()
    return raw.decode("utf-16" if raw.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig").splitlines()


def run(engine, destination, seconds=60, capture=False):
    engine = engine.resolve()
    destination.mkdir(parents=True, exist_ok=False)
    reports = []
    for renderer in ("software", "opengl"):
        root = (destination / renderer).resolve()
        root.mkdir()
        source = Path(__file__).parent / "imageops/startup.tjs"
        shutil.copy2(source, root / "startup.tjs")
        (root / "Kirikiroid2Preference.xml").write_text(
            f'<GlobalPreference><Item key="renderer" value="{renderer}"/></GlobalPreference>', encoding="utf-8")
        if capture:
            (root / "interaction-fixture.json").write_text('{"fixture":"imageops"}')
        command = [str(engine), "--game=" + str(root)]
        if capture:
            command.append("-ui-contract-driver=yes")
        report = {"engine": str(engine), "engine_sha256": digest(engine), "script_sha256": digest(source),
                  "renderer_requested": renderer, "command": command, "forced_stop": False, "failures": []}
        deadline = time.monotonic() + seconds
        with (root / "process.log").open("wb") as output:
            process = subprocess.Popen(command, cwd=engine.parent, stdout=output, stderr=subprocess.STDOUT)
            try:
                while time.monotonic() < deadline:
                    current = events(root)
                    if any(e.startswith("error:") for e in current):
                        raise RuntimeError(str(current))
                    if "ready" in current:
                        break
                    if process.poll() is not None:
                        raise RuntimeError("Engine exited before ready")
                    time.sleep(.1)
                else:
                    raise TimeoutError("No fixture ready event")
                if capture:
                    # Fresh sequence; capture completion is required, not a fixed sleep.
                    (root / "ui-action.txt").write_text("1 capture\n", encoding="utf-8")
                    while time.monotonic() < deadline:
                        path = root / "ui-driver-events.txt"
                        log = path.read_text(encoding="utf-8") if path.exists() else ""
                        if " error:" in log:
                            raise RuntimeError(log)
                        if "1 capture-saved" in log and (root / "ui-frame.png").exists():
                            break
                        if process.poll() is not None:
                            raise RuntimeError("Engine exited before capture")
                        time.sleep(.1)
                    else:
                        raise TimeoutError("No completed Cocos frame capture")
                (root / "imageops-close.txt").write_text("close\n", encoding="utf-8")
                process.wait(timeout=max(1, deadline-time.monotonic()))
            except Exception as error:
                report["failures"].append(f"{type(error).__name__}: {error}")
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
        report["events"] = events(root)
        required = ["passed:resize-" + str(i) for i in range(4)] + ["passed:blur", "passed:affine", "passed:perspective", "ready", "finished"]
        report["failures"] += ["Missing or repeated event: " + e for e in required if report["events"].count(e) != 1]
        report["failures"] += [e for e in report["events"] if e.startswith("error:")]
        if report["forced_stop"] or report["exit_code"] != 0:
            report["failures"].append("Engine did not exit normally")
        image = root / "imageops.bmp"
        if not image.exists() or image.stat().st_size < 384*192*3:
            report["failures"].append("Missing or incomplete Layer image")
        else:
            report["image_sha256"] = digest(image)
        report["passed"] = not report["failures"]
        (root / "report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
        reports.append(report)
    (destination / "report.json").write_text(json.dumps(reports, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(reports, ensure_ascii=False, indent=2))
    return 0 if all(r["passed"] for r in reports) else 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--capture", action="store_true", help="Require the opt-in Cocos UI test driver")
    args = parser.parse_args()
    raise SystemExit(run(args.engine, args.destination, capture=args.capture))
