//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// TJS2's C++ exception class and exception message
//---------------------------------------------------------------------------
#pragma once

#ifndef TJS_DECL_MESSAGE_BODY

#include <string>
#include "tjs.h"
#include "tjsVariant.h"
#include "tjsString.h"

namespace TJS {
    //---------------------------------------------------------------------------
    extern ttstr TJSNonamedException;

#define TJS_CONVERT_TO_TJS_EXCEPTION                                           \
    catch(const eTJSSilent &e) {                                               \
        throw e;                                                               \
    }                                                                          \
    catch(const eTJSScriptException &e) {                                      \
        throw e;                                                               \
    }                                                                          \
    catch(const eTJSScriptError &e) {                                          \
        throw e;                                                               \
    }                                                                          \
    catch(const eTJSError &e) {                                                \
        throw e;                                                               \
    }                                                                          \
    catch(const eTJS &e) {                                                     \
        TJS_eTJSError(e.GetMessage());                                         \
    }                                                                          \
    catch(const std::exception &e) {                                           \
        TJS_eTJSError(e.what());                                               \
    }                                                                          \
    catch(const tjs_char *text) {                                              \
        TJS_eTJSError(text);                                                   \
    }                                                                          \
    catch(const char *text) {                                                  \
        TJS_eTJSError(text);                                                   \
    }

    //---------------------------------------------------------------------------
    // TJSGetExceptionObject : retrieves TJS 'Exception' object
    //---------------------------------------------------------------------------
    extern void
    TJSGetExceptionObject(tTJS *tjs, tTJSVariant *res, tTJSVariant &msg,
                          tTJSVariant *trace /* trace is optional */ = nullptr);

    //---------------------------------------------------------------------------
    // eTJSxxxxx
    //---------------------------------------------------------------------------
    class eTJSSilent {
        // silent exception
    };

    //---------------------------------------------------------------------------
    class eTJS {
    public:
        eTJS() {}

        eTJS(const eTJS &) {}

        eTJS &operator=(const eTJS &e) = default;

        virtual ~eTJS() {}

        [[nodiscard]] virtual const ttstr &GetMessage() const {
            return TJSNonamedException;
        }
    };

    //---------------------------------------------------------------------------
    void TJS_eTJS();

    //---------------------------------------------------------------------------
    class eTJSError : public eTJS, public std::exception {
    public:
        eTJSError(const ttstr &Msg) : Message(Msg) {}

        const char *what() const noexcept override {
            cachedStr = Message.AsStdString();
            return cachedStr.c_str();
        }
        const ttstr &GetMessage() const override { return Message; }

        void AppendMessage(const ttstr &msg) { Message += msg; }

    private:
        ttstr Message;
        mutable std::string cachedStr;
    };

    //---------------------------------------------------------------------------
    void TJS_eTJSError(const ttstr &msg);

    void TJS_eTJSError(const tjs_char *msg);

    //---------------------------------------------------------------------------
    class eTJSVariantError : public eTJSError {
    public:
        eTJSVariantError(const ttstr &Msg) : eTJSError(Msg) {}

        eTJSVariantError(const eTJSVariantError &ref) : eTJSError(ref) {}
    };

    //---------------------------------------------------------------------------
    void TJS_eTJSVariantError(const ttstr &msg);

    void TJS_eTJSVariantError(const tjs_char *msg);

    //---------------------------------------------------------------------------
    class tTJSScriptBlock;

    class tTJSInterCodeContext;

    class eTJSScriptError : public eTJSError {
        class tScriptBlockHolder {
        public:
            tScriptBlockHolder(tTJSScriptBlock *block);

            ~tScriptBlockHolder();

            tScriptBlockHolder(const tScriptBlockHolder &holder);

            tTJSScriptBlock *Block;
        } Block;

        tjs_int Position;

        ttstr Trace;

    public:
        tTJSScriptBlock *GetBlockNoAddRef() const { return Block.Block; }

        tjs_int GetPosition() const { return Position; }

        tjs_int GetSourceLine() const;

        const tjs_char *GetBlockName() const;

        const ttstr &GetTrace() const { return Trace; }

        bool AddTrace(tTJSScriptBlock *block, tjs_int srcpos);

        bool AddTrace(tTJSInterCodeContext *context, tjs_int codepos);

        bool AddTrace(const ttstr &data);

        eTJSScriptError(const ttstr &Msg, tTJSScriptBlock *block, tjs_int pos);

        eTJSScriptError(const eTJSScriptError &ref) = default;
    };

    //---------------------------------------------------------------------------
    void TJS_eTJSScriptError(const ttstr &msg, tTJSScriptBlock *block,
                             tjs_int srcpos);

    void TJS_eTJSScriptError(const tjs_char *msg, tTJSScriptBlock *block,
                             tjs_int srcpos);

    void TJS_eTJSScriptError(const ttstr &msg, tTJSInterCodeContext *context,
                             tjs_int codepos);

    void TJS_eTJSScriptError(const tjs_char *msg, tTJSInterCodeContext *context,
                             tjs_int codepos);

    //---------------------------------------------------------------------------
    class eTJSScriptException : public eTJSScriptError {
        tTJSVariant Value;

    public:
        tTJSVariant &GetValue() { return Value; }

        eTJSScriptException(const ttstr &Msg, tTJSScriptBlock *block,
                            tjs_int pos, tTJSVariant &val) :
            eTJSScriptError(Msg, block, pos), Value(val) {}

        eTJSScriptException(const eTJSScriptException &ref) :
            eTJSScriptError(ref), Value(ref.Value) {
            ;
        }
    };

    //---------------------------------------------------------------------------
    void TJS_eTJSScriptException(const ttstr &msg, tTJSScriptBlock *block,
                                 tjs_int srcpos, tTJSVariant &val);

    void TJS_eTJSScriptException(const tjs_char *msg, tTJSScriptBlock *block,
                                 tjs_int srcpos, tTJSVariant &val);

    void TJS_eTJSScriptException(const ttstr &msg,
                                 tTJSInterCodeContext *context, tjs_int codepos,
                                 tTJSVariant &val);

    void TJS_eTJSScriptException(const tjs_char *msg,
                                 tTJSInterCodeContext *context, tjs_int codepos,
                                 tTJSVariant &val);

    //---------------------------------------------------------------------------
    class eTJSCompileError : public eTJSScriptError {
    public:
        eTJSCompileError(const ttstr &Msg, tTJSScriptBlock *block,
                         tjs_int pos) : eTJSScriptError(Msg, block, pos) {
        }

        eTJSCompileError(const eTJSCompileError &ref) : eTJSScriptError(ref) {
        }
    };

    //---------------------------------------------------------------------------
    void TJS_eTJSCompileError(const ttstr &msg, tTJSScriptBlock *block,
                              tjs_int srcpos);

    void TJS_eTJSCompileError(const tjs_char *msg, tTJSScriptBlock *block,
                              tjs_int srcpos);

    //---------------------------------------------------------------------------
    void TJSThrowFrom_tjs_error(tjs_error hr, const tjs_char *name = nullptr);

#define TJS_THROW_IF_ERROR(x)                                                  \
    {                                                                          \
        tjs_error ____er;                                                      \
        ____er = (x);                                                          \
        if(TJS_FAILED(____er))                                                 \
            TJSThrowFrom_tjs_error(____er);                                    \
    }
    //---------------------------------------------------------------------------
} // namespace TJS
//---------------------------------------------------------------------------
#endif // #ifndef TJS_DECL_MESSAGE_BODY

#define TJS_MSG_DECL(name, msg) inline TJS::tTJSMessageHolder name(TJS_W(#name), msg);
#define TJS_MSG_DECL_NULL(name) inline TJS::tTJSMessageHolder name(TJS_W(#name), nullptr);
#include "tjsErrorInc.h"

#undef TJS_MSG_DECL
#undef TJS_MSG_DECL_NULL
