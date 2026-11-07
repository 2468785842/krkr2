//---------------------------------------------------------------------------
// Version and about string formatting
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include <fmt/format.h>

#include "TVPError.h"
#include "TVPMessages.h"
#include "TVPVersion.h"

extern const tjs_char *TVPCompileDate;
extern const tjs_char *TVPCompileTime;
extern ttstr TVPReadAboutStringFromResource();

static TVPAboutLogAppender TVPAboutLogAppenderFunc = nullptr;

void TVPSetAboutLogAppender(TVPAboutLogAppender appender) {
    TVPAboutLogAppenderFunc = appender;
}

ttstr TVPGetAboutString() {
    TVPGetVersion();
    ttstr verstr{ fmt::format("{}.{}.{}.{}", TVPVersionMajor, TVPVersionMinor,
                              TVPVersionRelease, TVPVersionBuild) };

    ttstr tjsverstr{ fmt::format("{}.{}.{}", TJSVersionMajor, TJSVersionMinor,
                                 TJSVersionRelease) };

    ttstr result = TVPFormatMessage(TVPReadAboutStringFromResource().c_str(),
                                    verstr, tjsverstr);
    if(TVPAboutLogAppenderFunc)
        result += TVPAboutLogAppenderFunc();
    return result;
}

ttstr TVPGetVersionInformation() {
    TVPGetVersion();
    ttstr verstr{ fmt::format("{}.{}.{}.{}", TVPVersionMajor, TVPVersionMinor,
                              TVPVersionRelease, TVPVersionBuild) };

    ttstr tjsverstr{ fmt::format("{}.{}.{}", TJSVersionMajor, TJSVersionMinor,
                                 TJSVersionRelease) };

    ttstr str = TVPFormatMessage(TVPVersionInformation, verstr, tjsverstr);
    str.Replace(TJS_W("%DATE%"), ttstr(TVPCompileDate));
    str.Replace(TJS_W("%TIME%"), ttstr(TVPCompileTime));
    return str;
}

ttstr TVPGetVersionString() {
    TVPGetVersion();
    return fmt::format("{}.{}.{}.{}", TVPVersionMajor, TVPVersionMinor,
                       TVPVersionRelease, TVPVersionBuild);
}

void TVPGetSystemVersion(tjs_int &major, tjs_int &minor, tjs_int &release,
                         tjs_int &build) {
    TVPGetVersion();
    major = TVPVersionMajor;
    minor = TVPVersionMinor;
    release = TVPVersionRelease;
    build = TVPVersionBuild;
}

void TVPGetTJSVersion(tjs_int &major, tjs_int &minor, tjs_int &release) {
    major = TJSVersionMajor;
    minor = TJSVersionMinor;
    release = TJSVersionRelease;
}
