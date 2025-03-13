#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include "EventIntf.h"
#include "Platform.h"

void TVPGetMemoryInfo(TVPMemoryInfo &m) {
    /* to read /proc/meminfo */
    FILE *meminfo;
    char buffer[100] = { 0 };
    char *end;
    int found = 0;

    /* Try to read /proc/meminfo, bail out if fails */
    meminfo = fopen("/proc/meminfo", "r");

    static const char pszMemFree[] = "MemFree:", pszMemTotal[] = "MemTotal:",
                      pszSwapTotal[] = "SwapTotal:",
                      pszSwapFree[] = "SwapFree:",
                      pszVmallocTotal[] = "VmallocTotal:",
                      pszVmallocUsed[] = "VmallocUsed:";

    /* Read each line untill we got all we ned */
    while(fgets(buffer, sizeof(buffer), meminfo)) {
        if(strstr(buffer, pszMemFree) == buffer) {
            m.MemFree = strtol(buffer + sizeof(pszMemFree), &end, 10);
            found++;
        } else if(strstr(buffer, pszMemTotal) == buffer) {
            m.MemTotal = strtol(buffer + sizeof(pszMemTotal), &end, 10);
            found++;
        } else if(strstr(buffer, pszSwapTotal) == buffer) {
            m.SwapTotal = strtol(buffer + sizeof(pszSwapTotal), &end, 10);
            found++;
            break;
        } else if(strstr(buffer, pszSwapFree) == buffer) {
            m.SwapFree = strtol(buffer + sizeof(pszSwapFree), &end, 10);
            found++;
            break;
        } else if(strstr(buffer, pszVmallocTotal) == buffer) {
            m.VirtualTotal = strtol(buffer + sizeof(pszVmallocTotal), &end, 10);
            found++;
            break;
        } else if(strstr(buffer, pszVmallocUsed) == buffer) {
            m.VirtualUsed = strtol(buffer + sizeof(pszVmallocUsed), &end, 10);
            found++;
            break;
        }
    }
    fclose(meminfo);
}

#include <sched.h>
void TVPRelinquishCPU() { sched_yield(); }

void TVP_utime(const char *name, time_t modtime) {
    timeval mt[2];
    mt[0].tv_sec = modtime;
    mt[0].tv_usec = 0;
    mt[1].tv_sec = modtime;
    mt[1].tv_usec = 0;
    utimes(name, mt);
}

#ifdef LINUX
#include <filesystem>
#include <gtk/gtk.h>
#include <fstream>
#include <common/Defer.h>
#include <sys/stat.h>
#include <unistd.h>
#include <spdlog/spdlog.h>
#include "StorageImpl.h"
#undef st_atime
#undef st_mtime
#undef st_ctime

// FIXME: 这是临时方案，以后需要重构模块化

tjs_int TVPGetSystemFreeMemory() {
    struct sysinfo info;
    if(sysinfo(&info) == -1) {
        return -1; // 调用失败
    }
    return (info.freeram * info.mem_unit) / (1024 * 1024); // 转换为 MB
}

tjs_int TVPGetSelfUsedMemory() {
    std::ifstream statm{ "/proc/self/statm" };
    tjs_int pages = 0;
    statm >> pages; // 第一个字段是总内存页数
    return (pages * sysconf(_SC_PAGESIZE)) / (1024 * 1024); // 转换为 MB
}

std::string TVPGetPackageVersionString() { return "linux"; }

bool TVPCheckStartupPath(const std::string &path) { return true; }

void TVPControlAdDialog(int adType, int arg1, int arg2) {}
void TVPForceSwapBuffer() {}

std::string TVPGetCurrentLanguage() {
    const char *lang_env = std::getenv("LANG");
    if(!lang_env) {
        lang_env = std::getenv("LC_ALL");
        if(!lang_env) {
            lang_env = std::getenv("LC_MESSAGES");
            if(!lang_env) {
                return "en_US"; // 默认值
            }
        }
    }

    std::string locale(lang_env);
    size_t dot_pos = locale.find('.');
    if(dot_pos != std::string::npos) {
        locale = locale.substr(0, dot_pos); // 移除编码部分（如 .UTF-8）
    }

    // 分割语言和国家（例如 zh_CN → zh 和 CN）
    size_t underscore_pos = locale.find('_');
    if(underscore_pos != std::string::npos) {
        std::string language = locale.substr(0, underscore_pos);
        std::string country = locale.substr(underscore_pos + 1);

        // 将国家代码转为小写（如 CN → cn）
        for(char &c : country) {
            if(c >= 'A' && c <= 'Z') {
                c += 'a' - 'A';
            }
        }
        return language + "_" + country; // 格式：zh_cn
    }

    return locale; // 如果没有国家代码（如 "en"）
}

int TVPShowSimpleMessageBox(const ttstr &text, const ttstr &caption,
                            const std::vector<ttstr> &vecButtons) {
    GtkWidget *dialog = nullptr;
    DEFER {
        if(dialog) {
            gtk_widget_destroy(dialog);
        }
    };

    switch(vecButtons.size()) {
        case 1:
            dialog = gtk_message_dialog_new(
                NULL, // 父窗口
                GTK_DIALOG_MODAL, // 模态对话框
                GTK_MESSAGE_INFO, // 消息类型（信息）
                GTK_BUTTONS_OK, // 按钮类型
                "%s", text.AsStdString().c_str() // 消息内容
            );
            gtk_window_set_title(GTK_WINDOW(dialog),
                                 caption.AsStdString().c_str());
            gtk_dialog_run(GTK_DIALOG(dialog));
            return 0;
            break;
        case 2:
            dialog = gtk_message_dialog_new(
                NULL, // 父窗口
                GTK_DIALOG_MODAL, // 模态对话框
                GTK_MESSAGE_INFO, // 消息类型（信息）
                GTK_BUTTONS_YES_NO, // 按钮类型
                "%s", text.AsStdString().c_str() // 消息内容
            );
            gtk_window_set_title(GTK_WINDOW(dialog),
                                 caption.AsStdString().c_str());
            int result = gtk_dialog_run(GTK_DIALOG(dialog));
            switch(result) {
                case GTK_RESPONSE_YES:
                    return 0; // YES 0
                default:
                    return 1; // NO 1
            }
            break;
    }
    return -1;
}

extern "C" int TVPShowSimpleMessageBox(const char *pszText,
                                       const char *pszTitle, u_int nButton,
                                       const char **btnText) {
    std::vector<ttstr> vecButtons{};
    for(u_int i = 0; i < nButton; ++i) {
        vecButtons.emplace_back(btnText[i]);
    }
    return TVPShowSimpleMessageBox(pszText, pszTitle, vecButtons);
}

bool TVPCreateFolders(const ttstr &folder);

static bool _TVPCreateFolders(const ttstr &folder) {
    // create directories along with "folder"
    if(folder.IsEmpty())
        return true;

    if(TVPCheckExistentLocalFolder(folder))
        return true; // already created

    const tjs_char *p = folder.c_str();
    tjs_int i = folder.GetLen() - 1;

    if(p[i] == TJS_W(':'))
        return true;

    while(i >= 0 && (p[i] == TJS_W('/') || p[i] == TJS_W('\\')))
        i--;

    if(i >= 0 && p[i] == TJS_W(':'))
        return true;

    for(; i >= 0; i--) {
        if(p[i] == TJS_W(':') || p[i] == TJS_W('/') || p[i] == TJS_W('\\'))
            break;
    }

    ttstr parent(p, i + 1);
    if(!TVPCreateFolders(parent))
        return false;

    return !std::filesystem::create_directory(folder.AsStdString().c_str());
}

int TVPShowSimpleInputBox(ttstr &text, const ttstr &caption,
                          const ttstr &prompt,
                          const std::vector<ttstr> &vecButtons) {
    spdlog::get("core")->warn("linux platform simple input box not implement");
    return 0;
}

bool TVPCreateFolders(const ttstr &folder) {
    if(folder.IsEmpty())
        return true;

    const tjs_char *p = folder.c_str();
    tjs_int i = folder.GetLen() - 1;

    if(p[i] == TJS_W(':'))
        return true;

    if(p[i] == TJS_W('/') || p[i] == TJS_W('\\'))
        i--;

    return _TVPCreateFolders(ttstr(p, i + 1));
}
bool TVP_stat(const char *name, tTVP_stat &s) {
    struct stat t;
    if(stat(name, &t) != 0) {
        return false;
    }

    s.st_mode = t.st_mode;
    s.st_size = t.st_size;
    s.st_atime = t.st_atim.tv_sec; // 最后访问时间（秒）
    s.st_mtime = t.st_mtim.tv_sec; // 最后修改时间（秒）
    s.st_ctime = t.st_ctim.tv_sec; // 最后状态更改时间（秒）

    return true; // 成功
}

bool TVP_stat(const tjs_char *name, tTVP_stat &s) {
    return TVP_stat(ttstr{ name }.AsStdString().c_str(), s);
}

tjs_uint32 TVPGetRoughTickCount32() {
    tjs_uint32 uptime = 0;
    struct timespec on;
    if(clock_gettime(CLOCK_MONOTONIC, &on) == 0)
        uptime = on.tv_sec * 1000 + on.tv_nsec / 1000000;
    return uptime;
}

void TVPExitApplication(int code) {
    // clear some static data for memory leak detect
    TVPDeliverCompactEvent(TVP_COMPACT_LEVEL_MAX);
    exit(code);
}

bool TVPCheckStartupArg() { return false; }

void TVPProcessInputEvents() {}

bool TVPDeleteFile(const std::string &filename) {
    return unlink(filename.c_str()) == 0;
}

bool TVPRenameFile(const std::string &from, const std::string &to) {
    tjs_int ret = rename(from.c_str(), to.c_str());
    return !ret;
}

void TVPSendToOtherApp(const std::string &filename) {}

std::vector<std::string> TVPGetDriverPath() { return { "/" }; }

std::string TVPGetDefaultFileDir() {
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if(len == -1) {
        // 错误处理（例如抛出异常或返回空字符串）
        return "";
    }
    buffer[len] = '\0'; // 手动添加终止符
    // symbol link
    char resolved[PATH_MAX];
    if(realpath(buffer, resolved) != nullptr) {
        return std::string(resolved);
    }
    return std::string(buffer);
}

std::vector<std::string> TVPGetAppStoragePath() {
    std::vector<std::string> ret;
    ret.emplace_back(TVPGetDefaultFileDir());
    return ret;
}

bool TVPWriteDataToFile(const ttstr &filepath, const void *data,
                        unsigned int len) {
    FILE *handle = fopen(filepath.AsStdString().c_str(), "wb");
    if(handle) {
        bool ret = fwrite(data, 1, len, handle) == len;
        fclose(handle);
        return ret;
    }
    return false;
}

#endif
