"""Exercise Cocos presentation of a generated YUV420P clip from TJS."""
import argparse
import shutil
import subprocess
import time
from pathlib import Path

from frame_pixels import rgb_rows


def wait_for(path, marker, process, deadline):
    while time.monotonic() < deadline:
        if path.exists():
            lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
            if any("error:" in line for line in lines):
                raise RuntimeError(f"{path.name}: {lines}")
            if marker in lines:
                return
        if process.poll() is not None:
            raise RuntimeError(f"Engine exited with {process.returncode} before {marker}")
        time.sleep(0.05)
    raise TimeoutError(f"{path.name}: {marker}")


def sample_frame(path):
    points = ((0.10, 0.5), (0.375, 0.5), (0.625, 0.5), (0.90, 0.5))
    samples = [None] * len(points)
    for y, (width, height, channels, row) in enumerate(rgb_rows(path)):
        for i, (x_fraction, y_fraction) in enumerate(points):
            if y == int(y_fraction * height):
                start = int(x_fraction * width) * channels
                samples[i] = tuple(row[start:start + 3])
        if all(pixel is not None for pixel in samples):
            return samples
    raise ValueError(f"No sample row in {path}")


def near(pixel, color):
    return all(abs(actual - expected) <= 12 for actual, expected in zip(pixel, color))


def run(engine, root):
    root.mkdir(parents=True)
    shutil.copy2(Path(__file__).with_name("video") / "startup.tjs", root / "startup.tjs")
    (root / "interaction-fixture.json").write_text('{"fixture":"video"}', encoding="utf-8")
    subprocess.run([
        "ffmpeg", "-hide_banner", "-loglevel", "error",
        "-f", "lavfi", "-i", "color=red:size=96x64:rate=24:duration=12",
        "-vf", "drawbox=x=48:y=0:w=48:h=64:color=lime:t=fill",
        "-c:v", "mpeg4", "-pix_fmt", "yuv420p", str(root / "colors.mp4"),
    ], check=True, timeout=30)

    command = [str(engine), f"--game={root}", "-ui-contract-driver=yes"]
    events = root / "video-events.txt"
    driver = root / "ui-driver-events.txt"
    capture = root / "ui-frame.png"
    sequence = 0
    with (root / "process.log").open("wb") as log:
        process = subprocess.Popen(command, cwd=engine.parent, stdout=log, stderr=subprocess.STDOUT)
        try:
            wait_for(events, "ready", process, time.monotonic() + 25)
            for phase in (1, 2, 3):
                deadline = time.monotonic() + 8
                (root / f"video-{phase}.txt").touch()
                wait_for(events, f"phase:{phase}", process, deadline)
                background = (16, 32, 48)
                expected = [background, (255, 0, 0), (0, 255, 0), background]
                if phase == 2:
                    expected = [background] * 4
                while True:
                    sequence += 1
                    capture.unlink(missing_ok=True)
                    request = root / "ui-action.next"
                    request.write_text(f"{sequence} capture\n", encoding="utf-8")
                    request.replace(root / "ui-action.txt")
                    wait_for(driver, f"{sequence} capture-saved", process, deadline)
                    samples = sample_frame(capture)
                    if all(near(pixel, color) for pixel, color in zip(samples, expected)):
                        capture.rename(root / f"phase-{phase}.png")
                        print(f"Video phase {phase}: {samples}", flush=True)
                        break
                    if time.monotonic() >= deadline:
                        raise AssertionError(f"Video phase {phase}: {samples}, expected {expected}")
            (root / "video-4.txt").touch()
            if process.wait(timeout=8) != 0:
                raise RuntimeError(f"Engine exited with {process.returncode}")
            wait_for(events, "phase:4", process, time.monotonic() + 1)
            print("Video fixture exited normally.", flush=True)
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    run(args.engine.resolve(strict=True), args.destination.resolve())
