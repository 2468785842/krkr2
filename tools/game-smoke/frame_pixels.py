"""Read RGB samples from the non-interlaced 8-bit PNGs saved by Cocos."""
import struct
import zlib


def center_rgb(path):
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("Not a PNG")
    offset, compressed, header = 8, bytearray(), None
    while offset < len(data):
        size = struct.unpack_from(">I", data, offset)[0]
        kind = data[offset + 4:offset + 8]
        chunk = data[offset + 8:offset + 8 + size]
        crc = struct.unpack_from(">I", data, offset + 8 + size)[0]
        if zlib.crc32(kind + chunk) & 0xffffffff != crc:
            raise ValueError("PNG CRC mismatch")
        if kind == b"IHDR":
            header = struct.unpack(">IIBBBBB", chunk)
        elif kind == b"IDAT":
            compressed.extend(chunk)
        elif kind == b"IEND":
            break
        offset += size + 12
    if header is None:
        raise ValueError("Missing PNG header")
    width, height, depth, color, compression, filtering, interlace = header
    if (not 0 < width <= 16384 or not 0 < height <= 16384 or depth != 8 or
            color not in (2, 6) or (compression, filtering, interlace) != (0, 0, 0)):
        raise ValueError(f"Unsupported Cocos PNG format: {header}")
    channels = 3 if color == 2 else 4
    stride = width * channels
    raw = zlib.decompress(compressed)
    if len(raw) != (stride + 1) * height:
        raise ValueError("PNG scanline size mismatch")
    previous = bytearray(stride)
    for y in range(height // 2 + 1):
        start = y * (stride + 1)
        method = raw[start]
        row = bytearray(raw[start + 1:start + 1 + stride])
        for x in range(stride):
            left = row[x - channels] if x >= channels else 0
            above = previous[x]
            corner = previous[x - channels] if x >= channels else 0
            if method == 0:
                predictor = 0
            elif method == 1:
                predictor = left
            elif method == 2:
                predictor = above
            elif method == 3:
                predictor = (left + above) // 2
            elif method == 4:
                prediction = left + above - corner
                distances = [abs(prediction - value) for value in (left, above, corner)]
                predictor = (left, above, corner)[distances.index(min(distances))]
            else:
                raise ValueError(f"Unsupported PNG filter {method}")
            row[x] = (row[x] + predictor) & 255
        previous = row
    start = (width // 2) * channels
    return tuple(previous[start:start + 3])


def focus_frame_failures(directory):
    errors = []
    for name in ("native-focus-ready.png", "native-focus-recovered.png"):
        try:
            actual = center_rgb(directory / name)
            if actual != (48, 64, 80):
                errors.append(f"{name}: center RGB {actual} != (48, 64, 80)")
        except (OSError, ValueError, struct.error, zlib.error) as error:
            errors.append(f"{name}: {error}")
    return errors
