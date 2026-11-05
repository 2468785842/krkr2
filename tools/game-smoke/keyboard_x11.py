#!/usr/bin/env python3
"""Verify real X11 key presses reach TJS; run with xvfb-run on Linux."""
import argparse
from pathlib import Path
import subprocess
import tempfile
import time


SCRIPT = '''
global.events = [];
global.record = function(text) {
    events.add(text);
    events.save(System.exePath + "keys.txt");
};
class KeyWindow extends Window {
    function KeyWindow() {
        super.Window();
        setSize(640, 480);
        useMouseKey = false;
        visible = true;
    }
    function onKeyDown(key, shift) { record("down:" + string key); }
    function onKeyUp(key, shift) { record("up:" + string key); }
}
global.win = new KeyWindow();
global.layer = new Layer(win, null);
layer.setSize(640, 480);
layer.setImageSize(640, 480);
layer.visible = true;
global.timer = new Timer(function() {
    if(Storages.isExistentStorage(System.exePath + "done.txt")) System.exit();
}, "");
timer.interval = 50;
timer.enabled = true;
record("ready");
'''


def wait_for(check, process, description):
    deadline = time.monotonic() + 15
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError(f"Engine exited before {description}: {process.returncode}")
        result = check()
        if result:
            return result
        time.sleep(0.05)
    raise RuntimeError(f"Timed out waiting for {description}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', type=Path, required=True)
    parser.add_argument('--xdotool', required=True)
    args = parser.parse_args()
    engine = args.engine.resolve()
    with tempfile.TemporaryDirectory(prefix='krkr-x11-keys-') as temp:
        game = Path(temp)
        (game / 'startup.tjs').write_text(SCRIPT, encoding='utf-8')
        with (game / 'engine.log').open('wb') as log:
            process = subprocess.Popen([str(engine), '--game=' + str(game)],
                                       cwd=engine.parent, stdout=log, stderr=subprocess.STDOUT)
            try:
                def xdo(*arguments):
                    return subprocess.run([args.xdotool, *arguments], check=True,
                                          capture_output=True, text=True, timeout=10).stdout.strip()

                def events():
                    try:
                        return (game / 'keys.txt').read_text(encoding='utf-16').splitlines()
                    except (FileNotFoundError, UnicodeError):
                        return []

                wait_for(lambda: events() == ['ready'], process, 'script ready')
                windows = xdo('search', '--sync', '--onlyvisible', '--pid', str(process.pid)).splitlines()
                if len(windows) != 1:
                    raise RuntimeError(f"Expected one mapped engine window, got {windows}")
                xdo('windowfocus', '--sync', windows[0])
                expected = ['ready']
                for name, vk in [('Return', 13), ('KP_Enter', 13), ('Escape', 27),
                                 ('Down', 40), ('Control_L', 17)]:
                    xdo('key', '--clearmodifiers', name)
                    expected += [f'down:{vk}', f'up:{vk}']
                    wait_for(lambda: events() == expected, process, f'{name} down/up; got {events()}')
                    print(f'X11 key verified: {name} -> {vk}', flush=True)
                (game / 'done.txt').write_text('done', encoding='utf-8')
                if process.wait(timeout=10) != 0:
                    raise RuntimeError(f"Engine exit: {process.returncode}")
                print('X11_KEYBOARD_SMOKE_OK: Return, KP_Enter, Escape, Down, Control_L; normal exit')
            finally:
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait()
                log.flush()
                print((game / 'engine.log').read_text(encoding='utf-8', errors='replace'))


if __name__ == '__main__':
    main()
