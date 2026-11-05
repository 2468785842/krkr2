"""Generate original, redistributable PSB fixtures and exercise the TJS API.

Run under an X11 session: python3 tools/psbfile/smoke.py --engine /path/to/krkr2
Only synthetic files are written, in a fresh temporary directory.
"""
import argparse
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib


def array(values):
    width = max(1, (len(values).bit_length() + 7) // 8)
    return bytes([12 + width]) + len(values).to_bytes(width, 'little') + b'\x10' + b''.join(
        struct.pack('<I', value) for value in values)


def fixture(key='pixel.tjs', payload=b'global.psbResourceRead = 42;\n'):
    sections = [array([0]), key.encode() + b'\0', array([]), b'',
                array([0]), array([len(payload)]), payload,
                b'\x21' + array([0]) + b'\x11\0\x19\0']
    result = bytearray(40)
    offsets = []
    for section in sections:
        offsets.append(len(result))
        result.extend(section)
    struct.pack_into('<4sHH8I', result, 0, b'PSB\0', 1, 0, *offsets)
    return bytes(result)


def modern_fixture(version):
    # Build a single-key double-array name trie independently of the reader.
    key = b"pixel.tjs"
    bases = [0] * (256 * (len(key) + 2))
    parents = [0] * len(bases)
    parent = 0
    for index, char in enumerate(key):
        bases[parent] = 256 * (index + 1)
        child = bases[parent] + char
        parents[child] = parent
        parent = child
    terminal = len(parents) - 1
    parents[terminal] = parent
    names = array(bases) + array(parents) + array([terminal])
    payload = b'global.psbResourceRead = 42;\n'
    result = bytearray(40 if version == 2 else 44)
    offsets = [len(result)]
    for section in [names, array([]), b'', array([0]), array([len(payload)]),
                    payload, b'\x21' + array([0]) + array([0]) + b'\x19\0']:
        offsets.append(len(result))
        result.extend(section)
    struct.pack_into('<4sHH8I', result, 0, b'PSB\0', version, 0, *offsets)
    return bytes(result)


def octet(data):
    return '<% ' + data.hex(' ') + ' %>'


def generate(directory):
    directory.mkdir(parents=True, exist_ok=True)
    good = fixture()
    (directory / 'probe.pimg').write_bytes(good)
    nested = fixture('outer')[:-2] + b'\x20' + array([0]) + b'\x19\0'
    (directory / 'nested.pimg').write_bytes(nested)
    script = [
        'Plugins.link("layerExDraw.dll");',
        'Plugins.link("psbfile.dll");',
        'function check(value, message) { if (!value) throw message; }',
        'var file = new PSBFile();',
        'check(file.load("probe.pimg"), "file load");',
        'check(file.root instanceof "Dictionary", "root dictionary identity");',
        'check(typeof file.root["pixel.tjs"] == "Octet", "resource octet");',
        'Scripts.execStorage("psb://probe.pimg/pixel.tjs");',
        'check(global.psbResourceRead == 42, "registered resource");',
        'global.psbResourceRead = 0;',
        'var constructed = new PSBFile("probe.pimg");',
        'Scripts.execStorage("psb://probe.pimg/pixel.tjs");',
        'check(global.psbResourceRead == 42, "constructor resource");',
        'check(!Storages.isExistentStorage("psb://probe.pimg/missing"), "missing resource");',
        'var threw = false; try { Scripts.execStorage("psb://probe.pimg/missing"); } catch(e) { threw = true; }',
        'check(threw, "missing resource must throw");',
        'check(!Storages.isExistentStorage("psb://probe.pimg/missing"), "missing resource must stay missing");',
        f'var bytes = {octet(good)};',
        'check(file.load(bytes), "memory load");',
        'var memory = new PSBFile(bytes);',
        'check(memory.root["pixel.tjs"] == file.root["pixel.tjs"], "memory constructor");',
        'var retained = file.root["pixel.tjs"];',
        'var nested = new PSBFile("nested.pimg");',
        'check(nested.root.outer[0] == retained, "nested resource octet");',
        'global.psbResourceRead = 0;',
        'Scripts.execStorage("psb://nested.pimg/outer/0");',
        'check(global.psbResourceRead == 42, "nested registered resource");',
        'Storages.addAutoPath("psb://probe.pimg/");',
        'check(Storages.isExistentStorage("pixel.tjs"), "PSB media listing");',
        'check(!file.load("missing.psb"), "missing file");',
        'check(file.root["pixel.tjs"] == retained, "failed load preserves root");',
    ]
    bad = [b'', b'not a psb file' * 4, good[:39], good[:-1]]
    for offset, value in [(12, 0xffffffff), (36, 0xffffffff)]:
        data = bytearray(good)
        struct.pack_into('<I', data, offset, value)
        bad.append(bytes(data))
    data = bytearray(good)
    data[40] = 0x14  # Count would overwrite a four-byte stack variable.
    bad.append(bytes(data))
    data = bytearray(good)
    data[-1] = 255  # Out-of-range resource index.
    bad.append(bytes(data))
    data = bytearray(good)
    data[-3] = 255  # Out-of-range name index.
    bad.append(bytes(data))
    data = bytearray(good)
    data[42] = 255  # Invalid element width.
    bad.append(bytes(data))
    for index, data in enumerate(bad):
        script += [f'check(!file.load({octet(data)}), "invalid input {index}");',
                   'check(file.root["pixel.tjs"] == retained, "failed reload preserves data");']
    for magic in (b'MDF\0', b'mdf\0'):
        data = magic + struct.pack('<I', len(good)) + zlib.compress(good)
        script.append(f'check(file.load({octet(data)}), "MDF memory load");')
    for version in (2, 3):
        script += [f'check(file.load({octet(modern_fixture(version))}), "PSB v{version}");',
                   'check(file.root["pixel.tjs"] == retained, "name trie and resource");']
    # No id, and a non-string id, must both survive format inference.
    script += [f'check(file.load({octet(fixture("plain"))}), "generic dictionary");',
               f'check(file.load({octet(fixture("id"))}), "non-string id");',
               'check(typeof file.root.id == "Octet", "reload replaces root");',
               'Debug.message("PSBFILE_SMOKE_OK");', 'System.exit();']
    (directory / 'startup.tjs').write_text('\n'.join(script) + '\n', encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', type=Path)
    parser.add_argument('--generate', type=Path)
    args = parser.parse_args()
    if args.generate:
        generate(args.generate.resolve())
        return
    if not args.engine:
        parser.error('--engine or --generate is required')
    engine = args.engine.resolve()
    with tempfile.TemporaryDirectory(prefix='krkr-psbfile-') as temp:
        game = Path(temp)
        generate(game)
        result = subprocess.run([str(engine), '--game=' + str(game)],
                                cwd=engine.parent, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=60)
        output = result.stdout.decode('utf-8', errors='replace')
        print(output)
        if result.returncode or 'PSBFILE_SMOKE_OK' not in output:
            raise SystemExit('PSBFile smoke failed (exit %s)' % result.returncode)


if __name__ == '__main__':
    main()
