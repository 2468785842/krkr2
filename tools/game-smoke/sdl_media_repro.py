"""Play generated KAG media through SDL3 and measure window pixels and mixed PCM."""

import argparse
from array import array
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import time
import wave

from interaction_smoke import read_events
from sdl_desktop_repro import (
    FIXTURES, X11, activate, capture, click, client_window, desktop, digest, event,
    geometry, launch, stage_kag, wait_for,
)


def generate_media(game):
    for name, hz, duration in (("harbor.wav", 440, 2), ("bell.wav", 660, 1)):
        subprocess.run(["ffmpeg", "-nostdin", "-loglevel", "error", "-f", "lavfi",
                        "-i", f"sine=frequency={hz}:sample_rate=48000:duration={duration}",
                        str(game / name)], check=True, timeout=30)
    for name, duration in (("arrival.mp4", 3), ("departure.mp4", 20)):
        subprocess.run([
            "ffmpeg", "-nostdin", "-loglevel", "error",
            "-f", "lavfi", "-i", f"color=red:size=96x64:rate=24:duration={duration}",
            "-f", "lavfi", "-i", f"sine=frequency=880:sample_rate=48000:duration={duration}",
            "-vf", "drawbox=x=48:y=0:w=48:h=64:color=lime:t=fill",
            "-c:v", "mpeg4", "-pix_fmt", "yuv420p", "-c:a", "aac", str(game / name),
        ], check=True, timeout=30)


def measure_audio(path):
    samples = array("h")
    with wave.open(str(path), "rb") as capture_file:
        assert (capture_file.getframerate(), capture_file.getnchannels(),
                capture_file.getsampwidth()) == (48000, 1, 2)
        samples.frombytes(capture_file.readframes(capture_file.getnframes()))
    results = {}
    # These tone frequencies complete an integer number of cycles in 100 ms.
    block = 4800
    assert len(samples) > block, "No PCM from audio output"
    for hz in (440, 660, 880):
        step = 2 * math.pi * hz / 48000
        cosines = [math.cos(step * index) for index in range(block)]
        sines = [math.sin(step * index) for index in range(block)]
        peak = 0
        for start in range(0, len(samples) - block, block):
            chunk = samples[start:start + block]
            power = math.hypot(sum(a * b for a, b in zip(chunk, cosines)),
                               sum(a * b for a, b in zip(chunk, sines)))
            peak = max(peak, 2 * power / block)
        assert peak > 80, f"No audible {hz} Hz test tone in captured PCM (amplitude {peak:.1f})"
        results[hz] = round(peak, 1)
    return results


def play(x11, output, game, command_line, env):
    with launch(command_line, output / "process.log", env=env) as process:
        event(game, process, "ready")
        window = wait_for("KAG media window", lambda: client_window(process.pid), process)
        activate(window)

        def movie_frame(name, playing):
            def matches():
                image = capture(x11, window, geometry(x11, window))
                pixels = [image.getpixel(point) for point in ((248, 240), (392, 240))]
                expected = ((255, 0, 0), (0, 255, 0)) if playing else ((0, 0, 0),) * 2
                return image if all(abs(a - b) <= 15 for pixel, color in zip(pixels, expected)
                                    for a, b in zip(pixel, color)) else None
            image = wait_for(name, matches, process)
            image.save(output / f"{name}.png")

        def choice():
            def ready():
                image = capture(x11, window, geometry(x11, window))
                return sum(image.crop((40, 60, 500, 100)).convert("L").histogram()[192:]) > 100
            wait_for("visible KAG menu choice", ready, process)
            x11.motion(window, 300, 110)
            time.sleep(0.08)
            click(x11, window, 90, 80)

        event(game, process, "natural:start")
        movie_frame("natural-playing", True)
        event(game, process, "natural:end")
        movie_frame("natural-cleared", False)
        choice()
        event(game, process, "skip:start")
        movie_frame("skip-playing", True)
        skip_start = time.monotonic()
        click(x11, window, 480, 340)
        event(game, process, "skip:end")
        elapsed = time.monotonic() - skip_start
        assert elapsed < 5, elapsed
        movie_frame("skip-cleared", False)
        choice()
        assert process.wait(timeout=8) == 0
        assert read_events(game) == ["ready", "natural:start", "natural:end", "skip:start",
                                     "skip:end", "exit"]
        return elapsed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("kag_framework", type=Path)
    args = parser.parse_args()
    engine = args.engine.resolve(strict=True)
    output = args.output.resolve()
    game = output / "game"
    game.mkdir(parents=True)
    runtime = output / "runtime"
    runtime.mkdir(mode=0o700)
    stage_kag(game, args.kag_framework)
    fixture = FIXTURES / "media.ks"
    shutil.copy2(fixture, game / "scenario" / "first.ks")
    generate_media(game)
    command_line = [str(engine), "--host=sdl3", "--game", str(game)]
    report = {"command": command_line, "engine_sha256": digest(engine), "fixture_sha256": digest(fixture),
              "media_sha256": {path.name: digest(path) for path in sorted(game.iterdir())
                               if path.suffix in (".mp4", ".wav")}}
    env = dict(os.environ, XDG_RUNTIME_DIR=str(runtime), XDG_CONFIG_HOME=str(output / "config"),
               XDG_CACHE_HOME=str(output / "cache"), LC_ALL="C.UTF-8", SDL_IM_MODULE="none")
    config = output / "alsoft.conf"
    recording = output / "audio.wav"
    config.write_text(f"channels=mono\nfrequency=48000\nsample-type=int16\n[wave]\nfile={recording}\n",
                      encoding="utf-8")
    env.update(ALSOFT_DRIVERS="wave", ALSOFT_CONF=str(config))
    x11 = X11()
    try:
        with desktop("x11", x11, output, env) as (_, engine_env):
            report["skip_seconds"] = round(play(x11, output, game, command_line, engine_env), 3)
        report["audio_tone_amplitudes"] = measure_audio(recording)
        report.update(passed=True, forced_stop=False, exit_code=0)
    except Exception as error:
        report.update(passed=False, error=str(error))
        raise
    finally:
        x11.close()
        report["events"] = read_events(game)
        (output / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
