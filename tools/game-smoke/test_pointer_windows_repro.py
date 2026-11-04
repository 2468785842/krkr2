"""Ensure reports reject callbacks routed to the wrong or invalid Window."""
import unittest

import pointer_windows_repro as probe


class WindowPointerEvidenceTest(unittest.TestCase):
    def events(self):
        return ["ready", "pointer-windows-ready", "pointer-window-b",
                "pointer-window-b-invalidated", *probe.EXPECTED, "fixture-finished"]

    def test_complete_trace(self):
        self.assertEqual(probe.failures(self.events()), [])

    def test_callback_to_wrong_window_is_rejected(self):
        events = self.events()
        index = events.index("pointer:a:up:0")
        events[index] = "pointer:b:up:0"
        self.assertTrue(probe.failures(events))

    def test_old_touch_release_or_click_is_rejected(self):
        for event in ("pointer:b:up:0", "pointer:a:click", "pointer:b:down:1"):
            events = self.events()
            events.insert(-1, event)
            self.assertTrue(probe.failures(events))

    def test_missing_release_or_invalidation_is_rejected(self):
        for event in ("pointer:a:up:0", "pointer-window-b-invalidated", "fixture-finished"):
            events = self.events()
            events.remove(event)
            self.assertTrue(probe.failures(events))

    def test_script_error_is_rejected(self):
        self.assertTrue(probe.failures(self.events() + ["error:invalid Window"]))


if __name__ == "__main__":
    unittest.main()
