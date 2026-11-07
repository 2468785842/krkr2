"""Evidence checks do not replace real UI interactions."""
from pathlib import Path
from collections import Counter
import json
import sys
import tempfile
import time
import unittest
from unittest.mock import Mock, patch

import interaction_smoke
from interaction_repro import Reproducer


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

    def test_automation_failure_cannot_pass_with_complete_script_events(self):
        with tempfile.TemporaryDirectory() as temporary:
            target = Path(temporary) / "fixture"
            interaction_smoke.prepare(target)
            process = Mock(pid=123, returncode=0)
            process.poll.return_value = 0

            def failing_automation(process, directory, deadline, report):
                events = ["ready", "inform-return:0", "select-cancel-return:0", "select-accept-return:1",
                          "select-accept-name:file:///fixture.txt", "menu-click:1", "menu-click:0",
                          "veto-preserved", "close-query:0", "close-query:1", "close-returned"]
                (directory / "interaction-events.txt").write_text("\n".join(events), encoding="utf-8")
                raise TimeoutError("Missing UI acknowledgement")

            with patch("interaction_smoke.subprocess.Popen", return_value=process), patch("builtins.print"):
                code = interaction_smoke.run(Path(sys.executable), target, 5, automation=failing_automation)
            report = json.loads((target / "interaction-report.json").read_text())
            self.assertEqual(code, 1)
            self.assertFalse(report["passed"])
            self.assertTrue(any("Missing UI acknowledgement" in error for error in report["failures"]))
            process.terminate.assert_not_called()

    def test_repeated_round_requires_new_script_event(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "interaction-events.txt").write_text("select-cancel-return:0\n", encoding="utf-8")
            driver = Reproducer(Path(sys.executable), 2)
            driver.root = root
            driver.process = Mock(returncode=None)
            driver.process.poll.return_value = None
            driver.started = time.monotonic()
            driver.deadline = time.monotonic() + 0.02
            with self.assertRaises(TimeoutError):
                driver.script_event("select-cancel-return:0", Counter({"select-cancel-return:0": 1}))

    def test_other_action_id_cannot_acknowledge_a_click(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "ui-driver-events.txt").write_text("10 complete click-name\n", encoding="utf-8")
            driver = Reproducer(Path(sys.executable), 1)
            driver.root = root
            driver.process = Mock(returncode=None)
            driver.process.poll.return_value = None
            driver.started = time.monotonic()
            driver.deadline = time.monotonic() + 0.02
            with self.assertRaises(TimeoutError):
                driver.ui("click-name cancel")


if __name__ == "__main__":
    unittest.main()
