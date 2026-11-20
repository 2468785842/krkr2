import struct
import tempfile
import unittest
from unittest.mock import patch
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

    def test_edge_marker_rejects_a_clipped_column_and_bright_host_ui(self):
        width,height = 100,80
        pixels = [bytearray([112,64,112]*width) for _ in range(height)]
        # Bright UI below the central band and bright content at the left
        # must never count as the newly visible rightmost column.
        pixels[70][98*3:99*3] = bytes([255,255,255])
        pixels[40][0:3] = bytes([255,255,255])
        pixels[40][98*3:99*3] = bytes([168,136,168])  # 50% edge coverage
        def rows():
            return iter((width,height,3,row) for row in pixels)
        with patch.object(probe,"rgb_rows",side_effect=lambda _: rows()):
            self.assertEqual(probe.right_edge_marker_pixels(Path("unused")),0)
            pixels[40][98*3:99*3] = bytes([222,206,222])
            self.assertEqual(probe.right_edge_marker_pixels(Path("unused")),1)
