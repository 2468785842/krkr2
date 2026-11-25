"""Standard desktop presets must repair a cached disabled dependency install."""
import platform
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class DesktopPresetTest(unittest.TestCase):
    def test_plain_preset_reenables_installation_from_existing_cache(self):
        root = Path(__file__).resolve().parents[2]
        output = root / 'out/preset-regression'
        output.mkdir(parents=True, exist_ok=True)
        target = {'Windows': 'Windows', 'Linux': 'Linux'}.get(platform.system())
        if target is None:
            self.skipTest('Only supported desktop hosts have application presets')
        with tempfile.TemporaryDirectory(dir=output) as directory:
            project = Path(directory)
            shutil.copy2(root / 'CMakePresets.json', project / 'CMakePresets.json')
            (project / 'CMakeLists.txt').write_text(
                'cmake_minimum_required(VERSION 3.28)\n'
                'project(PresetProbe NONE)\n'
                'file(WRITE "${CMAKE_BINARY_DIR}/install-state.txt" "${VCPKG_MANIFEST_INSTALL}")\n',
                encoding='utf-8')
            for build_type in ('Debug', 'Release'):
                with self.subTest(build_type=build_type):
                    command = ['cmake', '--preset', f'{target} {build_type} Config']
                    seeded = subprocess.run(command + ['-DVCPKG_MANIFEST_INSTALL=OFF'],
                                            cwd=project, capture_output=True, text=True, timeout=30)
                    self.assertEqual(seeded.returncode, 0, seeded.stdout + seeded.stderr)
                    state = project / 'out' / target.lower() / build_type.lower() / 'install-state.txt'
                    self.assertEqual(state.read_text(), 'OFF')
                    result = subprocess.run(command, cwd=project, capture_output=True,
                                            text=True, timeout=30)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    self.assertIn(state.read_text().upper(), {'ON', 'TRUE', '1'})


if __name__ == '__main__':
    unittest.main()
