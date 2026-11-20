import struct
import tempfile
import unittest
from pathlib import Path
import imageops_repro as probe


class ImageOpsReports(unittest.TestCase):
    def test_event_reader_handles_partial_utf16(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.assertEqual(probe.events(root), [])
            path = root / "imageops-events.txt"
            path.write_bytes(b"\xff\xfe\x61")
            self.assertEqual(probe.events(root), [])
            path.write_text("ready\nfinished\n", encoding="utf-16")
            self.assertEqual(probe.events(root), ["ready", "finished"])

    def test_exported_bmp_decoder_handles_both_row_directions(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "frame.bmp"
            pixels = bytearray(384*192*4)
            for height in (192, -192):
                pixels[:] = bytes(len(pixels))
                row = 192-1-42 if height > 0 else 42
                offset = (row*384+17)*4
                pixels[offset:offset+4] = bytes([33, 22, 11, 255])
                header = b"BM" + struct.pack("<III", 54+len(pixels), 0, 54)
                header += struct.pack("<IiiHHIIIIII", 40, 384, height, 1, 32, 0, len(pixels), 0, 0, 0, 0)
                path.write_bytes(header+pixels)
                self.assertEqual(probe.bmp_rgb(path, 17, 42), (11, 22, 33))
                self.assertEqual(probe.bmp_rgb(path, 18, 42), (0, 0, 0))
                self.assertTrue(probe.frame_failures(path))
                path.write_bytes(header+pixels[:-1])
                with self.assertRaisesRegex(ValueError, "Truncated"):
                    probe.bmp_rgb(path, 17, 42)
