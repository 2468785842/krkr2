"""Exercise lazy WindowEx native-instance creation and destruction in the real engine."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

STARTUP = '''Plugins.link("windowEx.dll");
var window = new Window();
window.registerExEvent();
invalidate window;
["windowex-invalidated"].save(System.exePath + "result.txt");
System.exit();
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    engine = args.engine.resolve(strict=True)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix='run-', dir=args.output_dir.resolve()))
    script = root / 'startup.tjs'
    script.write_text(STARTUP, encoding='utf-8')
    report = {'engine_sha256': hashlib.sha256(engine.read_bytes()).hexdigest(),
              'script_sha256': hashlib.sha256(script.read_bytes()).hexdigest(),
              'forced_stop': False}
    with (root / 'process.log').open('wb') as log:
        process = subprocess.Popen([str(engine), '--game', str(root)], cwd=engine.parent,
                                   stdout=log, stderr=subprocess.STDOUT)
        try:
            report['exit_code'] = process.wait(timeout=25)
        except subprocess.TimeoutExpired:
            process.kill()
            report.update(exit_code=process.wait(), forced_stop=True)
    raw = (root / 'result.txt').read_bytes() if (root / 'result.txt').exists() else b''
    encoding = 'utf-16' if raw.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig'
    report['events'] = raw.decode(encoding, errors='replace').splitlines()
    report['passed'] = (report['exit_code'] == 0 and not report['forced_stop'] and
                        report['events'] == ['windowex-invalidated'])
    (root / 'report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(dict(report, evidence=str(root)), indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
