//
// Created by 33298 on 2025/8/14.
//
#define NCB_MODULE_NAME TJS_W("krkrsteam.dll")

#include "ncbind.hpp"
#include "tjs.h"

// 成就信息结构
struct AchievementInfo {
    std::string ach; // 成就标识名
    std::string name; // 显示名称
    std::string desc; // 描述
    bool hidden; // 是否隐藏
    bool achieved; // 是否已解锁
    tjs_int unlockTime; // 解锁时间（Unix 时间戳）
};

// 云存储容量信息结构
struct CloudQuotaInfo {
    tjs_int total; // 总容量
    tjs_int available; // 可用容量
};

// 云存储文件信息结构
struct CloudFileInfo {
    std::string filename; // 文件名
    tjs_int size; // 文件大小
    tjs_int time; // 时间戳
};

class Steam {
public:
    // 成就
    static bool initialized; // 成就信息是否已初始化
    static int achievementsCount; // 成就数量

    // 云存储
    static bool cloudEnabled; // Steam 云存储是否启用
    static int cloudFileCount; // Steam 云存储的文件数量


    // DLC
    static int dlcCount; // DLC的数量
    // 成就
    static tjs_error requestInitialize(tTJSVariant *r, tjs_int, tTJSVariant **,
                                       iTJSDispatch2 *) {
        if(r) {
            *r = int(1); // 假设初始化成功
        }
        return TJS_S_OK;
    }
    static int getInitialized() { return 1; }
    static tjs_error setInitialized(tTJSVariant *r, tjs_int, tTJSVariant **,
                                    iTJSDispatch2 *) {
        return TJS_S_OK;
    }

    static int getAchievementsCount() { return 0; }
    static tjs_error setAchievementsCount(tTJSVariant *r, tjs_int,
                                          tTJSVariant **, iTJSDispatch2 *) {
        return TJS_S_OK;
    }

    static tjs_error getAchievement(tTJSVariant *r, tjs_int n, tTJSVariant **,
                                    iTJSDispatch2 *) {
        if(r) {
            AchievementInfo info = {
                "ach1", "Achievement 1", "Description 1", false,
                true,   1633072800
            };
            *r = info.name; // 示例：返回成就名称
        }
        return TJS_S_OK;
    }

    static tjs_error setAchievement(tTJSVariant *r, tjs_int n, tTJSVariant **,
                                    iTJSDispatch2 *) {
        if(r) {
            *r = int(1); // 假设设置成功
        }
        return TJS_S_OK;
    }

    static tjs_error clearAchievement(tTJSVariant *r, tjs_int n, tTJSVariant **,
                                      iTJSDispatch2 *) {
        if(r) {
            *r = int(1); // 假设清除成功
        }
        return TJS_S_OK;
    }

    // 其他信息
    static tjs_error getLanguage(tTJSVariant *r, tjs_int, tTJSVariant **,
                                 iTJSDispatch2 *) {
        if(r) {
            *r = "English";
        }
        return TJS_S_OK;
    }

    // 云存储
    static bool getCloudEnabled() { return true; }
    static tjs_error setCloudEnabled(tTJSVariant *r, tjs_int, tTJSVariant **,
                                     iTJSDispatch2 *) {
        return TJS_S_OK;
    }

    static tjs_error getCloudQuota(tTJSVariant *r, tjs_int, tTJSVariant **,
                                   iTJSDispatch2 *) {
        if(r) {
            CloudQuotaInfo info = {
                1024 * 1024 * 1024, 1024 * 1024 * 1024
            }; // 假设总容量和可用容量为1GB
            *r = int(info.total); // 示例：返回总容量
        }
        return TJS_S_OK;
    }

    static tjs_error getCloudFileInfo(tTJSVariant *r, tjs_int n, tTJSVariant **,
                                      iTJSDispatch2 *) {
        if(r) {
            CloudFileInfo info = { "file1.txt", 1024,
                                   1633072800 }; // 示例文件信息
            *r = info.filename; // 示例：返回文件名
        }
        return TJS_S_OK;
    }
    static int getCloudFileCount() { return 0; }

    static tjs_error deleteCloudFile(tTJSVariant *r, tjs_int, tTJSVariant **,
                                     iTJSDispatch2 *) {
        if(r) {
            *r = int(1); // 假设删除成功
        }
        return TJS_S_OK;
    }

    static tjs_error copyCloudFile(tTJSVariant *r, tjs_int, tTJSVariant **,
                                   iTJSDispatch2 *) {
        if(r) {
            *r = int(1); // 假设复制成功
        }
        return TJS_S_OK;
    }

    // 截图
    static tjs_error triggerScreenshot(tTJSVariant *, tjs_int, tTJSVariant **,
                                       iTJSDispatch2 *) {
        return TJS_S_OK;
    }

    static tjs_error hookScreenshots(tTJSVariant *, tjs_int, tTJSVariant **,
                                     iTJSDispatch2 *) {
        return TJS_S_OK;
    }

    static tjs_error writeScreenshot(tTJSVariant *, tjs_int, tTJSVariant **,
                                     iTJSDispatch2 *) {
        return TJS_S_OK;
    }

    // 配信状态
    static tjs_error isBroadcasting(tTJSVariant *r, tjs_int, tTJSVariant **,
                                    iTJSDispatch2 *) {
        if(r) {
            *r = int(0); // 假设没有在直播
        }
        return TJS_S_OK;
    }

    static tjs_error hookBroadcasting(tTJSVariant *, tjs_int, tTJSVariant **,
                                      iTJSDispatch2 *) {
        return TJS_S_OK;
    }

    // DLC
    static tjs_error isIsSubscribedApp(tTJSVariant *r, tjs_int, tTJSVariant **,
                                       iTJSDispatch2 *) {
        if(r) {
            *r = int(1); // 假设已订阅
        }
        return TJS_S_OK;
    }

    static tjs_error ssDlcInstalled(tTJSVariant *r, tjs_int, tTJSVariant **,
                                    iTJSDispatch2 *) {
        if(r) {
            *r = int(1); // 假设已安装
        }
        return TJS_S_OK;
    }

    static tjs_error getDLCCount(tTJSVariant *r, tjs_int, tTJSVariant **,
                                 iTJSDispatch2 *) {
        if(r) {
            *r = int(2); // 假设有2个DLC
        }
        return TJS_S_OK;
    }
    static int _getDlcsCount() { return 0; }

    static tjs_error getDLCData(tTJSVariant *r, tjs_int no, tTJSVariant **,
                                iTJSDispatch2 *) {
        if(r) {
            *r = "DLC1"; // 示例：返回DLC名称
        }
        return TJS_S_OK;
    }
};
bool Steam::initialized = false;
int Steam::achievementsCount = 0;
bool Steam::cloudEnabled = false;
int Steam::cloudFileCount = 0;
int Steam::dlcCount = 0;


NCB_REGISTER_CLASS(Steam) {

    RawCallback("getAchievement", &Steam::getAchievement, TJS_STATICMEMBER);
    RawCallback("setAchievement", &Steam::setAchievement, TJS_STATICMEMBER);
    RawCallback("clearAchievement", &Steam::clearAchievement, TJS_STATICMEMBER);

    RawCallback("getLanguage", &Steam::getLanguage, TJS_STATICMEMBER);

    Property("initialized", &Steam::getInitialized, int());
    Property("achievementsCount", &Steam::getAchievementsCount, int());
    Property("cloudEnabled", &Steam::getCloudEnabled, int());
    Property("cloudFileCount", &Steam::getCloudFileCount, int());
    Property("dlcCount", &Steam::_getDlcsCount, int());


    RawCallback("requestInitialize", &Steam::requestInitialize,
                TJS_STATICMEMBER);

    RawCallback("getCloudQuota", &Steam::getCloudQuota, TJS_STATICMEMBER);
    RawCallback("getCloudFileInfo", &Steam::getCloudFileInfo, TJS_STATICMEMBER);
    RawCallback("deleteCloudFile", &Steam::deleteCloudFile, TJS_STATICMEMBER);
    RawCallback("copyCloudFile", &Steam::copyCloudFile, TJS_STATICMEMBER);

    RawCallback("triggerScreenshot", &Steam::triggerScreenshot,
                TJS_STATICMEMBER);
    RawCallback("hookScreenshots", &Steam::hookScreenshots, TJS_STATICMEMBER);
    RawCallback("writeScreenshot", &Steam::writeScreenshot, TJS_STATICMEMBER);

    RawCallback("isBroadcasting", &Steam::isBroadcasting, TJS_STATICMEMBER);
    RawCallback("hookBroadcasting", &Steam::hookBroadcasting, TJS_STATICMEMBER);

    RawCallback("isIsSubscribedApp", &Steam::isIsSubscribedApp,
                TJS_STATICMEMBER);
    RawCallback("ssDlcInstalled", &Steam::ssDlcInstalled, TJS_STATICMEMBER);
    RawCallback("getDLCCount", &Steam::getDLCCount, TJS_STATICMEMBER);
    RawCallback("getDLCData", &Steam::getDLCData, TJS_STATICMEMBER);
}