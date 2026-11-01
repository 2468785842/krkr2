//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Definition of Messages and Message Related Utilities
//---------------------------------------------------------------------------
#ifndef MsgIntfH
#define MsgIntfH

#include "TVPError.h"
#include "TVPMessages.h"
#include "TVPVersion.h"

#define TVPThrowInternalError                                                  \
    TVPThrowExceptionMessage(TVPInternalError, __FILE__, __LINE__)

#endif
