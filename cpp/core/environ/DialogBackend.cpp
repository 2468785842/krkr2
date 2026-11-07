#include "DialogBackend.h"

#include "Platform.h"
#include "TVPDialog.h"

namespace {

    class NativeDialogBackend final : public ITVDialogBackend {
    public:
        int showMessage(const std::string &text, const std::string &caption,
                        const std::vector<std::string> &buttons) override {
            std::vector<ttstr> ttButtons;
            ttButtons.reserve(buttons.size());
            for(const auto &button : buttons)
                ttButtons.emplace_back(button);

            const ttstr ttText(text);
            const ttstr ttCaption(caption);
            if(ttButtons.empty())
                return TVPShowSimpleMessageBox(ttText, ttCaption);
            return TVPShowSimpleMessageBox(ttText, ttCaption, ttButtons);
        }

        int showYesNo(const std::string &text,
                      const std::string &caption) override {
            return TVPShowSimpleMessageBoxYesNo(ttstr(text), ttstr(caption));
        }
    };

    NativeDialogBackend g_nativeDialogBackend;

} // namespace

void TVPInitNativeDialogBackend() {
    TVPSetDialogBackend(&g_nativeDialogBackend);
}
