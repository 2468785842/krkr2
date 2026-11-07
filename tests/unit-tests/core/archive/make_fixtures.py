"""创建不含外部游戏资源的 ZIP/ZIP64 回归语料。"""
from pathlib import Path
import sys
import zipfile
import tarfile
import io
import struct

root = Path(sys.argv[1])
root.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(root / 'mixed.zip', 'w') as archive:
    archive.writestr('DIR\\Stored.TXT', b'0123456789', compress_type=zipfile.ZIP_STORED)
    archive.writestr('日本語.txt', '中文と日本語'.encode('utf-8'))
    archive.writestr('deflated.txt', b'compressed-data-' * 64, compress_type=zipfile.ZIP_DEFLATED)
    archive.writestr('empty.bin', b'', compress_type=zipfile.ZIP_DEFLATED)
with zipfile.ZipFile(root / 'zip64.zip', 'w') as archive:
    with archive.open('zip64.bin', 'w', force_zip64=True) as entry:
        entry.write(b'zip64 local header')
(root / 'invalid.zip').write_bytes(b'not a zip file')
(root / 'truncated.zip').write_bytes((root / 'mixed.zip').read_bytes()[:30])
# ZipInfo 在 Windows 会规范化反斜线；按实际名称修改本地头和中央目录。
with zipfile.ZipFile(root / 'mixed.zip') as archive:
    original_name = archive.namelist()[0].encode('utf-8')
data = (root / 'mixed.zip').read_bytes()
assert data.count(original_name) == 2
(root / 'invalid-name.zip').write_bytes(data.replace(original_name, b'\xff' + original_name[1:]))
with tarfile.open(root / 'plain.tar', 'w', format=tarfile.USTAR_FORMAT) as archive:
    entry = tarfile.TarInfo('folder/readme.txt')
    entry.size = 10
    archive.addfile(entry, io.BytesIO(b'0123456789'))
(root / 'invalid-index.xp3').write_bytes(b'XP3\r\n \n\x1a\x8bg\x01' + struct.pack('<Q', 19) + b'\x00')
(root / 'ready.stamp').write_text('ready\n')
