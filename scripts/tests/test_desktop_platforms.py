"""Verify disabled platforms fail before toolchain or dependency installation."""
from pathlib import Path
import subprocess
import tempfile
import unittest


class DesktopPlatformTest(unittest.TestCase):
    def test_unsupported_targets_are_rejected_before_project(self):
        root = Path(__file__).resolve().parents[2]
        output = root / "out/platform-rejection"
        output.mkdir(parents=True, exist_ok=True)
        targets = ("ANDROID=ON", "MACOS=ON", "IOS=ON",
                   "CMAKE_SYSTEM_NAME=Android", "CMAKE_SYSTEM_NAME=Darwin",
                   "VCPKG_TARGET_TRIPLET=arm64-android",
                   "VCPKG_TARGET_TRIPLET=arm64-osx",
                   "CMAKE_TOOLCHAIN_FILE=missing-android-toolchain.cmake")
        for target in targets:
            with self.subTest(target=target), tempfile.TemporaryDirectory(dir=output) as build:
                result = subprocess.run(
                    ["cmake", "-S", str(root), "-B", build, "-D" + target],
                    capture_output=True, text=True, timeout=30)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("temporarily disabled", result.stdout + result.stderr)
                self.assertNotIn("Running vcpkg install", result.stdout + result.stderr)
                self.assertFalse((Path(build) / "vcpkg_installed").exists())


if __name__ == "__main__":
    unittest.main()
