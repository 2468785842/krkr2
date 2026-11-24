#pragma once

#include <string>
#include <vector>

class tTJSNI_MenuItem;

namespace krkr::sdl {
    void PumpNativeDialogs();
    void CloseNativeDialogs();
    void ShowPopupMenu(tTJSNI_MenuItem *menu);
    std::string ShowFileSelector(const std::string &title,
                                 const std::string &filename,
                                 const std::string &directory, bool save);
    int ShowInputBox(std::string &text, const std::string &caption,
                     const std::string &prompt,
                     const std::vector<std::string> &buttons);
    int ShowMessage(const std::string &text, const std::string &caption,
                    const std::vector<std::string> &buttons);
} // namespace krkr::sdl
