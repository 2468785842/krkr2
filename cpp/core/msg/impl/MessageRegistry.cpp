//---------------------------------------------------------------------------
// Instantiates tTJSMessageHolder objects from generated TVPMessages.decl.h
//---------------------------------------------------------------------------
#include "tjsCommHead.h"
#include "tjsError.h"

#define TVP_MSG_DECL(name, msg) tTJSMessageHolder name(TJS_W(#name), msg);
#define TVP_MSG_DECL_CONST(name, msg)                                          \
    tTJSMessageHolder name(TJS_W(#name), msg, false);
#define TVP_MSG_DECL_NULL(name)                                                \
    tTJSMessageHolder name(TJS_W(#name), nullptr, false);
#define TVP_MSG_DEFINE(name, msg) TVP_MSG_DECL_CONST(name, TJS_W(msg))

#include "TVPMessages.decl.h"
