"""Ensure reports reject callbacks routed to the wrong or invalid Window."""
import unittest

import pointer_windows_repro as probe


class WindowPointerEvidenceTest(unittest.TestCase):
    def events(self):
        return ["ready", "pointer-windows-ready", "pointer-window-b",
                "pointer-window-b-held", "pointer-window-b-invalidated",
                "pointer-window-a-held", "pointer-window-b-hidden", "pointer-window-b-shown",
                *probe.EXPECTED,
                *["pointer-position:" + ":".join(event.split(":")[1:3]) + ":320:240"
                  for event in probe.EXPECTED if ":click" not in event], "fixture-finished"]

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
        for event in ("pointer:a:up:0", "pointer-window-b-held", "pointer-window-a-held",
                      "pointer-window-b-invalidated", "fixture-finished"):
            events = self.events()
            events.remove(event)
            self.assertTrue(probe.failures(events))

    def test_script_error_is_rejected(self):
        self.assertTrue(probe.failures(self.events() + ["error:invalid Window"]))

    def test_offscreen_window_coordinates_are_rejected(self):
        for coordinates in ("-320:240", "640:240", "320:480", "nan:240"):
            events = self.events()
            index = events.index("pointer-position:a:down:320:240")
            events[index] = "pointer-position:a:down:" + coordinates
            self.assertTrue(probe.failures(events))

    def test_missing_or_misrouted_coordinates_are_rejected(self):
        events = self.events()
        events.remove("pointer-position:a:down:320:240")
        self.assertTrue(probe.failures(events))
        events.insert(-1, "pointer-position:b:down:320:240")
        self.assertTrue(probe.failures(events))


if __name__ == "__main__":
    unittest.main()
