#!/usr/bin/env python3
"""Exercise the real engine's text reader with original, synthetic fixtures.

Use --engine /path/to/krkr2 [--output-dir /evidence/parent/directory], or
--generate /fresh/fixture/directory to stage a fixture for a separate device.
On Linux without DISPLAY, xvfb-run must be available. No game files are used.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import time


SUCCESS = "TEXTSTREAM_SMOKE_OK"
FAILURE = "TEXTSTREAM_SMOKE_FAILED"
UNICODE = "A中あΩ🙂\tZ"


def simple_text(text, mode):
    """Build the documented five-byte header and UTF-16LE code units."""
    units = struct.unpack("<%dH" % (len(text.encode("utf-16le")) // 2),
                          text.encode("utf-16le"))
    if mode == 0:
        units = [unit ^ (((unit & 0xfe) << 8) ^ 1) if unit >= 0x20 else unit
                 for unit in units]
    elif mode == 1:
        units = [((unit & 0xaaaa) >> 1) | ((unit & 0x5555) << 1) for unit in units]
    else:
        raise ValueError("Only simple text modes 0 and 1 are fixtures")
    return bytes([0xfe, 0xfe, mode, 0xff, 0xfe]) + struct.pack("<%dH" % len(units), *units)


def quote(value):
    # The fixture itself uses UTF-16LE, so TJS does not need JSON's \u escapes.
    return json.dumps(value, ensure_ascii=False)


def generate(directory):
    directory.mkdir(parents=True, exist_ok=False)
    fixtures = {
        # Fixed vectors are independent of simple_text(), and include a newline.
        "fixed-mode0.txt": bytes.fromhex("fe fe 00 ff fe 40 40 2c 62 0a 00"),
        "fixed-mode1.txt": bytes.fromhex("fe fe 01 ff fe 82 00 1e 8d 05 00"),
        "unicode-utf8.txt": b"\xef\xbb\xbf" + (UNICODE + "\r\nlast").encode("utf-8"),
        "short-utf8.txt": b"\xef\xbb\xbfA",
        "empty.txt": b"",
        "empty-utf8.txt": b"\xef\xbb\xbf",
        "empty-utf16.txt": b"\xff\xfe",
        "signature-only.txt": b"\xfe\xfe",
        "unknown-mode.txt": b"\xfe\xfe\x03\xff\xfe",
    }
    malformed = ["signature-only.txt", "unknown-mode.txt"]
    for mode in (0, 1):
        fixtures[f"unicode-mode{mode}.txt"] = simple_text(UNICODE + "\r\nlast", mode)
        fixtures[f"empty-mode{mode}.txt"] = simple_text("", mode)
        source = 'global.textstreamExecuted++; global.textstreamValue = ' + quote(UNICODE) + ';\n'
        fixtures[f"execute-mode{mode}.tjs"] = simple_text(source, mode)
        invalid = {
            "short-mode": bytes([0xfe, 0xfe, mode]),
            "short-bom": bytes([0xfe, 0xfe, mode, 0xff]),
            "bad-bom": bytes([0xfe, 0xfe, mode, 0, 0]),
            "odd-payload": simple_text("A", mode) + b"\x00",
        }
        for label, data in invalid.items():
            name = f"{label}-{mode}.txt"
            fixtures[name] = data
            malformed.append(name)
    fixtures["offset-mode1.txt"] = b"prefix!" + fixtures["fixed-mode1.txt"]
    for name, data in fixtures.items():
        (directory / name).write_bytes(data)

    script = [
        'global.textstreamWindow = new Window();',
        'textstreamWindow.setSize(64, 64); textstreamWindow.visible = true;',
        'global.textstreamChecks = 0;',
        'function check(value, message) { if(!value) throw message; textstreamChecks++; }',
        'function lines(name, expected, mode="") {',
        ' var actual = new Array(); actual.load(name, mode);',
        ' check(actual.count == expected.count, name + " line count: " + actual.count);',
        ' for(var i = 0; i < expected.count; ++i)',
        '  check(actual[i] === expected[i], name + " contents/length at line " + i);',
        '}',
        'function rejected(name, mode="") {',
        ' var threw = false;',
        ' try { var actual = new Array(); actual.load(name, mode); } catch(e) { threw = true; }',
        ' check(threw, name + " must reject malformed input");',
        '}',
        'try {',
        ' lines("fixed-mode0.txt", ["A中"]);',
        ' lines("fixed-mode1.txt", ["A中"]);',
        f' lines("unicode-mode0.txt", [{quote(UNICODE)}, "last"]);',
        f' lines("unicode-mode1.txt", [{quote(UNICODE)}, "last"]);',
        f' lines("unicode-utf8.txt", [{quote(UNICODE)}, "last"]);',
        ' lines("short-utf8.txt", ["A"]);',
        ' lines("offset-mode1.txt", ["A中"], "o7");',
        f' lines("offset-mode1.txt", [], "o{len(fixtures["offset-mode1.txt"])}");',
        ' rejected("offset-mode1.txt", "o999");',
        ' global.textstreamExecuted = 0; global.textstreamValue = "";',
    ]
    for mode in (0, 1):
        script.extend([
            f' Scripts.execStorage("execute-mode{mode}.tjs");',
            f' check(textstreamExecuted == {mode + 1}, "mode {mode} script executes exactly once");',
            f' check(textstreamValue === {quote(UNICODE)}, "mode {mode} script Unicode");',
        ])
    for name in ("empty.txt", "empty-utf8.txt", "empty-utf16.txt",
                 "empty-mode0.txt", "empty-mode1.txt"):
        script.append(f' lines({quote(name)}, []);')
    for name in malformed:
        script.append(f' rejected({quote(name)});')
    script.extend([
        f' var saved = [{quote(UNICODE)}, "tail"];',
        ' saved.save("writer-mode1.txt", "c1");',
        ' lines("writer-mode1.txt", saved);',
        f' Debug.message("{SUCCESS} checks=" + textstreamChecks);',
        '} catch(e) {',
        f' Debug.message("{FAILURE}: " + (typeof e == "String" ? e : e.message));',
        '}',
        # Exit from the mapped window's event loop, as in layer_alpha.py.
        'global.textstreamExit = new Timer(function() {',
        ' textstreamExit.enabled = false; System.exit();',
        '}, "");',
        'textstreamExit.interval = 100; textstreamExit.enabled = true;',
    ])
    startup = "\n".join(script) + "\n"
    (directory / "startup.tjs").write_bytes(b"\xff\xfe" + startup.encode("utf-16le"))
    manifest = {name: hashlib.sha256(data).hexdigest() for name, data in fixtures.items()}
    manifest["startup.tjs"] = hashlib.sha256((directory / "startup.tjs").read_bytes()).hexdigest()
    (directory / "textstream-fixture.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")


def run(engine, directory):
    engine = engine.resolve(strict=True)
    generate(directory)
    command = [str(engine), directory.as_posix() + "/"]
    environment = os.environ.copy()
    environment.setdefault("LIBGL_ALWAYS_SOFTWARE", "1")
    environment.setdefault("ALSOFT_DRIVERS", "null")
    if sys.platform.startswith("linux") and not environment.get("DISPLAY"):
        xvfb = shutil.which("xvfb-run")
        if not xvfb:
            raise RuntimeError("An X11 DISPLAY or xvfb-run is required")
        command = [xvfb, "--auto-servernum", "--server-args=-screen 0 1280x720x24", *command]
    started = time.monotonic()
    digest = hashlib.sha256()
    with engine.open("rb") as binary:
        for block in iter(lambda: binary.read(1024 * 1024), b""):
            digest.update(block)
    engine_hash = digest.hexdigest()
    report = {"engine": str(engine), "engine_sha256": engine_hash, "directory": str(directory),
              "command": command, "forced_stop": False, "passed": False}
    process = subprocess.Popen(command, cwd=engine.parent, env=environment,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               start_new_session=(os.name == "posix"))
    try:
        output, _ = process.communicate(timeout=60)
    except subprocess.TimeoutExpired:
        report["forced_stop"] = True
        if os.name == "posix":
            try:
                os.killpg(process.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
        else:
            process.terminate()
        try:
            output, _ = process.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            if os.name == "posix":
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
            else:
                process.kill()
            output, _ = process.communicate(timeout=5)
    report["exit_code"] = process.returncode
    text = output.decode("utf-8", errors="replace")
    (directory / "textstream-process.log").write_bytes(output)
    writer = directory / "writer-mode1.txt"
    report["writer_header_valid"] = writer.is_file() and writer.read_bytes()[:5] == b"\xfe\xfe\x01\xff\xfe"
    report["passed"] = (report["exit_code"] == 0 and not report["forced_stop"]
                        and SUCCESS in text and FAILURE not in text and report["writer_header_valid"])
    report["elapsed_seconds"] = round(time.monotonic() - started, 2)
    (directory / "textstream-report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(text)
    print(json.dumps(report, indent=2))
    return 0 if report["passed"] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--engine", type=Path)
    action.add_argument("--generate", type=Path)
    parser.add_argument("--output-dir", type=Path)
    args = parser.parse_args()
    if args.generate:
        if args.output_dir:
            parser.error("--output-dir requires --engine")
        generate(args.generate.resolve())
        return 0
    if args.output_dir:
        parent = args.output_dir.resolve()
        parent.mkdir(parents=True, exist_ok=True)
        evidence = Path(tempfile.mkdtemp(prefix="run-", dir=parent))
        return run(args.engine, evidence / "fixture")
    with tempfile.TemporaryDirectory(prefix="krkr-textstream-") as temporary:
        return run(args.engine, Path(temporary) / "fixture")


if __name__ == "__main__":
    raise SystemExit(main())
