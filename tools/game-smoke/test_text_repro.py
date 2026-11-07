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

    def test_normalization_or_missing_text_after_NUL_is_rejected(self):
        for removed in ("text:native:1:769", "text:component:1:20013"):
            events = self.events()
            events.remove(removed)
            self.assertTrue(probe.failures(events))
        self.assertTrue(probe.failures(self.events() + ["text:component:0:-1"]))

    def test_closed_mode_or_inactive_window_leak_is_rejected(self):
        for phase in ("default-close", "default-dontcare-closed", "layer-close",
                      "layer-dontcare-closed", "secondary-close", "secondary-invalidated"):
            self.assertTrue(probe.failures(self.events() + [f"text:{phase}:1:81"]))

    def test_lost_layer_override_or_default_reset_is_rejected(self):
        for removed in probe.MODE_TEXT:
            events = self.events()
            events.remove(removed)
            self.assertTrue(probe.failures(events))

    def test_wrong_default_readback_or_focus_is_rejected(self):
        for marker in probe.MODE_MARKERS:
            events = self.events()
            events.remove(marker)
            events.append(marker + "wrong")
            self.assertTrue(probe.failures(events))

    def test_missing_checkpoint_or_error_is_rejected(self):
        events = self.events()
        events.remove("phase:unfocused-done")
        self.assertTrue(probe.failures(events))
        self.assertTrue(probe.failures(self.events() + ["error:IME callback failed"]))


if __name__ == "__main__":
    unittest.main()
