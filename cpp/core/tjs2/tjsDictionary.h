//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Dictionary class implementation
//---------------------------------------------------------------------------

#ifndef tjsDictionaryH
#define tjsDictionaryH

#include "tjsObject.h"
#include "tjsNative.h"
#include "tjsArray.h"

namespace TJS {
    //---------------------------------------------------------------------------
    // tTJSDictionaryClass : Dictoinary Class
    //---------------------------------------------------------------------------
    class tTJSDictionaryClass : public tTJSNativeClass {
        typedef tTJSNativeClass inherited;

    public:
        tTJSDictionaryClass();

        ~tTJSDictionaryClass() override;

    public:
        tjs_error CreateNew(tjs_uint32 flag, const tjs_char *membername,
                            tjs_uint32 *hint, iTJSDispatch2 **result,
                            tjs_int numparams, tTJSVariant **param,
                            iTJSDispatch2 *objthis) override;

    protected:
        tTJSNativeInstance *CreateNativeInstance() override;

        iTJSDispatch2 *CreateBaseTJSObject() override;

        static tjs_uint32 ClassID;

    private:
    protected:
    };

    //---------------------------------------------------------------------------
    // tTJSDictionaryNI : TJS Dictionary Native C++ instance
    //---------------------------------------------------------------------------
    class tTJSDictionaryNI : public tTJSNativeInstance,
                             public tTJSSaveStructuredDataCallback {
        typedef tTJSNativeInstance inherited;

        tTJSCustomObject *Owner;

    public:
        tTJSDictionaryNI();

        ~tTJSDictionaryNI() override;

        tjs_error Construct(tjs_int numparams, tTJSVariant **param,
                            iTJSDispatch2 *obj) override;

    private:
        void Invalidate() override; // Invalidate override

    public:
        [[nodiscard]] bool IsValid() const {
            return Owner != nullptr;
        } // check validation

        void Assign(iTJSDispatch2 *dsp, bool clear = true);

        void Clear();

    private:
        struct tAssignCallback : public tTJSDispatch {
            tTJSCustomObject *Owner{};

            tjs_error FuncCall(tjs_uint32 flag, const tjs_char *membername,
                               tjs_uint32 *hint, tTJSVariant *result,
                               tjs_int numparams, tTJSVariant **param,
                               iTJSDispatch2 *objthis) override;
            // method from iTJSDispatch2, for enumeration callback
        };

        friend class tSaveStructCallback;

    public:
        void SaveStructuredData(std::vector<iTJSDispatch2 *> &stack,
                                iTJSTextWriteStream &stream,
                                const ttstr &indentstr) override;

        void SaveStructuredBinary(std::vector<iTJSDispatch2 *> &stack,
                                  tTJSBinaryStream &stream) override;
        // method from tTJSSaveStructuredDataCallback
    private:
        struct tSaveStructCallback : public tTJSDispatch {
            std::vector<iTJSDispatch2 *> *Stack{};
            iTJSTextWriteStream *Stream{};
            const ttstr *IndentStr{};
            bool First{};

            tjs_error FuncCall(tjs_uint32 flag, const tjs_char *membername,
                               tjs_uint32 *hint, tTJSVariant *result,
                               tjs_int numparams, tTJSVariant **param,
                               iTJSDispatch2 *objthis) override;
        };

        friend struct tSaveStructCallback;

        struct tSaveStructBinayCallback : public tTJSDispatch {
            std::vector<iTJSDispatch2 *> *Stack{};
            tTJSBinaryStream *Stream{};

            tjs_error FuncCall(tjs_uint32 flag, const tjs_char *membername,
                               tjs_uint32 *hint, tTJSVariant *result,
                               tjs_int numparams, tTJSVariant **param,
                               iTJSDispatch2 *objthis) override;
        };

        friend struct tSaveStructBinayCallback;

        struct tSaveMemberCountCallback : public tTJSDispatch {
            tjs_uint Count;

            tSaveMemberCountCallback() : Count(0) {}

            tjs_error FuncCall(tjs_uint32 flag, const tjs_char *membername,
                               tjs_uint32 *hint, tTJSVariant *result,
                               tjs_int numparams, tTJSVariant **param,
                               iTJSDispatch2 *objthis) override;
        };

        friend struct tSaveMemberCountCallback;

    public:
        void AssignStructure(iTJSDispatch2 *dsp,
                             std::vector<iTJSDispatch2 *> &stack);

        struct tAssignStructCallback : public tTJSDispatch {
            std::vector<iTJSDispatch2 *> *Stack{};
            iTJSDispatch2 *Dest{};

            tjs_error FuncCall(tjs_uint32 flag, const tjs_char *membername,
                               tjs_uint32 *hint, tTJSVariant *result,
                               tjs_int numparams, tTJSVariant **param,
                               iTJSDispatch2 *objthis) override;
        };

        friend struct tAssignStructCallback;
    };

    //---------------------------------------------------------------------------
    class tTJSDictionaryObject : public tTJSCustomObject {
        typedef tTJSCustomObject inherited;

    public:
        tTJSDictionaryObject();

        tTJSDictionaryObject(tjs_int hashbits);

        ~tTJSDictionaryObject() override;

        tjs_error FuncCall(tjs_uint32 flag, const tjs_char *membername,
                           tjs_uint32 *hint, tTJSVariant *result,
                           tjs_int numparams, tTJSVariant **param,
                           iTJSDispatch2 *objthis) override;

        tjs_error PropGet(tjs_uint32 flag, const tjs_char *membername,
                          tjs_uint32 *hint, tTJSVariant *result,
                          iTJSDispatch2 *objthis) override;

        tjs_error CreateNew(tjs_uint32 flag, const tjs_char *membername,
                            tjs_uint32 *hint, iTJSDispatch2 **result,
                            tjs_int numparams, tTJSVariant **param,
                            iTJSDispatch2 *objthis) override;

        tjs_error Operation(tjs_uint32 flag, const tjs_char *membername,
                            tjs_uint32 *hint, tTJSVariant *result,
                            const tTJSVariant *param,
                            iTJSDispatch2 *objthis) override;
    };
    //---------------------------------------------------------------------------

    //---------------------------------------------------------------------------
    // TJSGetDictionaryClassID
    //---------------------------------------------------------------------------
    extern tjs_int32 TJSGetDictionaryClassID();
    //---------------------------------------------------------------------------

    //---------------------------------------------------------------------------
    // TJSCreateDictionaryObject
    //---------------------------------------------------------------------------
    TJS_EXP_FUNC_DEF(iTJSDispatch2 *, TJSCreateDictionaryObject,
                     (iTJSDispatch2 **classout = nullptr));
    //---------------------------------------------------------------------------

    //---------------------------------------------------------------------------
} // namespace TJS
#endif
