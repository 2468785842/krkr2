"""Reject incomplete Unicode, normalization, truncation and stale-input evidence."""
import unittest
import text_repro as probe


class TextEvidenceTest(unittest.TestCase):
    def events(self):
        return [*probe.MARKERS, *probe.EXPECTED]

    def test_complete_utf16_trace(self):
        self.assertEqual(probe.failures(self.events()), [])

    def test_truncated_or_reordered_surrogate_pair_is_rejected(self):
        events = self.events()
        events.remove("text:native:1:55357")
        self.assertTrue(probe.failures(events))
        events.append("text:native:1:55357")
        self.assertTrue(probe.failures(events))

    def test_prefix_from_invalid_commit_or_unfocused_text_is_rejected(self):
        for extra in ("text:component:1:65", "text:unfocused:1:20013"):
            self.assertTrue(probe.failures(self.events() + [extra]))

    def test_normalization_or_missing_empty_key_is_rejected(self):
        for removed in ("text:native:1:769", "text:component:0:-1"):
            events = self.events()
            events.remove(removed)
            self.assertTrue(probe.failures(events))

    def test_missing_checkpoint_or_error_is_rejected(self):
        events = self.events()
        events.remove("phase:unfocused-done")
        self.assertTrue(probe.failures(events))
        self.assertTrue(probe.failures(self.events() + ["error:IME callback failed"]))


if __name__ == "__main__":
    unittest.main()
