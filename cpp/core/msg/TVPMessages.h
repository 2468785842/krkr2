//---------------------------------------------------------------------------
// TVP engine message holder declarations (generated list in TVPMessages.decl.h)
//---------------------------------------------------------------------------
#ifndef TVP_MESSAGES_H_
#define TVP_MESSAGES_H_

#include "tjsMessage.h"

#ifndef TVP_MSG_DECL
#define TVP_MSG_DECL(name, msg) extern tTJSMessageHolder name;
#define TVP_MSG_DECL_CONST(name, msg) extern tTJSMessageHolder name;
#define TVP_MSG_DECL_NULL(name) extern tTJSMessageHolder name;
#endif

#define WIDEN2(x) TJS_W(x)
#define WIDEN(x) WIDEN2(x)
#define TVP_MSG_DEFINE(name, msg) TVP_MSG_DECL_CONST(name, TJS_W(msg))

#include "TVPMessages.decl.h"

#endif
