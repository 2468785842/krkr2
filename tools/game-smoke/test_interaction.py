"""Evidence checks do not replace real UI interactions."""
from pathlib import Path
import tempfile
import unittest

import interaction_smoke


class InteractionEvidenceTest(unittest.TestCase):
    def test_clean_process_without_interactions_is_not_a_pass(self):
        failures = interaction_smoke.failures(["ready", "fixture-finished"])
        self.assertIn("inform-return:0", failures)
        self.assertIn("menu-click:1", failures)
        self.assertIn("close-query:1", failures)

    def test_cancel_cannot_count_as_file_selection(self):
        failures = interaction_smoke.failures(["select-cancel-return:0", "select-accept-return:0",
                                               "select-accept-name:file:///fixture.txt"])
        self.assertIn("select-accept-return:1", failures)
        self.assertIn("menu-click:0", failures)

    def test_prepare_refuses_to_overwrite_existing_directory(self):
        with tempfile.TemporaryDirectory() as temporary:
            target = Path(temporary) / "fixture"
            interaction_smoke.prepare(target)
            sentinel = target / "fixture.txt"
            sentinel.write_text("existing")
            with self.assertRaises(FileExistsError):
                interaction_smoke.prepare(target)
            self.assertEqual(sentinel.read_text(), "existing")


if __name__ == "__main__":
    unittest.main()
