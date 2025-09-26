#include "ClipboardIntf.h"

//---------------------------------------------------------------------------
// clipboard related functions
//---------------------------------------------------------------------------
bool TVPClipboardHasFormat(tTVPClipboardFormat format) {
    switch(format) {
        case cbfText: {
            bool result = false;
            return result; // ANSI text or UNICODE text
        }
        default:
            return false;
    }
}

void TVPClipboardSetText(const ttstr &text) {}

bool TVPClipboardGetText(ttstr &text) { return false; }