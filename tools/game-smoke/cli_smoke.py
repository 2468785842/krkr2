"""Check CLI errors/help against the real runtime without creating game windows."""
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    sys.dont_write_bytecode = True
    engine = Path(sys.argv[1]).resolve(strict=True)
    fixture_module = Path(__file__).parents[1] / "xp3" / "smoke_test.py"
    spec = importlib.util.spec_from_file_location("xp3_smoke_fixture", fixture_module)
    fixture = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(fixture)
    with tempfile.TemporaryDirectory(prefix="krkr2-cli-") as directory:
        root = Path(directory)
        invalid = root / "invalid.xp3"
        invalid.write_bytes(b"not an archive")
        no_startup = root / "without-startup.xp3"
        no_startup.write_bytes(fixture.archive(False)[0])
        cases = [
            (["--help"], 0, "Usage: krkr2"),
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
        if "--require-game" in sys.argv[2:]:
            cases.append(([], 2, "requires --game PATH"))
        for args, expected_code, message in cases:
            result = subprocess.run([str(engine), *args], cwd=engine.parent,
                                    capture_output=True, encoding="utf-8", errors="replace", timeout=15)
            output = result.stdout + result.stderr
            if result.returncode != expected_code or message not in output:
                raise AssertionError((args, result.returncode, output))
            if "gl.renderer" in output or "Direct game startup" in output:
                raise AssertionError("Error/help invocation entered graphical startup")
    print(f"{len(cases)} runtime CLI help/error checks passed")


if __name__ == "__main__":
    main()
