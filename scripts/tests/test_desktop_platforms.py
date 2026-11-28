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
        cases = (
            ("IOS=ON", "iOS is temporarily disabled"),
            ("CMAKE_SYSTEM_NAME=iOS", "iOS is temporarily disabled"),
            ("CMAKE_TOOLCHAIN_FILE=missing-ios-toolchain.cmake", "iOS is temporarily disabled"),
            ("ANDROID=ON", "Android requires ANDROID_ABI"),
            ("CMAKE_SYSTEM_NAME=Android", "Android requires ANDROID_ABI"),
            ("VCPKG_TARGET_TRIPLET=arm64-android", "Android requires ANDROID_ABI"),
            ("CMAKE_TOOLCHAIN_FILE=android.toolchain.cmake", "Android requires ANDROID_ABI"),
        )
        for target, diagnostic in cases:
            with self.subTest(target=target), tempfile.TemporaryDirectory(dir=output) as build:
                result = subprocess.run(
                    ["cmake", "-S", str(root), "-B", build, "-D" + target],
                    capture_output=True, text=True, timeout=30)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(diagnostic, result.stdout + result.stderr)
                self.assertNotIn("Running vcpkg install", result.stdout + result.stderr)
                self.assertFalse((Path(build) / "vcpkg_installed").exists())


if __name__ == "__main__":
    unittest.main()
