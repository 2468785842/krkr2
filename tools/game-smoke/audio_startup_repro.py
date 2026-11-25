"""Measure real first-play PCM while delaying native worker startup on Linux."""
import argparse
from array import array
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import tempfile
import wave


SCRIPT = '''global.window = new Window();
window.visible = true;
global.sound = new WaveSoundBuffer(global);
sound.open(System.exePath + "tone.wav");
sound.looping = true;
sound.play();
global.timer = new Timer(function() {
    timer.enabled = false;
    var result = ["audio-finished"];
    result.save(System.exePath + "result.txt");
    System.exit();
}, "");
timer.interval = 1200;
timer.enabled = true;
'''


def run(engine, delay_library, parent):
    parent.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="first-buffer-", dir=parent))
    (root / "startup.tjs").write_text(SCRIPT, encoding="utf-8")
    with wave.open(str(root / "tone.wav"), "wb") as tone:
        tone.setparams((1, 2, 48000, 0, "NONE", "not compressed"))
        pcm = array("h", (int(4000 * math.sin(2 * math.pi * 440 * i / 48000)) for i in range(96000)))
        tone.writeframes(pcm.tobytes())
    recording = root / "audio.wav"
    config = root / "alsoft.conf"
    config.write_text(f"channels=mono\nfrequency=48000\nsample-type=int16\n[wave]\nfile={recording}\n", encoding="utf-8")
    env = dict(os.environ, ALSOFT_DRIVERS="wave", ALSOFT_CONF=str(config),
               XDG_CONFIG_HOME=str(root / "config"))
    env["LD_PRELOAD"] = str(delay_library) + (":" + env["LD_PRELOAD"] if env.get("LD_PRELOAD") else "")
    report = {"engine_sha256": hashlib.sha256(engine.read_bytes()).hexdigest(),
              "worker_delay_ms": 150, "forced_stop": False, "passed": False}
    with (root / "process.log").open("wb") as log:
        process = subprocess.Popen([str(engine), "--host=sdl3", "--game", str(root)],
                                   env=env, cwd=engine.parent, stdout=log, stderr=subprocess.STDOUT)
        try:
            report["exit_code"] = process.wait(timeout=20)
        except subprocess.TimeoutExpired:
            process.kill()
            report.update(exit_code=process.wait(), forced_stop=True)
    try:
        assert report["exit_code"] == 0 and not report["forced_stop"], "Engine did not exit normally"
        assert (root / "result.txt").read_text(encoding="utf-16").splitlines() == ["audio-finished"]
        with wave.open(str(recording), "rb") as capture:
            assert (capture.getframerate(), capture.getnchannels(), capture.getsampwidth()) == (48000, 1, 2)
            samples = array("h", capture.readframes(recording.stat().st_size // 2))
        block = 4800
        cosine = [math.cos(2 * math.pi * 440 * i / 48000) for i in range(block)]
        sine = [math.sin(2 * math.pi * 440 * i / 48000) for i in range(block)]
        peaks = [2 * math.hypot(sum(a * b for a, b in zip(samples[start:start + block], cosine)),
                                sum(a * b for a, b in zip(samples[start:start + block], sine))) / block
                 for start in range(0, len(samples) - block, block)]
        report["amplitude_440_hz"] = round(max(peaks, default=0), 1)
        assert report["amplitude_440_hz"] > 1000, "First audio buffer was not played"
        report["passed"] = True
    except Exception as error:
        report["error"] = str(error)
    (root / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(report, evidence=str(root)), indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--delay-library", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    raise SystemExit(run(args.engine.resolve(strict=True), args.delay_library.resolve(strict=True), args.output_dir.resolve()))
