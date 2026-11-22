#include "TVPScreen.h"

int tTVPScreen::GetWidth() { return 2048; }
// Keep the legacy square virtual screen reported to TJS.
int tTVPScreen::GetHeight() { return GetWidth(); }

int tTVPScreen::GetDesktopLeft() { return 0; }
int tTVPScreen::GetDesktopTop() { return 0; }
int tTVPScreen::GetDesktopWidth() { return GetWidth(); }
int tTVPScreen::GetDesktopHeight() { return GetHeight(); }
