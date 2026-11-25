import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("dependency_audit", Path(__file__).resolve().parents[1] / "audit_dependencies.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class DependencyAuditTest(unittest.TestCase):
    def test_features_and_continuations_combine_without_counting_ports_twice(self):
        ports = module.installed_ports("""Package: codec
Version: 1.0
Architecture: x64-linux
Depends: zlib, tool:x64-linux,
 other
Status: install ok installed

Package: codec
Feature: simd
Architecture: x64-linux
Depends: zlib
Status: install ok installed

Package: stale
Architecture: x64-linux
Status: deinstall ok not-installed
""")
        self.assertEqual(set(ports), {"codec:x64-linux"})
        self.assertEqual(ports["codec:x64-linux"]["features"], {"simd"})
        self.assertEqual(ports["codec:x64-linux"]["dependencies"], {"zlib", "tool:x64-linux", "other"})

    def test_inventory_sizes_unique_files_and_reports_missing_files(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "vcpkg/info").mkdir(parents=True)
            (root / "x64-linux/lib").mkdir(parents=True)
            (root / "x64-linux/lib/a.a").write_bytes(b"ABC")
            (root / "vcpkg/info/codec_1_x64-linux.list").write_text(
                "x64-linux/\nx64-linux/lib/a.a\nx64-linux/lib/a.a\nx64-linux/lib/missing.a\n")
            result = module.footprint(root, {"name": "codec", "triplet": "x64-linux"})
            self.assertEqual(result["installed_bytes"], 3)
            self.assertEqual(result["file_count"], 1)
            self.assertEqual(result["missing_files"], ["x64-linux/lib/missing.a"])
            for invalid in ("../outside", "C:/outside", "/outside"):
                (root / "vcpkg/info/codec_1_x64-linux.list").write_text(invalid + "\n")
                with self.assertRaisesRegex(ValueError, "Invalid SDK"):
                    module.footprint(root, {"name": "codec", "triplet": "x64-linux"})

    def test_host_and_target_packages_remain_separate(self):
        ports = module.installed_ports("""Package: generator
Architecture: x64-linux
Status: install ok installed

Package: generator
Architecture: arm64-android
Status: install ok installed
""")
        self.assertEqual(len(ports), 2)


if __name__ == "__main__":
    unittest.main()
