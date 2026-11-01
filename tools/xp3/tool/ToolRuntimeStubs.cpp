#include "tjsCommHead.h"

#include "DebugIntf.h"
#include "EventIntf.h"
#include "SysInitIntf.h"
#include "TVPLog.h"

//---------------------------------------------------------------------------
// 工具链运行时桩：避免链接 environ / utils / 完整 event 模块
//---------------------------------------------------------------------------

void TVPAddLog(const ttstr &line, bool important) {
    if(important)
        G_EngineLog.warn(line.AsStdString());
    else
        G_EngineLog.info(line.AsStdString());
}

void TVPAddLog(const ttstr &line) { TVPAddLog(line, false); }

void TVPAddImportantLog(const ttstr &line) { TVPAddLog(line, true); }

//---------------------------------------------------------------------------
void TVPAddCompactEventHook(tTVPCompactEventCallbackIntf *) {}

//---------------------------------------------------------------------------
// 工具进程退出后由 OS 回收资源即可。若在静态析构阶段调用 XP3 的 AtExit
// 清理（TVPShutdownArchiveHandleCache），可能与 tTJSCriticalSection 的
// 析构顺序冲突，导致 mutex lock failed。
void TVPAddAtExitHandler(tjs_int /*pri*/, void(/*handler*/)) {}

//---------------------------------------------------------------------------
ttstr TVPGetMessageByLocale(const std::string &key) {
    if(key == "err_not_xp3_archive")
        return TJS_W("Not an XP3 archive: %1");
    return ttstr(key);
}
