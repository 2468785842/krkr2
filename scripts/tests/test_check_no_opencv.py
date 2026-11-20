import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("check_no_opencv", Path(__file__).resolve().parents[1] / "check_no_opencv.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class NoOpenCVTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / "compile_commands.json").write_text(json.dumps([{"file": "ImageOps_native.cpp", "command": "c++ -c ImageOps_native.cpp"}]))
        (self.root / "build.ninja").write_text("build imageops_tests: CXX_LINK libcore_imageops.a\n")

    def test_standard_build_and_unrelated_ports(self):
        self.assertFalse(module.inspect(self.root)["opencv"])
        status = self.root / "vcpkg_installed/vcpkg/status"
        status.parent.mkdir(parents=True)
        status.write_text("Package: catch2\n")
        self.assertEqual(module.inspect(self.root)["ports"], ["catch2"])
        status.write_text("Package: opencv4\n")
        with self.assertRaisesRegex(ValueError, "installed"):
            module.inspect(self.root)

    def test_link_only_and_header_only_dependencies_are_rejected(self):
        (self.root / "build.ninja").write_text("LINK_LIBRARIES = libopencv_core.a")
        with self.assertRaisesRegex(ValueError, "link metadata"):
            module.inspect(self.root)
        (self.root / "build.ninja").write_text("")
        (self.root / "compile_commands.json").write_text(json.dumps([{"file": "ImageOps_native.cpp", "command": "c++ -I/install/include/opencv4 -c ImageOps_native.cpp"}]))
        with self.assertRaisesRegex(ValueError, "reference dependency"):
            module.inspect(self.root)

    def test_checkout_name_does_not_count_as_a_dependency(self):
        (self.root / "compile_commands.json").write_text(json.dumps([{"file": "/work/remove-opencv/ImageOps.cpp", "command": "c++ -I/work/remove-opencv/cpp -c /work/remove-opencv/ImageOps.cpp"}]))
        (self.root / "build.ninja").write_text("build /work/remove-opencv/imageops_tests: CXX_LINK libcore_imageops.a")
        self.assertFalse(module.inspect(self.root)["opencv"])

    def test_missing_evidence_is_not_success(self):
        (self.root / "build.ninja").unlink()
        with self.assertRaisesRegex(ValueError, "No generated link"):
            module.inspect(self.root)
        (self.root / "compile_commands.json").write_text("[]")
        with self.assertRaisesRegex(ValueError, "No compilation"):
            module.inspect(self.root)
