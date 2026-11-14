"""Exercise installed module evidence, including platform library names and stale files."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location(
    "check_opencv_modules", Path(__file__).resolve().parents[1] / "check_opencv_modules.py")
CHECK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECK)


class OpenCVModulesTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.prefix = self.root / "x64-test"
        self.write("share/opencv4/OpenCVModules.cmake", "\n".join(
            f"add_library(opencv_{m} STATIC IMPORTED)" for m in ("core", "imgproc")))
        self.write("include/opencv2/opencv_modules.hpp",
                   "#define HAVE_OPENCV_CORE\n#define HAVE_OPENCV_IMGPROC\n")
        self.write("include/opencv2/core/version.hpp",
                   "#define CV_VERSION_MAJOR 4\n#define CV_VERSION_MINOR 7\n#define CV_VERSION_REVISION 0\n")
        status = self.root / "vcpkg/status"
        status.parent.mkdir()
        status.write_text("Package: opencv4\nVersion: 4.7.0\nPort-Version: 6\n"
                          "Architecture: x64-test\nStatus: install ok installed\n\n"
                          "Package: opencv4\nFeature: imageops\nArchitecture: x64-test\n"
                          "Status: install ok installed\n", encoding="utf-8")

    def write(self, name, contents):
        path = self.prefix / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(contents, encoding="utf-8")

    def test_static_and_versioned_platform_libraries(self):
        for suffix in (".a", "470.lib", "470d.dll", ".so.4.7.0", ".4.7.0.dylib"):
            with self.subTest(suffix=suffix):
                for module in ("core", "imgproc"):
                    self.write(f"lib/libopencv_{module}{suffix}", "data")
                report = CHECK.inspect(self.prefix)
                self.assertEqual(report["modules"], ["opencv_core", "opencv_imgproc"])
        self.assertEqual(report["library_bytes"], 40)

    def test_missing_library_is_rejected(self):
        self.write("lib/libopencv_core.a", "data")
        with self.assertRaisesRegex(ValueError, "Missing installed"):
            CHECK.inspect(self.prefix)

    def test_stale_unexported_library_is_rejected(self):
        self.write("lib/libopencv_video.a", "data")
        with self.assertRaisesRegex(ValueError, "Unexported"):
            CHECK.inspect(self.prefix)

    def test_header_export_disagreement_is_rejected(self):
        self.write("include/opencv2/opencv_modules.hpp", "#define HAVE_OPENCV_CORE\n")
        with self.assertRaisesRegex(ValueError, "export/header"):
            CHECK.inspect(self.prefix)

    def test_uninstalled_feature_is_rejected(self):
        status = self.root / "vcpkg/status"
        status.write_text(status.read_text().replace("install ok installed", "deinstall ok config-files"))
        with self.assertRaisesRegex(ValueError, "not fully installed"):
            CHECK.inspect(self.prefix)

    def test_wrong_port_version_is_rejected(self):
        status = self.root / "vcpkg/status"
        status.write_text(status.read_text().replace("Port-Version: 6", "Port-Version: 7"))
        with self.assertRaisesRegex(ValueError, "port identity"):
            CHECK.inspect(self.prefix)


if __name__ == "__main__":
    unittest.main()
