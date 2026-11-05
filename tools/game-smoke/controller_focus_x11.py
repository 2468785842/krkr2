#!/usr/bin/env python3
"""Exercise opt-in KAG ButtonLayer navigation through real X11 input."""
import argparse
from pathlib import Path
from keyboard_x11 import run


SCRIPT = '''
global.events = [];
global.record = function(text) {
    events.add(text);
    events.save(System.exePath + "keys.txt");
};
class TestWindow extends Window {
    function TestWindow() { super.Window(); setSize(640, 480); visible = true; }
    function onKeyDown(key, shift) {
        if(key == 69) { edit.visible = true; edit.focus(); }
        if(key == 77) { dialog.visible = true; dialog.setMode(); dialog.focus(); }
    }
    function onKeyUp(key, shift) {
        record("key:" + key + ":focus:" + (focusedLayer == null ? "none" : focusedLayer.name));
    }
}
class ButtonLayer extends Layer {
    function ButtonLayer(parent, label, y) {
        super.Layer(win, parent);
        name = label; left = 100; top = y; setSize(200, 40);
        focusable = false; visible = true;
    }
    function onKeyUp(key, shift) { if(key == 13) record("click:" + name); }
}
class EditLayer extends Layer {
    function EditLayer() {
        super.Layer(win, root); name = "edit"; focusable = true; visible = false;
    }
    function onKeyDown(key, shift) {}
}
global.win = new TestWindow();
global.root = new Layer(win, null);
root.setSize(640, 480); root.setImageSize(640, 480); root.visible = true;
global.bottom = new ButtonLayer(root, "bottom", 300);
global.middle = new ButtonLayer(root, "middle", 200);
global.disabled = new ButtonLayer(root, "disabled", 150); disabled.enabled = false;
global.top = new ButtonLayer(root, "top", 100);
global.outside = new ButtonLayer(root, "outside", -100);
global.hidden = new Layer(win, root); hidden.visible = false;
global.hiddenButton = new ButtonLayer(hidden, "hidden", 0);
global.edit = new EditLayer();
global.dialog = new Layer(win, root);
dialog.name = "dialog"; dialog.setSize(640, 480); dialog.focusable = true; dialog.visible = false;
global.modalButton = new ButtonLayer(dialog, "modal", 100);
global.timer = new Timer(function() {
    if(Storages.isExistentStorage(System.exePath + "done.txt")) System.exit();
}, "");
timer.interval = 50; timer.enabled = true;
record("ready");
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', type=Path, required=True)
    parser.add_argument('--xdotool', required=True)
    args = parser.parse_args()
    run(args.engine, args.xdotool, SCRIPT,
        [('Down', ['key:40:focus:none']), ('Return', ['key:13:focus:none'])])
    run(args.engine, args.xdotool, SCRIPT, [
        ('Down', ['key:40:focus:top']),
        ('Down', ['key:40:focus:middle']),
        ('Right', ['key:39:focus:bottom']),
        ('Up', ['key:38:focus:middle']),
        ('Left', ['key:37:focus:top']),
        ('Return', ['key:13:focus:top', 'click:top']),
        ('e', ['key:69:focus:edit']),
        ('Down', ['key:40:focus:edit']),
        ('m', ['key:77:focus:dialog']),
        ('Down', ['key:40:focus:modal']),
        ('Return', ['key:13:focus:modal', 'click:modal']),
    ], ['--', '-controllerfocus=yes'])


if __name__ == '__main__':
    main()
