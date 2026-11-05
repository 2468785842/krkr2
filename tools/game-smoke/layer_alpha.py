#!/usr/bin/env python3
"""Synthetic layerExBTOA pixel regression; run under X11 (e.g. xvfb-run)."""
import argparse
from pathlib import Path
import subprocess
import tempfile


SCRIPT = r'''
try {
function check(value, message) { if(!value) throw message; }
Plugins.link("layerExBTOA.dll");
global.win = new Window();
win.setSize(64, 64);
win.visible = true;
global.root = new Layer(win, null);
root.setSize(64, 64);
root.visible = true;
function image() {
    var layer = new Layer(win, root);
    layer.type = ltAlpha;
    layer.setImageSize(4, 3);
    layer.setSize(4, 3);
    return layer;
}
global.dst = image(); global.src = image();
function reset() {
    dst.type = ltAlpha; dst.face = dfAlpha; dst.holdAlpha = false;
    src.setClip(); dst.setClip();
    dst.fillRect(0, 0, 4, 3, 0x80123456);
    src.fillRect(0, 0, 4, 3, 0x80000000);
}
function pixel(layer, x, y, alpha, rgb, label) {
    check(layer.getMaskPixel(x, y) == alpha, label + " alpha: " + layer.getMaskPixel(x, y) + " != " + alpha);
    check(layer.getMainPixel(x, y) == rgb, label + " RGB: " + layer.getMainPixel(x, y) + " != " + rgb);
}
reset();
// Compare RGB against the pre-operation readback: the existing OpenGL
// getMainPixel exposes native R/B order, independently of this plugin.
global.baseRGB = dst.getMainPixel(0, 0);
src.setMaskPixel(0, 0, 0); src.setMaskPixel(2, 0, 255);
dst.imageModified = false;
dst.clipAlphaRect(0, 0, src, 0, 0, 4, 3);
pixel(dst, 0, 0, 0, baseRGB, "zero");
pixel(dst, 1, 0, 64, baseRGB, "half");
pixel(dst, 2, 0, 128, baseRGB, "full");
pixel(dst, 3, 2, 64, baseRGB, "last row/pitch");
check(dst.imageModified, "modified flag");
pixel(src, 1, 0, 128, 0, "source unchanged");

reset(); dst.setClip(1, 1, 2, 1); src.setClip(3, 2, 1, 1);
dst.face = dfMask; dst.holdAlpha = true;
dst.clipAlphaRect(2, 1, src, 0, 0, 1, 1, 17);
pixel(dst, 0, 1, 128, baseRGB, "outside clip");
pixel(dst, 1, 1, 17, baseRGB, "clear inside clip");
pixel(dst, 2, 1, 64, baseRGB, "ignore source clip and face");
pixel(dst, 3, 1, 128, baseRGB, "outside clip right");
pixel(dst, 1, 2, 128, baseRGB, "outside clip bottom");

reset(); dst.clipAlphaRect(0, 0, src, -1, -1, 3, 3, 0);
pixel(dst, 0, 0, 0, baseRGB, "negative source clear");
pixel(dst, 1, 1, 64, baseRGB, "negative source alignment");
pixel(dst, 3, 2, 0, baseRGB, "clear suffix");
reset(); src.setMaskPixel(1, 1, 255);
dst.clipAlphaRect(-1, -1, src, 0, 0, 3, 3);
pixel(dst, 0, 0, 128, baseRGB, "negative destination alignment");
pixel(dst, 1, 1, 64, baseRGB, "negative destination extent");
pixel(dst, 2, 1, 128, baseRGB, "omitted clear");
reset(); dst.clipAlphaRect(0, 0, src, 3, 2, 4, 3, 0);
pixel(dst, 0, 0, 64, baseRGB, "source right/bottom bounds");
pixel(dst, 1, 0, 0, baseRGB, "source right clipped");
pixel(dst, 0, 1, 0, baseRGB, "source bottom clipped");

for(var clear = -1; clear <= 256; clear += 257) {
    reset(); dst.clipAlphaRect(1, 0, src, 0, 0, 1, 1, clear);
    pixel(dst, 0, 0, 128, baseRGB, "out of range clear ignored");
}
reset(); dst.clipAlphaRect(1, 0, src, 0, 0, 1, 1, void);
pixel(dst, 0, 0, 128, baseRGB, "void clear ignored");
dst.clipAlphaRect(2147483647, 2147483647, src, -2147483647-1, 0, 2147483647, 1, 23);
pixel(dst, 3, 2, 23, baseRGB, "overflow-safe disjoint clear");
dst.clipAlphaRect(-2147483647-1, 0, src, 0, 0, 2147483647, 1);
pixel(dst, 0, 0, 23, baseRGB, "disjoint without clear");

reset(); dst.setMaskPixel(0, 0, 255);
dst.clipAlphaRect(1, 0, dst, 0, 0, 3, 1, 0);
pixel(dst, 0, 0, 0, baseRGB, "self-copy clear");
pixel(dst, 1, 0, 128, baseRGB, "self-copy source before clear");
pixel(dst, 2, 0, 64, baseRGB, "self-copy overlap");
pixel(dst, 3, 0, 64, baseRGB, "self-copy not cascading");
reset(); dst.assignImages(src);
dst.clipAlphaRect(0, 0, src, 0, 0, 4, 3);
pixel(dst, 0, 0, 64, 0, "shared image destination");
pixel(src, 0, 0, 128, 0, "copy-on-write source");

reset(); dst.type = ltAddAlpha; dst.face = dfAddAlpha;
dst.fillRect(0, 0, 4, 3, 0x80402010);
var addRGB = dst.getMainPixel(0, 0);
var halfRGB = (addRGB >> 1) & 0x7f7f7f;
src.setMaskPixel(0, 0, 255); src.setMaskPixel(2, 0, 0);
dst.clipAlphaRect(0, 0, src, 0, 0, 3, 1, 128);
pixel(dst, 0, 0, 128, addRGB, "premultiplied full");
pixel(dst, 1, 0, 64, halfRGB, "premultiplied half");
pixel(dst, 2, 0, 0, 0, "premultiplied zero");
pixel(dst, 3, 2, 64, halfRGB, "premultiplied clear scales RGB");
dst.clipAlphaRect(10, 10, src, 0, 0, 1, 1, 0);
pixel(dst, 3, 2, 0, 0, "premultiplied disjoint clear");

var threw = false;
try { dst.clipAlphaRect(0, 0, src, 0, 0, 0, 1); } catch(e) { threw = true; }
check(threw, "zero width rejected");
threw = false;
try { dst.clipAlphaRect(0, 0, null, 0, 0, 1, 1); } catch(e) { threw = true; }
check(threw, "null source rejected");
threw = false;
try { dst.clipAlphaRect(0, 0); } catch(e) { threw = true; }
check(threw, "missing arguments rejected");
Debug.message("LAYER_ALPHA_SMOKE_OK");
} catch(e) {
    Debug.message("LAYER_ALPHA_SMOKE_FAILED: " + (typeof e == "String" ? e : e.message));
}
global.exitTimer = new Timer(function() {
    exitTimer.enabled = false;
    System.exit();
}, "");
exitTimer.interval = 100;
exitTimer.enabled = true;
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', type=Path)
    parser.add_argument('--generate', type=Path)
    args = parser.parse_args()
    if args.generate:
        args.generate.mkdir(parents=True, exist_ok=True)
        (args.generate / 'startup.tjs').write_text(SCRIPT, encoding='utf-8')
        return
    if not args.engine:
        parser.error('--engine or --generate is required')
    engine = args.engine.resolve()
    with tempfile.TemporaryDirectory(prefix='krkr-layer-alpha-') as temp:
        game = Path(temp)
        (game / 'startup.tjs').write_text(SCRIPT, encoding='utf-8')
        try:
            result = subprocess.run([str(engine), '--game=' + str(game)],
                                    cwd=engine.parent, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, timeout=60)
        except subprocess.TimeoutExpired as error:
            print((error.stdout or b'').decode('utf-8', errors='replace'))
            raise
        output = result.stdout.decode('utf-8', errors='replace')
        print(output)
        if result.returncode or 'LAYER_ALPHA_SMOKE_OK' not in output:
            raise SystemExit(f'Layer alpha smoke failed (exit {result.returncode})')


if __name__ == '__main__':
    main()
