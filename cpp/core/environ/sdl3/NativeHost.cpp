#include "tjsCommHead.h"
#include "NativeDialogs.h"
#include "Platform.h"

ttstr TVPGetPlatformName() { return "Linux"; }
ttstr TVPGetOSName() { return TVPGetPlatformName(); }

void TVPShowPopMenu(tTJSNI_MenuItem *menu) { krkr::sdl::ShowPopupMenu(menu); }

std::string TVPShowFileSelector(const std::string &title,
                                const std::string &filename,
                                std::string directory, bool save) {
    return krkr::sdl::ShowFileSelector(title, filename, directory, save);
}
