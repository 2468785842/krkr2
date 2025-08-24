//
// Created by LiDong on 2025/8/24.
// TODO: implement method
//
#include <string>

#include "Platform.h"
#include "tjsString.h"

bool TVPDeleteFile(const std::string &filename) {
    return false;
}

bool TVPRenameFile(const std::string &from, const std::string &to) {
    return false;
}

bool TVPCreateFolders(const TJS::ttstr &folder) {
    return false;
}

std::vector<std::string> TVPGetDriverPath() {
    return {};
}

void TVPGetMemoryInfo(TVPMemoryInfo &m) {

}

void TVPRelinquishCPU() {

}

void TVPSendToOtherApp(const std::string &filename) {}

bool TVPCheckStartupArg() {
    return false;
}

void TVPControlAdDialog(int adType, int arg1, int arg2) {

}

void TVPExitApplication(int) {

}

void TVPForceSwapBuffer() {

}

bool TVPWriteDataToFile(const ttstr &filepath, const void *data,
                        unsigned int len) {

}

bool TVPCheckStartupPath(const std::string &path) { return true; }


std::vector<std::string> TVPGetAppStoragePath() {
    return {};
}

tjs_int TVPGetSelfUsedMemory() {
    return 0;
}

std::string TVPGetCurrentLanguage() {
    return "en_us";
}

void TVPProcessInputEvents() {

}

int TVPShowSimpleInputBox(ttstr &text, const ttstr &caption,
                          const ttstr &prompt,
                          const std::vector<ttstr> &vecButtons) {

}

tjs_uint32 TVPGetRoughTickCount32() {
    return 0;
}

tjs_int TVPGetSystemFreeMemory() {
    return 2000;
}

int TVPShowSimpleMessageBox(const ttstr &text, const ttstr &caption,
                            const std::vector<ttstr> &vecButtons) {

}

int TVPShowSimpleMessageBox(const char *pszText, const char *pszTitle,
                            unsigned int nButton, const char **btnText) {
    return 0;
}

std::string TVPGetPackageVersionString() {
    return "v1.0.0";
}

bool TVP_stat(const tjs_char *name, tTVP_stat &s) {
    return false;
}

bool TVP_stat(char const*, tTVP_stat&) {
    return false;
}

void TVP_utime(const char *name, time_t modtime) {

}