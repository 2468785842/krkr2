"""Controller tests use Python as a fake child; they do not emulate the game engine."""
import contextlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest

import game_smoke


class ControllerTest(unittest.TestCase):
    def child(self, freeze, probe=False):
        with tempfile.TemporaryDirectory() as directory:
            game = Path(directory)
            (game / "smoke-manifest.json").write_text("{}")
            # Python runs this file while the real runtime treats it as an archive.
            (game / "data.xp3").write_text(
                "import pathlib, time\n"
                "p = pathlib.Path(__file__).with_name('smoke-state.txt')\n"
                "for tick in range(1000):\n"
                f"    if not {freeze!r} or tick < 3:\n"
                "        p.write_text('krkr2-game-smoke-v1\\nheartbeat\\n' + str(tick) + '\\n0\\nproject\\nsavedata\\nkag-present\\n', encoding='utf-16')\n"
                "    time.sleep(0.2)\n",
                encoding="utf-8",
            )
            with contextlib.redirect_stdout(io.StringIO()):
                result = game_smoke.run(Path(sys.executable), game, 4 if freeze else 2, probe_engine_args=probe)
            report = json.loads((game / "smoke-report.json").read_text(encoding="utf-8"))
            return result, report

    def test_fresh_heartbeat_and_owned_process_cleanup(self):
        result, report = self.child(False)
        self.assertEqual(result, 0)
        self.assertTrue(report["forced_stop"])
        self.assertTrue(report["alive_at_deadline"])

    def test_frozen_child_is_not_a_pass(self):
        result, report = self.child(True)
        self.assertEqual(result, 1)
        self.assertTrue(report["alive_at_deadline"])
        self.assertGreater(report["heartbeat_age_seconds"], 3)

    def test_heartbeat_cannot_hide_missing_engine_arguments(self):
        result, report = self.child(False, probe=True)
        self.assertEqual(result, 1)
        self.assertTrue(report["alive_at_deadline"])
        self.assertFalse(report["engine_argument_probe_passed"])


if __name__ == "__main__":
    unittest.main()
