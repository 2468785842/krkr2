"""Reject incomplete or contaminated callback evidence; not a UI simulation."""
import unittest

import pointer_repro


class PointerEvidenceTest(unittest.TestCase):
    def events(self):
        return ["pointer-probe-ready", "pointer-hidden", "pointer-shown",
                *pointer_repro.EXPECTED, "fixture-finished"]

    def test_complete_trace(self):
        self.assertEqual(pointer_repro.failures(self.events()), [])

    def test_cancel_click_and_duplicate_release_are_rejected(self):
        for unexpected in ("pointer:click", "pointer:up:0"):
            events = self.events()
            events.insert(5, unexpected)
            self.assertTrue(pointer_repro.failures(events))

    def test_wrong_button_or_missing_callback_is_rejected(self):
        events = self.events()
        events.remove("pointer:up:1")
        self.assertTrue(pointer_repro.failures(events))
        events.append("pointer:up:0")
        self.assertTrue(pointer_repro.failures(events))

    def test_script_error_or_unfinished_fixture_is_rejected(self):
        self.assertTrue(pointer_repro.failures(self.events() + ["error:fixture failed"]))
        self.assertTrue(pointer_repro.failures(self.events()[:-1]))


if __name__ == "__main__":
    unittest.main()
