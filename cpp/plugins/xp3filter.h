#pragma once
#include "tjsObject.h"

class CBinaryAccessor : public tTJSDispatch {
    unsigned int m_length;
    unsigned int m_curPos;
    unsigned char *m_buff;

    tjs_error FuncXor(tjs_int numparams, tTJSVariant **param);

    tjs_error FuncAdd(tjs_int numparams, tTJSVariant **param);

public:
    CBinaryAccessor(unsigned char *buff, unsigned int len);

public:
    virtual tjs_error FuncCall( // function invocation
        tjs_uint32 flag, // calling flag
        const tjs_char *membername, // member name ( nullptr for a
                                    // default member )
        tjs_uint32 *hint, // hint for the member name (in/out)
        tTJSVariant *result, // result
        tjs_int numparams, // number of parameters
        tTJSVariant **param, // parameters
        iTJSDispatch2 *objthis // object as "this"
    );

    virtual tjs_error PropGet( // property get
        tjs_uint32 flag, // calling flag
        const tjs_char *membername, // member name ( nullptr for a
                                    // default member )
        tjs_uint32 *hint, // hint for the member name (in/out)
        tTJSVariant *result, // result
        iTJSDispatch2 *objthis // object as "this"
    );

    virtual tjs_error PropGetByNum( // property get by index number
        tjs_uint32 flag, // calling flag
        tjs_int num, // index number
        tTJSVariant *result, // result
        iTJSDispatch2 *objthis // object as "this"
    );

    virtual tjs_error PropSet( // property set
        tjs_uint32 flag, // calling flag
        const tjs_char *membername, // member name ( nullptr for a
                                    // default member )
        tjs_uint32 *hint, // hint for the member name (in/out)
        const tTJSVariant *param, // parameters
        iTJSDispatch2 *objthis // object as "this"
    );

    virtual tjs_error PropSetByNum( // property set by index number
        tjs_uint32 flag, // calling flag
        tjs_int num, // index number
        const tTJSVariant *param, // parameters
        iTJSDispatch2 *objthis // object as "this"
    );

    virtual tjs_error GetCount( // get member count
        tjs_int *result, // variable that receives the result
        const tjs_char *membername, // member name ( nullptr for a
                                    // default member )
        tjs_uint32 *hint, // hint for the member name (in/out)
        iTJSDispatch2 *objthis // object as "this"
    );

    virtual tjs_error Invalidate( // invalidation
        tjs_uint32 flag, // calling flag
        const tjs_char *membername, // member name ( nullptr for a
                                    // default member )
        tjs_uint32 *hint, // hint for the member name (in/out)
        iTJSDispatch2 *objthis // object as "this"
    );

    virtual tjs_error IsValid( // get validation, returns TJS_S_TRUE (valid) or
                               // TJS_S_FALSE (invalid)
        tjs_uint32 flag, // calling flag
        const tjs_char *membername, // member name ( nullptr for a
                                    // default member )
        tjs_uint32 *hint, // hint for the member name (in/out)
        iTJSDispatch2 *objthis // object as "this"
    );

    virtual tjs_error Operation( // operation with member
        tjs_uint32 flag, // calling flag
        const tjs_char *membername, // member name ( nullptr for a
                                    // default member )
        tjs_uint32 *hint, // hint for the member name (in/out)
        tTJSVariant *result, // result ( can be nullptr )
        const tTJSVariant *param, // parameters
        iTJSDispatch2 *objthis // object as "this"
    );

    virtual tjs_error OperationByNum( // operation with member by index number
        tjs_uint32 flag, // calling flag
        tjs_int num, // index number
        tTJSVariant *result, // result ( can be nullptr )
        const tTJSVariant *param, // parameters
        iTJSDispatch2 *objthis // object as "this"
    );
};
