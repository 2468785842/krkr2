//
// Created by LiDong on 2025/8/24.
// TODO: implement method
//
#include <string>

#include <algorithm>
#include <CoreFoundation/CoreFoundation.h>

#include <mach-o/dyld.h>
#include <mach/task.h>
#include <mach/vm_statistics.h>
#include <mach/mach_host.h>
#include <mach/mach_init.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/sysctl.h>

#include <vector>
#import <CoreServices/CoreServices.h>
#import <Foundation/Foundation.h>

#include "EventIntf.h"
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

std::vector<std::string> GetAccessibleDirectories() {
    std::vector<std::string> result;

    // 获取应用程序支持目录
    CFURLRef appSupportDir = nullptr;
    FSRef fsRef;
    char path[PATH_MAX];

    // 获取文档目录
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    if (paths && [paths count] > 0) {
        NSString* documentsPath = [paths objectAtIndex:0];
        result.push_back([documentsPath UTF8String]);
    }
    
    if (FSFindFolder(kUserDomain, kApplicationSupportFolderType, kCreateFolder, &fsRef) == noErr) {
        CFURLRef url = CFURLCreateFromFSRef(kCFAllocatorDefault, &fsRef);
        if (url && CFURLGetFileSystemRepresentation(url, true, (UInt8*)path, PATH_MAX)) {
            result.push_back(path);
        }
        if (url) CFRelease(url);
    }

    // 获取桌面目录
    paths = NSSearchPathForDirectoriesInDomains(NSDesktopDirectory, NSUserDomainMask, YES);
    if (paths && [paths count] > 0) {
        NSString* desktopPath = [paths objectAtIndex:0];
        result.push_back([desktopPath UTF8String]);
    }

    return result;
}

std::vector<std::string> TVPGetDriverPath() {
    return GetAccessibleDirectories();
}

void TVPGetMemoryInfo(TVPMemoryInfo &m) {
    // 初始化所有字段为0
    m.MemTotal = 0;
    m.MemFree = 0;
    m.SwapTotal = 0;
    m.SwapFree = 0;
    m.VirtualTotal = 0;
    m.VirtualUsed = 0;

    // 获取物理内存总量
    int mib[2];
    int64_t total_memory;
    size_t length = sizeof(total_memory);

    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    if (sysctl(mib, 2, &total_memory, &length, NULL, 0) == 0) {
        m.MemTotal = total_memory / 1024; // 转换为KB
    }

    // 获取内存统计信息
    vm_size_t page_size;
    vm_statistics64_data_t vm_stats;
    mach_port_t mach_port = mach_host_self();
    mach_msg_type_number_t count = sizeof(vm_stats) / sizeof(natural_t);

    if (host_page_size(mach_port, &page_size) == KERN_SUCCESS &&
        host_statistics64(mach_port, HOST_VM_INFO, (host_info64_t)&vm_stats, &count) == KERN_SUCCESS) {
        // 计算可用内存 (空闲内存 + 非活跃内存)
        int64_t free_memory = (vm_stats.free_count + vm_stats.inactive_count) * page_size;
        m.MemFree = free_memory / 1024; // 转换为KB
    }

    // 获取交换空间信息
    xsw_usage swap_usage;
    size_t swap_size = sizeof(swap_usage);
    mib[0] = CTL_VM;
    mib[1] = VM_SWAPUSAGE;

    if (sysctl(mib, 2, &swap_usage, &swap_size, NULL, 0) == 0) {
        m.SwapTotal = swap_usage.xsu_total / 1024; // 转换为KB
        m.SwapFree = (swap_usage.xsu_total - swap_usage.xsu_used) / 1024; // 转换为KB
    }

    // macOS 没有直接的 Vmalloc 概念，但可以获取虚拟内存信息
    // 这里使用进程虚拟内存大小作为近似值
    struct task_basic_info_64 info;
    mach_msg_type_number_t info_count = TASK_BASIC_INFO_64_COUNT;

    if (task_info(mach_task_self(), TASK_BASIC_INFO_64, (task_info_t)&info, &info_count) == KERN_SUCCESS) {
        m.VirtualTotal = info.virtual_size / 1024; // 转换为KB
        m.VirtualUsed = info.resident_size / 1024; // 转换为KB
    }
}

void TVPRelinquishCPU() {
    sched_yield();
}

void TVPSendToOtherApp(const std::string &filename) {}

bool TVPCheckStartupArg() {
    return false;
}

void TVPControlAdDialog(int adType, int arg1, int arg2) {

}

void TVPExitApplication(int code) {
    // clear some static data for memory leak detect
    TVPDeliverCompactEvent(TVP_COMPACT_LEVEL_MAX);
    exit(code);
}

void TVPForceSwapBuffer() {

}

bool TVPWriteDataToFile(const ttstr &filepath, const void *data,
                        unsigned int len) {

}

bool TVPCheckStartupPath(const std::string &path) { return true; }

std::string TVPGetDefaultFileDir() {
    char buffer[PATH_MAX];
    uint32_t size = sizeof(buffer);

    // 使用_NSGetExecutablePath获取可执行文件路径
    if (_NSGetExecutablePath(buffer, &size) != 0) {
        // 缓冲区不足时返回错误
        return "";
    }

    // 解析路径中的符号链接
    char resolved[PATH_MAX];
    if (realpath(buffer, resolved) != nullptr) {
        return {resolved};
    }

    return {buffer};
}

std::vector<std::string> TVPGetAppStoragePath() {
    std::vector<std::string> ret;
    ret.emplace_back(TVPGetDefaultFileDir());
    return ret;
}

tjs_int TVPGetSelfUsedMemory() {
    mach_task_basic_info info{};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;

    kern_return_t kr = task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                                (task_info_t)&info, &count);
    if (kr != KERN_SUCCESS) {
        return -1;
    }

    // 返回驻留内存大小（字节转换为MB）
    return info.resident_size / (1024 * 1024);
}

std::string TVPGetCurrentLanguage() {
    // 获取首选语言列表
    CFArrayRef preferredLanguages = CFLocaleCopyPreferredLanguages();
    if (preferredLanguages == nullptr || CFArrayGetCount(preferredLanguages) == 0) {
        return "en_US"; // 默认值
    }

    // 获取首选语言
    CFStringRef preferredLanguage = (CFStringRef)CFArrayGetValueAtIndex(preferredLanguages, 0);

    // 转换为 C 字符串
    char buffer[256];
    CFStringGetCString(preferredLanguage, buffer, sizeof(buffer), kCFStringEncodingUTF8);

    // 转换为 C++ 字符串
    std::string localeStr(buffer);

    // 释放资源
    CFRelease(preferredLanguages);

    // 处理语言代码格式（将连字符替换为下划线）
    std::replace(localeStr.begin(), localeStr.end(), '-', '_');

    // 分割语言和国家代码
    size_t underscore_pos = localeStr.find('_');
    if (underscore_pos != std::string::npos) {
        std::string language = localeStr.substr(0, underscore_pos);
        std::string country = localeStr.substr(underscore_pos + 1);

        // 将国家代码转为小写
        std::transform(country.begin(), country.end(), country.begin(), ::tolower);

        return language + "_" + country;
    }

    // 如果没有国家代码，直接返回语言代码
    return localeStr;
}

void TVPProcessInputEvents() {

}

int TVPShowSimpleInputBox(ttstr &text, const ttstr &caption,
                          const ttstr &prompt,
                          const std::vector<ttstr> &vecButtons) {

}

tjs_uint32 TVPGetRoughTickCount32() {
    tjs_uint32 uptime = 0;
    timespec on{};
    if(clock_gettime(CLOCK_MONOTONIC, &on) == 0)
        uptime = on.tv_sec * 1000 + on.tv_nsec / 1000000;
    return uptime;
}


tjs_int TVPGetSystemFreeMemory() {
    // 使用 Mach API 获取内存统计
    vm_statistics64_data_t vm_stats;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    kern_return_t kern_return = host_statistics64(mach_host_self(),
                                                HOST_VM_INFO64,
                                                (host_info64_t)&vm_stats,
                                                &count);

    if (kern_return != KERN_SUCCESS) {
        return -1;
    }

    // 获取页大小
    vm_size_t page_size;
    host_page_size(mach_host_self(), &page_size);

    // 计算可用内存 (空闲内存 + 非活跃内存)
    natural_t free_memory = (vm_stats.free_count + vm_stats.inactive_count) * page_size;

    // 转换为 MB
    return free_memory / (1024 * 1024);
}

int TVPShowSimpleMessageBox(const ttstr &text, const ttstr &caption,
                            const std::vector<ttstr> &vecButtons) {

}

int TVPShowSimpleMessageBox(const char *pszText, const char *pszTitle,
                            unsigned int nButton, const char **btnText) {
    return 0;
}

std::string TVPGetPackageVersionString() {
    return "macos";
}

bool TVP_stat(const char *name, tTVP_stat &s) {
    struct stat t{};
    if(stat(name, &t) != 0) {
        return false;
    }

    s.st_mode = t.st_mode;
    s.st_size = t.st_size;
    s.st_atime = t.st_atimespec.tv_sec; // 最后访问时间（秒）
    s.st_mtime = t.st_mtimespec.tv_sec; // 最后修改时间（秒）
    s.st_ctime = t.st_ctimespec.tv_sec; // 最后状态更改时间（秒）

    return true; // 成功
}

bool TVP_stat(const tjs_char *name, tTVP_stat &s) {
    return TVP_stat(ttstr{ name }.AsStdString().c_str(), s);
}

void TVP_utime(const char *name, time_t modtime) {
    timeval mt[2];
    mt[0].tv_sec = modtime;
    mt[0].tv_usec = 0;
    mt[1].tv_sec = modtime;
    mt[1].tv_usec = 0;
    utimes(name, mt);
}

