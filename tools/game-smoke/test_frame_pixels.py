"""Exercise PNG filtering and rejection of black or missing frame evidence."""
from pathlib import Path
import struct
import tempfile
import unittest
import zlib

from frame_pixels import center_rgb, focus_frame_failures


def png(rows, color=2):
    def chunk(kind, data):
        return (struct.pack(">I", len(data)) + kind + data +
                struct.pack(">I", zlib.crc32(kind + data) & 0xffffffff))
    return (b"\x89PNG\r\n\x1a\n" +
            chunk(b"IHDR", struct.pack(">IIBBBBB", 2, 2, 8, color, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))


class FramePixelsTest(unittest.TestCase):
    def test_all_five_png_filters(self):
        # The same two RGB rows, independently encoded with each PNG filter.
        rows = [([10, 20, 30, 48, 64, 80], [1, 2, 3, 48, 64, 80]),
                ([10, 20, 30, 38, 44, 50], [1, 2, 3, 47, 62, 77]),
                ([10, 20, 30, 48, 64, 80], [247, 238, 229, 0, 0, 0]),
                ([10, 20, 30, 43, 54, 65], [252, 248, 244, 24, 31, 39]),
                ([10, 20, 30, 38, 44, 50], [247, 238, 229, 0, 0, 50])]
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "frame.png"
            for method, (first, second) in enumerate(rows):
                with self.subTest(filter=method):
                    path.write_bytes(png(bytes([method, *first, method, *second])))
                    self.assertEqual(center_rgb(path), (48, 64, 80))

    def test_rgba_and_corrupt_crc(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "frame.png"
            data = png(bytes([0, 1, 2, 3, 255, 48, 64, 80, 255] * 2), color=6)
            path.write_bytes(data)
            self.assertEqual(center_rgb(path), (48, 64, 80))
            path.write_bytes(data[:-1] + bytes([data[-1] ^ 1]))
            with self.assertRaisesRegex(ValueError, "CRC"):
                center_rgb(path)

    def test_missing_or_black_recovered_frame_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.assertEqual(len(focus_frame_failures(root)), 2)
            frame = png(bytes([0, 1, 2, 3, 48, 64, 80] * 2))
            (root / "native-focus-ready.png").write_bytes(frame)
            recovered = root / "native-focus-recovered.png"
            recovered.write_bytes(frame)
            self.assertEqual(focus_frame_failures(root), [])
            recovered.write_bytes(png(bytes(14)))
            self.assertEqual(len(focus_frame_failures(root)), 1)


if __name__ == "__main__":
    unittest.main()
