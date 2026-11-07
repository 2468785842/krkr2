"""Exercise the extractor with self-generated raw and zlib XP3 archives."""
import pathlib
import struct
import subprocess
import sys
import tempfile
import zlib


def chunk(tag, data):
    return tag + struct.pack("<Q", len(data)) + data


def archive(compressed):
    payload = b"XP3 isolation smoke test\n" * 100
    name = "nested/example.txt"
    stored = zlib.compress(payload) if compressed else payload
    info = struct.pack("<IQQH", 0, len(payload), len(stored), len(name))
    info += name.encode("utf-16-le")
    segment = struct.pack("<IQQQ", int(compressed), 19, len(payload), len(stored))
    index = chunk(b"File", chunk(b"info", info) + chunk(b"segm", segment)
                  + chunk(b"adlr", struct.pack("<I", zlib.adler32(payload))))
    if compressed:
        packed = zlib.compress(index)
        index = b"\x01" + struct.pack("<QQ", len(packed), len(index)) + packed
    else:
        index = b"\x00" + struct.pack("<Q", len(index)) + index
    return b"XP3\r\n \n\x1a\x8bg\x01" + struct.pack("<Q", 19 + len(stored)) + stored + index, payload


def main():
    executable = pathlib.Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="xp3-smoke-") as temporary:
        root = pathlib.Path(temporary)
        for compressed in (False, True):
            data, payload = archive(compressed)
            source = root / ("compressed.xp3" if compressed else "raw.xp3")
            source.write_bytes(data)
            subprocess.run([str(executable), str(source), "-o", str(root / "out")], check=True)
            result = root / "out" / source.stem / "nested" / "example.txt"
            assert result.read_bytes() == payload, result
        invalid = root / "invalid.xp3"
        invalid.write_bytes(b"This is not an XP3 archive.\n")
        result = subprocess.run([str(executable), str(invalid), "-o", str(root / "out")],
                                capture_output=True, text=True)
        assert result.returncode == 0 and "Not an XP3 archive" in result.stderr, result
        assert not (root / "out" / "invalid").exists()
    print("XP3 raw/zlib extraction and invalid-signature checks passed")


if __name__ == "__main__":
    main()
