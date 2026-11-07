//---------------------------------------------------------------------------
// TVP version and about string API
//---------------------------------------------------------------------------
#ifndef TVP_VERSION_H_
#define TVP_VERSION_H_

#include "tjs.h"

TJS_EXP_FUNC_DEF(ttstr, TVPGetAboutString, ());

TJS_EXP_FUNC_DEF(ttstr, TVPGetVersionInformation, ());

TJS_EXP_FUNC_DEF(ttstr, TVPGetVersionString, ());

extern tjs_int TVPVersionMajor;
extern tjs_int TVPVersionMinor;
extern tjs_int TVPVersionRelease;
extern tjs_int TVPVersionBuild;

extern void TVPGetVersion();

TJS_EXP_FUNC_DEF(void, TVPGetSystemVersion,
                 (tjs_int & major, tjs_int &minor, tjs_int &release,
                  tjs_int &build));

TJS_EXP_FUNC_DEF(void, TVPGetTJSVersion,
                 (tjs_int & major, tjs_int &minor, tjs_int &release));

using TVPAboutLogAppender = ttstr (*)();
void TVPSetAboutLogAppender(TVPAboutLogAppender appender);

#endif
