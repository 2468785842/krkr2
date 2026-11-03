"""Run the SDL/TJS experiment and verify real renderer readback and event results."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def pixel(path, x, y):
    data = path.read_bytes()
    if data[:2] != b"BM":
        raise ValueError(f"Not a BMP: {path.name}")
    offset = struct.unpack_from("<I", data, 10)[0]
    width, height = struct.unpack_from("<ii", data, 18)
    bits = struct.unpack_from("<H", data, 28)[0]
    compression = struct.unpack_from("<I", data, 30)[0]
    if (width, abs(height)) != (640, 480) or bits not in (24, 32) or compression not in (0, 3):
        raise ValueError(f"Unexpected readback format: {width}x{height}, {bits} bits, compression {compression}")
    row = abs(height) - 1 - y if height > 0 else y
    stride = ((width * bits + 31) // 32) * 4
    start = offset + row * stride + x * (bits // 8)
    if compression == 3:
        if bits != 32:
            raise ValueError("Only 32-bit bitfields are supported")
        masks = struct.unpack_from("<III", data, 54)
        if masks != (0x00ff0000, 0x0000ff00, 0x000000ff):
            raise ValueError(f"Unexpected RGB masks: {masks}")
    blue, green, red = data[start:start + 3]
    return (red << 16) | (green << 8) | blue


def verify(directory):
    report = json.loads((directory / "report.json").read_text(encoding="utf-8"))
    expected = {"completed": True, "renderer": "software", "self_test": True, "frames": 4, "key_downs": 1,
                "key_ups": 1, "close_vetoes": 1, "width": 640, "height": 480}
    for name, value in expected.items():
        if report.get(name) != value:
            raise ValueError(f"{name}: {report.get(name)} != {value}")
    events = (directory / "events.txt").read_text(encoding="utf-8").splitlines()
    if events != ["key:65:1:1:0", "key:65:0:0:0", "close-accepted:0", "close-accepted:1"]:
        raise ValueError(f"Unexpected event sequence: {events}")
    samples = [("initial.bmp", 10, 10, 0x203040), ("initial.bmp", 100, 120, 0xd08030),
               ("pressed.bmp", 100, 120, 0x30c070), ("released.bmp", 100, 120, 0x203040),
               ("released.bmp", 200, 120, 0x30c070), ("veto.bmp", 10, 10, 0x403020)]
    for name, x, y, expected_color in samples:
        actual = pixel(directory / name, x, y)
        if actual != expected_color:
            raise ValueError(f"{name}({x},{y}): {actual:06x} != {expected_color:06x}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--script", type=Path, default=Path(__file__).with_name("fixture.tjs"))
    args = parser.parse_args()
    engine = args.engine.resolve(strict=True)
    script = args.script.resolve(strict=True)
    output = args.destination.resolve()
    if output.exists():
        parser.error("Destination must be new")
    output.parent.mkdir(parents=True, exist_ok=True)
    report = {"engine": str(engine), "engine_sha256": digest(engine), "script_sha256": digest(script),
              "passed": False, "forced_stop": False, "input_scope": "SDL_PushEvent, not OS keyboard"}
    try:
        result = subprocess.run([str(engine), str(script), str(output), "--self-test"],
                                cwd=engine.parent, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
        report["exit_code"] = result.returncode
        output.mkdir(exist_ok=True)
        (output / "process.log").write_bytes(result.stdout)
        if result.returncode != 0:
            raise RuntimeError(f"Prototype exited {result.returncode}")
        verify(output)
        if digest(engine) != report["engine_sha256"] or digest(script) != report["script_sha256"]:
            raise RuntimeError("Executable or script changed during verification")
        report["captures"] = {path.name: digest(path) for path in sorted(output.glob("*.bmp"))}
        report["passed"] = True
    except subprocess.TimeoutExpired as error:
        report["forced_stop"] = True
        report["failure"] = str(error)
    except (OSError, RuntimeError, ValueError, struct.error) as error:
        report["failure"] = str(error)
    output.mkdir(exist_ok=True)
    (output / "verification.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
