#include "TVPDialog.h"

namespace {

    ITVDialogBackend *g_dialogBackend = nullptr;

} // namespace

void TVPSetDialogBackend(ITVDialogBackend *backend) {
    g_dialogBackend = backend;
}

ITVDialogBackend *TVPGetDialogBackend() { return g_dialogBackend; }

int TVPShowDialogMessage(const std::string &text, const std::string &caption,
                         const std::vector<std::string> &buttons) {
    if(auto *backend = TVPGetDialogBackend())
        return backend->showMessage(text, caption, buttons);
    return 0;
}

int TVPShowDialogYesNo(const std::string &text, const std::string &caption) {
    if(auto *backend = TVPGetDialogBackend())
        return backend->showYesNo(text, caption);
    return 1;
}
