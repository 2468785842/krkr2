"""Evidence validation only; actual OS input is exercised by the Windows probe."""
import unittest

import focus_repro as probe


class FocusEvidenceTest(unittest.TestCase):
    def events(self):
        return [*probe.REQUIRED, *probe.HELD_DOWN, *probe.CANCELLED, *probe.RECOVERED]

    def test_complete_trace_and_native_release_order(self):
        self.assertEqual(probe.failures(self.events()), [])
        events = self.events()
        first = events.index(probe.CANCELLED[0])
        events[first:first + 2] = reversed(events[first:first + 2])
        self.assertEqual(probe.failures(events), [])

    def test_cancel_click_or_duplicate_up_is_rejected(self):
        for event in ("focus:held:click:0:0", probe.CANCELLED[0]):
            self.assertTrue(probe.failures(self.events() + [event]))

    def test_focus_release_must_clear_all_keys_before_callbacks(self):
        events = self.events()
        events[events.index(probe.CANCELLED[0])] = "focus:held:mouse-up:0:1:0"
        self.assertTrue(probe.failures(events))

    def test_unfocused_or_late_recovered_input_is_rejected(self):
        for event in ("focus:unfocused:key-down:65:1:0", probe.RECOVERED[1]):
            self.assertTrue(probe.failures(self.events() + [event]))

    def test_stale_key_or_mouse_state_after_recovery_is_rejected(self):
        events = self.events()
        events[events.index(probe.RECOVERED[0])] = "focus:recovered:key-down:65:1:1"
        self.assertTrue(probe.failures(events))

    def test_missing_marker_error_or_background_notification_is_rejected(self):
        for event in probe.REQUIRED:
            events = self.events()
            events.remove(event)
            self.assertTrue(probe.failures(events))
        for event in ("error:stuck input", "lifecycle:deactivate"):
            self.assertTrue(probe.failures(self.events() + [event]))


if __name__ == "__main__":
    unittest.main()
