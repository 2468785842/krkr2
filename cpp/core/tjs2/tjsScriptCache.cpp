//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Script block caching
//---------------------------------------------------------------------------

#include "tjsCommHead.h"

#include "tjs.h"
#include "tjsScriptCache.h"
#include "tjsScriptBlock.h"
#include "tjsByteCodeLoader.h"

#define TJS_SCRIPT_CACHE_MAX 64

// currently this object holds only anonymous, single-context
// expression.

namespace TJS {
    //---------------------------------------------------------------------------
    // tTJSScriptCache - a class to cache script blocks
    //---------------------------------------------------------------------------
    tTJSScriptCache::tTJSScriptCache(tTJS *owner) :
        Cache(TJS_SCRIPT_CACHE_MAX) {
        Owner = owner;
    }

    //---------------------------------------------------------------------------
    tTJSScriptCache::~tTJSScriptCache() = default;

    //---------------------------------------------------------------------------
    void tTJSScriptCache::ExecScript(const tjs_char *script,
                                     tTJSVariant *result,
                                     iTJSDispatch2 *context,
                                     const tjs_char *name, tjs_int lineofs) {
        // currently this does nothing with normal script blocks.
        tTJSScriptBlock *blk = new tTJSScriptBlock(Owner);

        try {
            if(name)
                blk->SetName(name, lineofs);
            blk->SetText(result, script, context, false);
        } catch(...) {
            blk->Release();
            throw;
        }

        blk->Release();
    }

    //---------------------------------------------------------------------------
    void tTJSScriptCache::ExecScript(const ttstr &script, tTJSVariant *result,
                                     iTJSDispatch2 *context, const ttstr *name,
                                     tjs_int lineofs) {
        tTJSScriptBlock *blk = new tTJSScriptBlock(Owner);

        try {
            if(name)
                blk->SetName(name->c_str(), lineofs);
            blk->SetText(result, script.c_str(), context, false);
        } catch(...) {
            blk->Release();
            throw;
        }

        blk->Release();
    }

    //---------------------------------------------------------------------------
    void tTJSScriptCache::EvalExpression(const tjs_char *expression,
                                         tTJSVariant *result,
                                         iTJSDispatch2 *context,
                                         const tjs_char *name,
                                         tjs_int lineofs) {
        // currently this works only with anonymous script blocks.
        if(name) {
            tTJSScriptBlock *blk = new tTJSScriptBlock(Owner);

            try {
                blk->SetName(name, lineofs);
                blk->SetText(result, expression, context, true);
            } catch(...) {
                blk->Release();
                throw;
            }

            blk->Release();
            return;
        }

        // search through script block cache
        tScriptCacheData data;
        data.Script = expression;
        data.ExpressionMode = true;
        data.MustReturnResult = result != nullptr;

        tjs_uint32 hash = tScriptCacheHashFunc::Make(data);

        tScriptBlockHolder *holder = Cache.FindAndTouchWithHash(data, hash);

        if(holder) {
            // found in cache

            // execute script block in cache
            holder->GetObjectNoAddRef()->ExecuteTopLevelScript(result, context);
            return;
        }

        // not found in cache
        tTJSScriptBlock *blk = new tTJSScriptBlock(Owner);

        try {
            blk->SetText(result, expression, context, true);
        } catch(...) {
            blk->Release();
            throw;
        }

        // add to cache
        if(blk->IsReusable()) {
            // currently only single-context script block is cached
            tScriptBlockHolder newholder(blk);
            Cache.AddWithHash(data, hash, newholder);
        }

        blk->Release();
    }

    //---------------------------------------------------------------------------
    void tTJSScriptCache::EvalExpression(const ttstr &expression,
                                         tTJSVariant *result,
                                         iTJSDispatch2 *context,
                                         const ttstr *name, tjs_int lineofs) {
        // currently this works only with anonymous script blocks.

        // note that this function is basically the same as function
        // above.

        if(name && !name->IsEmpty()) {
            tTJSScriptBlock *blk = new tTJSScriptBlock(Owner);

            try {
                blk->SetName(name->c_str(), lineofs);
                blk->SetText(result, expression.c_str(), context, true);
            } catch(...) {
                blk->Release();
                throw;
            }

            blk->Release();
            return;
        }

        // search through script block cache
        tScriptCacheData data;
        data.Script = expression;
        data.ExpressionMode = true;
        data.MustReturnResult = result != nullptr;

        tjs_uint32 hash = tScriptCacheHashFunc::Make(data);

        tScriptBlockHolder *holder = Cache.FindAndTouchWithHash(data, hash);

        if(holder) {
            // found in cache

            // execute script block in cache
            holder->GetObjectNoAddRef()->ExecuteTopLevelScript(result, context);
            return;
        }

        // not found in cache
        tTJSScriptBlock *blk = new tTJSScriptBlock(Owner);

        try {
            blk->SetText(result, expression.c_str(), context, true);
        } catch(...) {
            blk->Release();
            throw;
        }

        // add to cache
        if(blk->IsReusable()) {
            // currently only single-context script block is cached
            tScriptBlockHolder newholder(blk);
            Cache.AddWithHash(data, hash, newholder);
        }

        blk->Release();
    }

    //---------------------------------------------------------------------------
    // for Bytecode
    void tTJSScriptCache::LoadByteCode(const tjs_uint8 *buff, size_t len,
                                       tTJSVariant *result,
                                       iTJSDispatch2 *context,
                                       const tjs_char *name) {
        auto loader = std::make_unique<tTJSByteCodeLoader>();
        std::unique_ptr<tTJSScriptBlock, std::function<void(tTJSScriptBlock *)>>
            blk{ loader->ReadByteCode(Owner, name, buff, len),
                 [](auto *ptr) { ptr->Release(); } };
        if(blk != nullptr) {
            blk->ExecuteTopLevel(result, context);
            return;
        }
        TJS_eTJSScriptError(TJSByteCodeBroken, blk.get(), 0);
    }
    //---------------------------------------------------------------------------
} // namespace TJS
