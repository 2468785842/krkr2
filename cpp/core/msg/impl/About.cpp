//---------------------------------------------------------------------------
// Version constants and about string template
//---------------------------------------------------------------------------
#include "tjsCommHead.h"
#include "TVPVersion.h"

tjs_int TVPVersionMajor;
tjs_int TVPVersionMinor;
tjs_int TVPVersionRelease;
tjs_int TVPVersionBuild;

#ifndef WIDEN2
#define WIDEN2(x) TJS_W(x)
#define WIDEN(x) WIDEN2(x)
#endif

const tjs_char *TVPCompileDate = WIDEN(__DATE__);
const tjs_char *TVPCompileTime = WIDEN(__TIME__);

void TVPGetVersion() {
    static bool DoGet = true;
    if(DoGet) {
        DoGet = false;

        TVPVersionMajor = 2;
        TVPVersionMinor = 32;
        TVPVersionRelease = 2;
        TVPVersionBuild = 426;
    }
}

ttstr TVPReadAboutStringFromResource() {
    return TJS_W("Kirikiri2 Runtime Core version %1(TJS version %2)");
}
