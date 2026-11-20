"""Run real software/OpenGL Layer operations in isolated self-authored scenes."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import struct
import time
from frame_pixels import color_counts


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def events(root):
    path = root / "imageops-events.txt"
    if not path.exists():
        return []
    try:
        raw = path.read_bytes()
        return raw.decode("utf-16" if raw.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig").splitlines()
    except (OSError, UnicodeError):
        return []


def bmp_rgb(path, x, y, expected_size=(384, 192)):
    data = path.read_bytes()
    if data[:2] != b"BM" or len(data) < 54:
        raise ValueError("Invalid BMP header")
    offset = struct.unpack_from("<I", data, 10)[0]
    header = struct.unpack_from("<I", data, 14)[0]
    width, height, planes, bits, compression = struct.unpack_from("<iiHHI", data, 18)
    if header < 40 or planes != 1 or bits not in (24, 32) or compression not in (0, 3):
        raise ValueError("Unsupported BMP pixel layout")
    if compression == 3 and (len(data) < 66 or struct.unpack_from("<III", data, 54) != (0xff0000, 0xff00, 0xff)):
        raise ValueError("Unsupported BMP masks")
    if (width, abs(height)) != expected_size or not (0 <= x < width and 0 <= y < abs(height)):
        raise ValueError("Unexpected BMP dimensions or sample")
    stride = ((width * bits + 31) // 32) * 4
    if offset < 14 + header or len(data) < offset + stride * abs(height):
        raise ValueError("Truncated BMP pixels")
    row = abs(height) - 1 - y if height > 0 else y
    start = offset + row * stride + x * (bits // 8)
    b, g, r = data[start:start+3]
    return (r, g, b)


def frame_failures(path, renderer="opengl"):
    expected = [(64, 160, 64), (160, 64, 160), (64, 64, 64), (160, 160, 160)]
    panels = [(i*96, 0, [(24,16),(72,16),(24,48),(72,48)]) for i in range(4)]
    panels += [(0,96,[(16,16),(48,16),(16,48),(48,48)]),
               (96,96,[(27,27),(63,29),(29,63),(65,65)]),
               (192,96,[(20,20),(65,22),(22,65),(61,61)])]
    errors = []
    for panel, (left, top, points) in enumerate(panels):
        for i, (x, y) in enumerate(points):
            actual = bmp_rgb(path, left+x, top+y)
            color = expected[i]
            if panel == 6 and renderer == "software":
                color = tuple(channel*255//256 for channel in color)
            if actual != color:
                errors.append(f"Panel {panel} sample {i}: RGB {actual} != {color}")
    if bmp_rgb(path, 350, 150) != (16, 32, 16):
        errors.append("Background guard pixel differs")
    return errors


def capture_frame(root, renderer, sequence, deadline, process, palette, file_name):
    # The Cocos frame includes the software compositor's existing alpha/256
    # conversion. Keep exact colors rather than a pixel tolerance.
    if renderer == "software":
        palette = [tuple(c*255//256 for c in color) for color in palette]
    (root / "ui-action.txt").write_text(f"{sequence} capture\n", encoding="utf-8")
    while time.monotonic() < deadline:
        path = root / "ui-driver-events.txt"
        log = path.read_text(encoding="utf-8") if path.exists() else ""
        if " error:" in log:
            raise RuntimeError(log)
        if f"{sequence} capture-saved" in log and (root / "ui-frame.png").exists():
            if f"{sequence} renderer:{renderer}" not in log:
                raise RuntimeError("Capture renderer does not match requested backend: " + log)
            frame = root / file_name
            shutil.copy2(root / "ui-frame.png", frame)
            counts = color_counts(frame, palette)
            if min(counts.values()) < 16:
                raise RuntimeError("Cocos frame does not contain the rendered fixture palette: " + str(counts))
            return {"renderer_observed":renderer, "frame_file":file_name,
                    "frame_color_counts":{str(color):count for color,count in counts.items()},
                    "frame_sha256":digest(frame)}
        if process.poll() is not None:
            raise RuntimeError("Engine exited before capture")
        time.sleep(.1)
    raise TimeoutError("No completed Cocos frame capture")


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
        command = [str(engine), "--game=" + str(root), "-imageops-renderer=" + renderer]
        if capture:
            command.append("-ui-contract-driver=yes")
        report = {"engine": str(engine), "engine_sha256": digest(engine), "script_sha256": digest(root / "startup.tjs"),
                  "controller_sha256": digest(Path(__file__)),
                  "preferences_sha256": digest(root / "Kirikiroid2Preference.xml"),
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
                    palette = [(64,160,64),(160,64,160),(64,64,64),(160,160,160)]
                    report.update(capture_frame(root, renderer, 1, deadline, process, palette, "ui-imageops.png"))
                    phases = []
                    for step, (width, height, color) in enumerate([
                        (65,33,(96,176,96)), (129,65,(176,96,176)),
                        (65,33,(48,96,48)), (65,33,(160,192,160))], 1):
                        (root / f"presentation-{step}.txt").write_text("present\n", encoding="utf-8")
                        while time.monotonic() < deadline:
                            current = events(root)
                            if any(e.startswith("error:") for e in current):
                                raise RuntimeError(str(current))
                            if current.count(f"presented:{step}") == 1:
                                break
                            if process.poll() is not None:
                                raise RuntimeError("Engine exited during presentation replacement")
                            time.sleep(.1)
                        else:
                            raise TimeoutError(f"No presentation event {step}")
                        phase = {"step":step, "width":width, "height":height}
                        phase.update(capture_frame(root, renderer, step+1, deadline, process, [color], f"ui-presentation-{step}.png"))
                        bitmap = root / f"presentation-{step}.bmp"
                        for x,y in [(0,0),(width//2,height//2),(width-1,height-1)]:
                            if bmp_rgb(bitmap,x,y,(width,height)) != color:
                                raise RuntimeError(f"Presentation {step} BMP sample differs")
                        phase["image_sha256"] = digest(bitmap)
                        phases.append(phase)
                    report["presentation_phases"] = phases
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
        if capture:
            required += [f"presented:{i}" for i in range(1,5)]
        report["failures"] += ["Missing or repeated event: " + e for e in required if report["events"].count(e) != 1]
        report["failures"] += [e for e in report["events"] if e.startswith("error:")]
        if report["forced_stop"] or report["exit_code"] != 0:
            report["failures"].append("Engine did not exit normally")
        image = root / "imageops.bmp"
        if not image.exists() or image.stat().st_size < 384*192*3:
            report["failures"].append("Missing or incomplete Layer image")
        else:
            report["image_sha256"] = digest(image)
            try:
                report["failures"].extend(frame_failures(image, renderer))
            except (ValueError, OSError, struct.error) as error:
                report["failures"].append("Layer BMP validation: " + str(error))
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
