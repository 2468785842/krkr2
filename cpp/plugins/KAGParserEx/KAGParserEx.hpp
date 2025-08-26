#ifndef KAGParserExHPP
#define KAGParserExHPP

#define NO_V2LINK

#ifndef NO_V2LINK
#include <windows.h>
#endif


#include "tp_stub.h"

#define TVP_KAGPARSER_EX_PLUGIN

#include "tjsHashSearch.h"
using namespace TJS;
#include <vector>

#define TVP_KAGPARSER_EX_PLUGIN
#define TVP_KAGPARSER_EX_CLASSNAME TJS_W("KAGParser")

#define TVP_KAGPARSER_CONCAT(a,b) a ## b

// 正規のプラグインの場合の細工
#ifdef __TP_STUB_H__

#define TVP_KAGPARSER_MESSAGEMAP(name) (TJSGetMessageMapMessage(TVP_KAGPARSER_CONCAT(L,#name)).c_str())
// from tjsConfig.h
#define TJS_strchr			wcschr
#define TJS_strcmp			wcscmp
#define TJS_strncpy			wcsncpy
#define TJS_strlen          wcslen

// from MsgIntf.h (and MESSAGEMAP modified)
#define TVPThrowInternalError TVPThrowExceptionMessage(TVP_KAGPARSER_MESSAGEMAP(TVPInternalError), __FILE__,  __LINE__)

#endif

#include "KAGParser.h"


extern const tjs_char* TVPKAGNoLine;
extern const tjs_char* TVPKAGCannotOmmitFirstLabelName;
//const tjs_char* TVPInternalError = TJS_W("内部エラーが発生しました: at %1 line %2");
extern const tjs_char* TVPKAGMalformedSaveData;
extern const tjs_char* TVPKAGLabelNotFound;
extern const tjs_char* TVPLabelOrScriptInMacro;
extern const tjs_char* TVPKAGInlineScriptNotEnd ;
extern const tjs_char* TVPKAGSyntaxError;
extern const tjs_char* TVPKAGCallStackUnderflow;
extern const tjs_char* TVPKAGReturnLostSync;
extern const tjs_char* TVPKAGSpecifyKAGParser;
extern const tjs_char* TVPUnknownMacroName;
#endif
