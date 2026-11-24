"""Check CLI errors/help against the real runtime without creating game windows."""
import argparse
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    sys.dont_write_bytecode = True
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine", type=Path)
    parser.add_argument("--host", choices=("cocos", "sdl3"))
    options = parser.parse_args()
    engine = options.engine.resolve(strict=True)
    host_args = [f"--host={options.host}"] if options.host else []
    fixture_module = Path(__file__).parents[1] / "xp3" / "smoke_test.py"
    spec = importlib.util.spec_from_file_location("xp3_smoke_fixture", fixture_module)
    fixture = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(fixture)
    with tempfile.TemporaryDirectory(prefix="krkr2-cli-") as directory:
        root = Path(directory)
        env = dict(os.environ)
        if options.host == "sdl3":
            # Exercise native storage before XP3 validation without writing to
            # the developer's normal preference directory.
            env["XDG_CONFIG_HOME"] = str(root / "config")
        invalid = root / "invalid.xp3"
        invalid.write_bytes(b"not an archive")
        no_startup = root / "without-startup.xp3"
        no_startup.write_bytes(fixture.archive(False)[0])
        cases = [
            ([], 2, "requires --game PATH"),
            (["--help"], 0, "Usage: krkr2"),
            (["--list-settings"], 0, "renderer\tsoftware|opengl"),
            (["--set"], 2, "Expected --set KEY=VALUE"),
            (["--set", "unknown=1"], 2, "Unknown desktop setting"),
            (["--set=fps_limit=0"], 2, "Invalid value for fps_limit"),
            (["--game"], 2, "Missing game path"),
            (["--game", str(root / "missing")], 2, "not a regular file"),
            (["--game", str(root)], 2, "needs startup.tjs or data.xp3"),
            (["--game", str(invalid)], 2, "No startup.tjs"),
            (["--game", str(no_startup)], 2, "No startup.tjs"),
            ([str(no_startup)], 2, "No startup.tjs"),
            (["--unknown"], 2, "Unknown launch option"),
            (["--game", "a", "--game", "b"], 2, "exactly one game"),
            (["--config", "future.json"], 2, "Unknown launch option"),
        ]
        for args, expected_code, message in cases:
            result = subprocess.run([str(engine), *host_args, *args], cwd=engine.parent, env=env,
                                    capture_output=True, encoding="utf-8", errors="replace", timeout=15)
            output = result.stdout + result.stderr
            if result.returncode != expected_code or message not in output:
                raise AssertionError((args, result.returncode, output))
            if "gl.renderer" in output or "Direct game startup" in output:
                raise AssertionError("Error/help invocation entered graphical startup")
            if options.host == "sdl3" and expected_code == 0 and (root / "config").exists():
                raise AssertionError("SDL3 help/settings invocation created a preference directory")
    host_label = f" ({options.host})" if options.host else ""
    print(f"{len(cases)} runtime CLI help/error checks passed{host_label}")


if __name__ == "__main__":
    main()
