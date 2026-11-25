"""A file with the same basename from an earlier run is not current evidence."""
from pathlib import PureWindowsPath
import unittest

import sdl_windows_repro


class NativeFileEvidenceTest(unittest.TestCase):
    def events(self, path):
        return ['ready', 'select-cancel-return:0', 'select-accept-return:1',
                'menu-click:1', 'menu-click:0', 'veto-preserved', 'close-query:0',
                'close-query:1', 'close-returned', 'inform-return:0',
                'select-accept-name:' + path]

    def test_prior_fixture_with_same_basename_is_rejected(self):
        errors = sdl_windows_repro.failures(
            self.events('file://./d/probes/old/fixture.txt'),
            PureWindowsPath('D:/probes/current/fixture.txt'))
        self.assertIn('selected path is not the current fixture', errors)

    def test_current_storage_and_local_paths_are_accepted(self):
        expected = PureWindowsPath('D:/probes/current/fixture.txt')
        for path in ('file://./d/probes/current/fixture.txt',
                     'D:\\probes\\current\\fixture.txt'):
            with self.subTest(path=path):
                self.assertEqual(sdl_windows_repro.failures(self.events(path), expected), [])


if __name__ == '__main__':
    unittest.main()
