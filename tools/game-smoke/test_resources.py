"""Resource staging covers mode switches and rejects invalid manifests."""
import contextlib
import importlib.util
import io
from pathlib import Path
import sys
import tempfile
import unittest

sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location("runtime_assets", Path(__file__).parents[2] / "cmake/scripts/sync_runtime_assets.py")
assets = importlib.util.module_from_spec(spec)
spec.loader.exec_module(assets)


class ResourceSyncTest(unittest.TestCase):
    def test_retired_files_need_no_source_and_preserve_custom_resources(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source, destination = root / "assets", root / "runtime"
            source.mkdir()
            (source / "current.txt").write_text("current")
            for name in ("img/old.png", "img/custom.png", "out_ui/old.csd"):
                target = destination / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text(name)
            with contextlib.redirect_stdout(io.StringIO()):
                assets.sync(source, destination, {"current.txt"}, {"current.txt"},
                            {"img/old.png", "out_ui/old.csd"})
            self.assertEqual((destination / "current.txt").read_text(), "current")
            self.assertEqual((destination / "img/custom.png").read_text(), "img/custom.png")
            self.assertFalse((destination / "img/old.png").exists())
            self.assertFalse((destination / "out_ui").exists())

    def test_retirement_conflict_is_rejected_before_copying(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source, destination = root / "assets", root / "runtime"
            source.mkdir()
            destination.mkdir()
            (source / "current.txt").write_text("new")
            (destination / "current.txt").write_text("original")
            with self.assertRaises(ValueError):
                assets.sync(source, destination, {"current.txt"}, {"current.txt"}, {"current.txt"})
            self.assertEqual((destination / "current.txt").read_text(), "original")

    def test_retirement_refuses_directory_before_copying(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source, destination = root / "assets", root / "runtime"
            source.mkdir()
            destination.mkdir()
            (source / "current.txt").write_text("new")
            (destination / "current.txt").write_text("original")
            (destination / "img/old.png").mkdir(parents=True)
            with self.assertRaises(ValueError):
                assets.sync(source, destination, {"current.txt"}, {"current.txt"}, {"img/old.png"})
            self.assertEqual((destination / "current.txt").read_text(), "original")
            self.assertTrue((destination / "img/old.png").is_dir())

    def test_mode_switch_prunes_only_excluded_and_restores(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source, destination = root / "assets", root / "runtime"
            source.mkdir()
            destination.mkdir()
            all_files = {"shared.csd", "launcher.csd"}
            for name in all_files:
                (source / name).write_text(name)
            custom = destination / "custom.txt"
            custom.write_text("keep")
            with contextlib.redirect_stdout(io.StringIO()):
                assets.sync(source, destination, all_files, all_files)
                assets.sync(source, destination, all_files, {"shared.csd"})
                self.assertTrue((destination / "shared.csd").is_file())
                self.assertFalse((destination / "launcher.csd").exists())
                self.assertEqual(custom.read_text(), "keep")
                assets.sync(source, destination, all_files, all_files)
            self.assertEqual((destination / "launcher.csd").read_text(), "launcher.csd")

    def test_invalid_manifest_paths(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest = Path(directory) / "manifest.txt"
            for name in ("../escape", "/absolute", "C:/absolute", "folder\\escape"):
                manifest.write_text(name)
                with self.subTest(name=name), self.assertRaises(ValueError):
                    assets.entries(manifest)

    def test_invalid_selection_and_missing_source_leave_destination_untouched(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source, destination = root / "assets", root / "runtime"
            source.mkdir()
            destination.mkdir()
            (source / "prune").write_text("original")
            sentinel = destination / "prune"
            sentinel.write_text("keep")
            with self.assertRaises(ValueError):
                assets.sync(source, destination, {"prune"}, {"foreign"})
            with self.assertRaises(ValueError):
                assets.sync(source, destination, {"prune", "missing"}, set())
            self.assertEqual(sentinel.read_text(), "keep")

    def test_resolved_path_cannot_escape_destination(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with self.assertRaises(ValueError):
                assets.within(root / "assets", "../outside")


if __name__ == "__main__":
    unittest.main()
