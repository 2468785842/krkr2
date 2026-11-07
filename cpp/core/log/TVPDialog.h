#pragma once

#include <string>
#include <vector>

struct ITVDialogBackend {
    virtual int showMessage(const std::string &text, const std::string &caption,
                            const std::vector<std::string> &buttons) = 0;
    virtual int showYesNo(const std::string &text,
                          const std::string &caption) = 0;
    virtual ~ITVDialogBackend() = default;
};

void TVPSetDialogBackend(ITVDialogBackend *backend);
ITVDialogBackend *TVPGetDialogBackend();

// Returns backend result, or 0 when backend is nullptr (headless default).
int TVPShowDialogMessage(const std::string &text, const std::string &caption,
                         const std::vector<std::string> &buttons = {});

int TVPShowDialogYesNo(const std::string &text, const std::string &caption);
