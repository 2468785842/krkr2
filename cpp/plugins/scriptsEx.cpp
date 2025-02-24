//
// Created by Li_Dong on 2024/12/9.
// source url: https://github.com/wamsoft/scriptsEx/blob
//
#include "ncbind/ncbind.hpp"
#include <vector>
#include <algorithm>
#include "PluginImpl.h"

#define NCB_MODULE_NAME TJS_W("ScriptsEx.dll")

static bool TJS_USERENTRY

_CatchFuncCall(void *data, const tTVPExceptionDesc &desc) {
    throw desc;
}

struct t_iTJSDispatch2_AddRef {
    tjs_uint _ret;
    iTJSDispatch2 *_this;

    t_iTJSDispatch2_AddRef(iTJSDispatch2 *_this_) : _this(_this_) {}
};

struct t_iTJSDispatch2_PropGet {
    tjs_error _ret;
    iTJSDispatch2 *_this;
    tjs_uint32 flag;
    const tjs_char *membername;
    tjs_uint32 *hint;
    tTJSVariant *result;
    iTJSDispatch2 *objthis;

    t_iTJSDispatch2_PropGet(iTJSDispatch2 *_this_, tjs_uint32 flag_,
                            const tjs_char *membername_, tjs_uint32 *hint_,
                            tTJSVariant *result_, iTJSDispatch2 *objthis_) :
        _this(_this_),
        flag(flag_), membername(membername_), hint(hint_), result(result_),
        objthis(objthis_) {
        ;
    }
};

static void TJS_USERENTRY

_Try_iTJSDispatch2_PropGet(void *data) {
    t_iTJSDispatch2_PropGet *arg = (t_iTJSDispatch2_PropGet *)data;
    arg->_ret = arg->_this->PropGet(arg->flag, arg->membername, arg->hint,
                                    arg->result, arg->objthis);
}

tjs_error Try_iTJSDispatch2_PropGet(iTJSDispatch2 *_this, tjs_uint32 flag,
                                    const tjs_char *membername,
                                    tjs_uint32 *hint, tTJSVariant *result,
                                    iTJSDispatch2 *objthis) {
    t_iTJSDispatch2_PropGet arg(_this, flag, membername, hint, result, objthis);
    TVPDoTryBlock(_Try_iTJSDispatch2_PropGet, _CatchFuncCall, nullptr, &arg);
    return arg._ret;
}

static void TJS_USERENTRY

_Try_iTJSDispatch2_AddRef(void *data) {
    t_iTJSDispatch2_AddRef *arg = (t_iTJSDispatch2_AddRef *)data;
    arg->_ret = arg->_this->AddRef(

    );
}

tjs_uint Try_iTJSDispatch2_AddRef(iTJSDispatch2 *_this) {
    t_iTJSDispatch2_AddRef arg(_this);
    TVPDoTryBlock(_Try_iTJSDispatch2_AddRef, _CatchFuncCall, nullptr, &arg);
    return arg._ret;
}

struct t_iTJSDispatch2_PropGetByNum {
    tjs_error _ret;
    iTJSDispatch2 *_this;
    tjs_uint32 flag;
    tjs_int num;
    tTJSVariant *result;
    iTJSDispatch2 *objthis;

    t_iTJSDispatch2_PropGetByNum(iTJSDispatch2 *_this_, tjs_uint32 flag_,
                                 tjs_int num_, tTJSVariant *result_,
                                 iTJSDispatch2 *objthis_) :
        _this(_this_),
        flag(flag_), num(num_), result(result_), objthis(objthis_) {
        ;
    }
};

static void TJS_USERENTRY

_Try_iTJSDispatch2_PropGetByNum(void *data) {
    t_iTJSDispatch2_PropGetByNum *arg = (t_iTJSDispatch2_PropGetByNum *)data;
    arg->_ret = arg->_this->PropGetByNum(arg->flag, arg->num, arg->result,
                                         arg->objthis);
}

struct t_iTJSDispatch2_PropSet {
    tjs_error _ret;
    iTJSDispatch2 *_this;
    tjs_uint32 flag;
    const tjs_char *membername;
    tjs_uint32 *hint;
    const tTJSVariant *param;
    iTJSDispatch2 *objthis;

    t_iTJSDispatch2_PropSet(iTJSDispatch2 *_this_, tjs_uint32 flag_,
                            const tjs_char *membername_, tjs_uint32 *hint_,
                            const tTJSVariant *param_,
                            iTJSDispatch2 *objthis_) :
        _this(_this_),
        flag(flag_), membername(membername_), hint(hint_), param(param_),
        objthis(objthis_) {
        ;
    }
};

tjs_error Try_iTJSDispatch2_PropGetByNum(iTJSDispatch2 *_this, tjs_uint32 flag,
                                         tjs_int num, tTJSVariant *result,
                                         iTJSDispatch2 *objthis) {
    t_iTJSDispatch2_PropGetByNum arg(_this, flag, num, result, objthis);
    TVPDoTryBlock(_Try_iTJSDispatch2_PropGetByNum, _CatchFuncCall, nullptr,
                  &arg);
    return arg._ret;
}

static void TJS_USERENTRY

_Try_iTJSDispatch2_PropSet(void *data) {
    t_iTJSDispatch2_PropSet *arg = (t_iTJSDispatch2_PropSet *)data;
    arg->_ret = arg->_this->PropSet(arg->flag, arg->membername, arg->hint,
                                    arg->param, arg->objthis);
}

tjs_error Try_iTJSDispatch2_PropSet(iTJSDispatch2 *_this, tjs_uint32 flag,
                                    const tjs_char *membername,
                                    tjs_uint32 *hint, const tTJSVariant *param,
                                    iTJSDispatch2 *objthis) {
    t_iTJSDispatch2_PropSet arg(_this, flag, membername, hint, param, objthis);
    TVPDoTryBlock(_Try_iTJSDispatch2_PropSet, _CatchFuncCall, nullptr, &arg);
    return arg._ret;
}

struct t_iTJSDispatch2_PropSetByNum {
    tjs_error _ret;
    iTJSDispatch2 *_this;
    tjs_uint32 flag;
    tjs_int num;
    const tTJSVariant *param;
    iTJSDispatch2 *objthis;

    t_iTJSDispatch2_PropSetByNum(iTJSDispatch2 *_this_, tjs_uint32 flag_,
                                 tjs_int num_, const tTJSVariant *param_,
                                 iTJSDispatch2 *objthis_) :
        _this(_this_),
        flag(flag_), num(num_), param(param_), objthis(objthis_) {
        ;
    }
};

static void TJS_USERENTRY

_Try_iTJSDispatch2_PropSetByNum(void *data) {
    t_iTJSDispatch2_PropSetByNum *arg = (t_iTJSDispatch2_PropSetByNum *)data;
    arg->_ret =
        arg->_this->PropSetByNum(arg->flag, arg->num, arg->param, arg->objthis);
}

tjs_error Try_iTJSDispatch2_PropSetByNum(iTJSDispatch2 *_this, tjs_uint32 flag,
                                         tjs_int num, const tTJSVariant *param,
                                         iTJSDispatch2 *objthis) {
    t_iTJSDispatch2_PropSetByNum arg(_this, flag, num, param, objthis);
    TVPDoTryBlock(_Try_iTJSDispatch2_PropSetByNum, _CatchFuncCall, nullptr,
                  &arg);
    return arg._ret;
}

/**
 * メソッド追加用
 */
class ScriptsAdd {

public:
    ScriptsAdd(){};

    /**
     * メンバ名一覧の取得
     */
    static tjs_error getKeys(tTJSVariant *result, tjs_int numparams,
                             tTJSVariant **param, iTJSDispatch2 *objthis);

    /**
     * メンバの個数の取得
     */
    static tjs_error getCount(tTJSVariant *result, tjs_int numparams,
                              tTJSVariant **param, iTJSDispatch2 *objthis);

    /**
     * コンテキストの取得
     */
    static tTJSVariant getObjectContext(tTJSVariant obj);

    /**
     * コンテキストが nullptr かどうか判定
     */
    static bool isNullContext(tTJSVariant obj);

    //----------------------------------------------------------------------
    // 構造体比較関数
    static bool equalStruct(tTJSVariant v1, tTJSVariant v2);

    //----------------------------------------------------------------------
    // 構造体比較関数(数字の比較はゆるい)
    static bool equalStructNumericLoose(tTJSVariant v1, tTJSVariant v2);

    //----------------------------------------------------------------------
    // 全配列・辞書巡回
    static tjs_error foreach(tTJSVariant *result, tjs_int numparams,
                             tTJSVariant * *param, iTJSDispatch2 * objthis);

    //----------------------------------------------------------------------
    // hash値取得
    static tjs_error getMD5HashString(tTJSVariant *result, tjs_int numparams,
                                      tTJSVariant **param,
                                      iTJSDispatch2 *objthis);

    //----------------------------------------------------------------------
    // オブジェクト複製
    static tTJSVariant clone(tTJSVariant v1);

    //----------------------------------------------------------------------
    // フラグ指定つきプロパティ操作
    static tjs_error propSet(tTJSVariant *result, tjs_int numparams,
                             tTJSVariant **param, iTJSDispatch2 *objthis);

    static tjs_error propGet(tTJSVariant *result, tjs_int numparams,
                             tTJSVariant **param, iTJSDispatch2 *objthis);

    //----------------------------------------------------------------------
    // (const)つき辞書／配列を安全に評価
    static tjs_error safeEvalStorage(tTJSVariant *result, tjs_int numparams,
                                     tTJSVariant **param,
                                     iTJSDispatch2 *objthis);

private:
    /**
     * メンバ名一覧の取得
     */
    static void _getKeys(tTJSVariant *result, tTJSVariant &obj);
};

/**
 * 辞書のキー一覧取得用
 */
class DictMemberGetCaller : public tTJSDispatch /** EnumMembers 用 */
{
public:
    DictMemberGetCaller(iTJSDispatch2 *array) : array(array){};

    virtual tjs_error FuncCall( // function invocation
        tjs_uint32 flag, // calling flag
        const tjs_char *membername, // member name ( nullptr for a
                                    // default member )
        tjs_uint32 *hint, // hint for the member name (in/out)
        tTJSVariant *result, // result
        tjs_int numparams, // number of parameters
        tTJSVariant **param, // parameters
        iTJSDispatch2 *objthis // object as "this"
    ) {
        if(numparams > 1) {
            tTVInteger flag = param[1]->AsInteger();
            static tjs_uint addHint = 0;
            if(!(flag & TJS_HIDDENMEMBER)) {
                array->FuncCall(0, TJS_W("add"), &addHint, 0, 1, &param[0],
                                array);
            }
        }
        if(result) {
            *result = true;
        }
        return TJS_S_OK;
    }

protected:
    iTJSDispatch2 *array;
};

//----------------------------------------------------------------------
// 辞書を作成
tTJSVariant createDictionary() {
    iTJSDispatch2 *obj = TJSCreateDictionaryObject();
    tTJSVariant result(obj, obj);
    obj->Release();
    return result;
}

//----------------------------------------------------------------------
// 配列を作成
tTJSVariant createArray() {
    iTJSDispatch2 *obj = TJSCreateArrayObject();
    tTJSVariant result(obj, obj);
    obj->Release();
    return result;
}

//----------------------------------------------------------------------
// 辞書の要素を全比較するCaller
class DictMemberCompareCaller : public tTJSDispatch {
public:
    tTJSVariantClosure &target;
    bool match;

    DictMemberCompareCaller(tTJSVariantClosure &_target) :
        target(_target), match(true) {}

    virtual tjs_error FuncCall( // function invocation
        tjs_uint32 flag, // calling flag
        const tjs_char *membername, // member name ( nullptr for a
                                    // default member )
        tjs_uint32 *hint, // hint for the member name (in/out)
        tTJSVariant *result, // result
        tjs_int numparams, // number of parameters
        tTJSVariant **param, // parameters
        iTJSDispatch2 *objthis // object as "this"
    ) {
        if(result)
            *result = true;
        if(numparams > 1) {
            if((int)*param[1] != TJS_HIDDENMEMBER) {
                const tjs_char *key = param[0]->GetString();
                tTJSVariant value = *param[2];
                tTJSVariant targetValue;
                if(target.PropGet(TJS_MEMBERMUSTEXIST, key, nullptr,
                                  &targetValue, nullptr) == TJS_S_OK) {
                    match =
                        match && ScriptsAdd::equalStruct(value, targetValue);
                    if(result)
                        *result = match;
                } else {
                    match = false;
                    if(result) {
                        *result = match;
                    }
                }
            }
        }
        return TJS_S_OK;
    }
};

//----------------------------------------------------------------------
// 辞書の要素を全比較するCaller(数字の比較はゆるい)
class DictMemberCompareNumericLooseCaller : public tTJSDispatch {
public:
    tTJSVariantClosure &target;
    bool match;

    DictMemberCompareNumericLooseCaller(tTJSVariantClosure &_target) :
        target(_target), match(true) {}

    virtual tjs_error FuncCall( // function invocation
        tjs_uint32 flag, // calling flag
        const tjs_char *membername, // member name ( nullptr for a
                                    // default member )
        tjs_uint32 *hint, // hint for the member name (in/out)
        tTJSVariant *result, // result
        tjs_int numparams, // number of parameters
        tTJSVariant **param, // parameters
        iTJSDispatch2 *objthis // object as "this"
    ) {
        if(result)
            *result = true;
        if(numparams > 1) {
            if((int)*param[1] != TJS_HIDDENMEMBER) {
                const tjs_char *key = param[0]->GetString();
                tTJSVariant value = *param[2];
                tTJSVariant targetValue;
                if(target.PropGet(0, key, nullptr, &targetValue, nullptr) ==
                   TJS_S_OK) {
                    match = match &&
                        ScriptsAdd::equalStructNumericLoose(value, targetValue);
                    if(result)
                        *result = match;
                }
            }
        }
        return TJS_S_OK;
    }
};

//----------------------------------------------------------------------
// 辞書を巡回するcaller
class DictIterateCaller : public tTJSDispatch {
public:
    iTJSDispatch2 *func;
    iTJSDispatch2 *functhis;
    tTJSVariant **paramList;
    tjs_int paramCount;
    tTJSVariant breakResult;

    DictIterateCaller(iTJSDispatch2 *func, iTJSDispatch2 *functhis,
                      tTJSVariant **_paramList, tjs_int _paramCount) :
        func(func),
        functhis(functhis), paramList(_paramList), paramCount(_paramCount) {}

    virtual tjs_error FuncCall( // function invocation
        tjs_uint32 flag, // calling flag
        const tjs_char *membername, // member name ( nullptr for a
                                    // default member )
        tjs_uint32 *hint, // hint for the member name (in/out)
        tTJSVariant *result, // result
        tjs_int numparams, // number of parameters
        tTJSVariant **param, // parameters
        iTJSDispatch2 *objthis // object as "this"
    ) {
        breakResult.Clear();
        if(numparams > 1) {
            if((int)*param[1] != TJS_HIDDENMEMBER) {
                paramList[0] = param[0];
                paramList[1] = param[2];
                func->FuncCall(0, 0, 0, &breakResult, paramCount, paramList,
                               functhis);
            }
        }
        if(result) {
            *result = breakResult.Type() == tvtVoid;
        }
        return TJS_S_OK;
    }
};

//----------------------------------------------------------------------
// 変数
tjs_uint32 countHint;

void ScriptsAdd::_getKeys(tTJSVariant *result, tTJSVariant &obj) {
    if(result) {
        iTJSDispatch2 *array = TJSCreateArrayObject();
        DictMemberGetCaller *caller = new DictMemberGetCaller(array);
        tTJSVariantClosure closure(caller);
        obj.AsObjectClosureNoAddRef().EnumMembers(
            TJS_IGNOREPROP | TJS_ENUM_NO_VALUE, &closure, nullptr);
        caller->Release();
        static tjs_uint sortHint = 0;
        // 返すキーはソートする
        array->FuncCall(0, TJS_W("sort"), &sortHint, 0, 0, 0, array);
        *result = tTJSVariant(array, array);
        array->Release();
    }
}

/**
 * メンバ名一覧の取得
 */
tjs_error ScriptsAdd::getKeys(tTJSVariant *result, tjs_int numparams,
                              tTJSVariant **param, iTJSDispatch2 *objthis) {
    if(numparams < 1)
        return TJS_E_BADPARAMCOUNT;
    _getKeys(result, *param[0]);
    return TJS_S_OK;
}

/**
 * メンバの個数の取得
 */
tjs_error ScriptsAdd::getCount(tTJSVariant *result, tjs_int numparams,
                               tTJSVariant **param, iTJSDispatch2 *objthis) {
    if(numparams < 1)
        return TJS_E_BADPARAMCOUNT;
    if(result) {
        tjs_int count;
        param[0]
            ->

            AsObjectClosureNoAddRef()

            .GetCount(&count, nullptr, nullptr, nullptr);
        *result = count;
    }
    return TJS_S_OK;
}

/**
 * コンテキストの取得
 */
tTJSVariant ScriptsAdd::getObjectContext(tTJSVariant obj) {
    iTJSDispatch2 *objthis = obj.AsObjectClosureNoAddRef().ObjThis;
    return tTJSVariant(objthis, objthis);
}

/**
 * コンテキストが nullptr かどうか判定
 */
bool ScriptsAdd::isNullContext(tTJSVariant obj) {
    return obj.

        AsObjectClosureNoAddRef()

            .ObjThis == nullptr;
}

//----------------------------------------------------------------------
// 構造体比較関数
bool ScriptsAdd::equalStruct(tTJSVariant v1, tTJSVariant v2) {
    // タイプがオブジェクトなら特殊判定
    if(v1.

           Type()

           == tvtObject &&
       v2.

           Type()

           == tvtObject) {
        if(v1.

           AsObjectNoAddRef()

           == v2.

              AsObjectNoAddRef()

        )
            return true;

        tTJSVariantClosure &o1 = v1.AsObjectClosureNoAddRef();
        tTJSVariantClosure &o2 = v2.AsObjectClosureNoAddRef();

        // 関数どうしなら特別扱いで関数比較
        if(o1.IsInstanceOf(0, nullptr, nullptr, TJS_W("Function"), nullptr) ==
               TJS_S_TRUE &&
           o2.IsInstanceOf(0, nullptr, nullptr, TJS_W("Function"), nullptr) ==
               TJS_S_TRUE)
            return v1.DiscernCompare(v2);

        // Arrayどうしなら全項目を比較
        if(o1.IsInstanceOf(0, nullptr, nullptr, TJS_W("Array"), nullptr) ==
               TJS_S_TRUE &&
           o2.IsInstanceOf(0, nullptr, nullptr, TJS_W("Array"), nullptr) ==
               TJS_S_TRUE) {
            // 長さが一致していなければ比較失敗
            tTJSVariant o1Count, o2Count;
            o1.PropGet(0, TJS_W("count"), &countHint, &o1Count, nullptr);
            o2.PropGet(0, TJS_W("count"), &countHint, &o2Count, nullptr);
            if(!o1Count.DiscernCompare(o2Count))
                return false;
            // 全項目を順番に比較
            tjs_int count = o1Count;
            tTJSVariant o1Val, o2Val;
            for(tjs_int i = 0; i < count; i++) {
                o1.PropGetByNum(TJS_IGNOREPROP, i, &o1Val, nullptr);
                o2.PropGetByNum(TJS_IGNOREPROP, i, &o2Val, nullptr);
                if(!equalStruct(o1Val, o2Val))
                    return false;
            }
            return true;
        }

        // Dictionaryどうしなら全項目を比較
        if(o1.IsInstanceOf(0, nullptr, nullptr, TJS_W("Dictionary"), nullptr) ==
               TJS_S_TRUE &&
           o2.IsInstanceOf(0, nullptr, nullptr, TJS_W("Dictionary"), nullptr) ==
               TJS_S_TRUE) {
            // キー一覧が一致してなければ比較失敗
            tTJSVariant k1, k2;
            _getKeys(&k1, v1);
            _getKeys(&k2, v2);
            if(!equalStruct(k1, k2)) {
                return false;
            }
            // 全項目を順番に比較
            DictMemberCompareCaller *caller = new DictMemberCompareCaller(o2);
            tTJSVariantClosure closure(caller);
            tTJSVariant(o1.EnumMembers(TJS_IGNOREPROP, &closure, nullptr));
            bool result = caller->match;
            caller->Release();

            return result;
        }
    }

    return v1.DiscernCompare(v2);
}

//----------------------------------------------------------------------
// 構造体比較関数(数字の比較はゆるい)
bool ScriptsAdd::equalStructNumericLoose(tTJSVariant v1, tTJSVariant v2) {
    // タイプがオブジェクトなら特殊判定
    if(v1.

           Type()

           == tvtObject &&
       v2.

           Type()

           == tvtObject) {
        if(v1.

           AsObjectNoAddRef()

           == v2.

              AsObjectNoAddRef()

        )
            return true;

        tTJSVariantClosure &o1 = v1.AsObjectClosureNoAddRef();
        tTJSVariantClosure &o2 = v2.AsObjectClosureNoAddRef();

        // 関数どうしなら特別扱いで関数比較
        if(o1.IsInstanceOf(0, nullptr, nullptr, TJS_W("Function"), nullptr) ==
               TJS_S_TRUE &&
           o2.IsInstanceOf(0, nullptr, nullptr, TJS_W("Function"), nullptr) ==
               TJS_S_TRUE)
            return v1.DiscernCompare(v2);

        // Arrayどうしなら全項目を比較
        if(o1.IsInstanceOf(0, nullptr, nullptr, TJS_W("Array"), nullptr) ==
               TJS_S_TRUE &&
           o2.IsInstanceOf(0, nullptr, nullptr, TJS_W("Array"), nullptr) ==
               TJS_S_TRUE) {
            // 長さが一致していなければ比較失敗
            tTJSVariant o1Count, o2Count;
            o1.PropGet(0, TJS_W("count"), &countHint, &o1Count, nullptr);
            o2.PropGet(0, TJS_W("count"), &countHint, &o2Count, nullptr);
            if(!o1Count.DiscernCompare(o2Count))
                return false;
            // 全項目を順番に比較
            tjs_int count = o1Count;
            tTJSVariant o1Val, o2Val;
            for(tjs_int i = 0; i < count; i++) {
                o1.PropGetByNum(TJS_IGNOREPROP, i, &o1Val, nullptr);
                o2.PropGetByNum(TJS_IGNOREPROP, i, &o2Val, nullptr);
                if(!equalStructNumericLoose(o1Val, o2Val))
                    return false;
            }
            return true;
        }

        // Dictionaryどうしなら全項目を比較
        if(o1.IsInstanceOf(0, nullptr, nullptr, TJS_W("Dictionary"), nullptr) ==
               TJS_S_TRUE &&
           o2.IsInstanceOf(0, nullptr, nullptr, TJS_W("Dictionary"), nullptr) ==
               TJS_S_TRUE) {
            // 項目数が一致していなければ比較失敗
            tjs_int o1Count, o2Count;
            o1.GetCount(&o1Count, nullptr, nullptr, nullptr);
            o2.GetCount(&o2Count, nullptr, nullptr, nullptr);
            if(o1Count != o2Count)
                return false;
            // 全項目を順番に比較
            DictMemberCompareNumericLooseCaller *caller =
                new DictMemberCompareNumericLooseCaller(o2);
            tTJSVariantClosure closure(caller);
            tTJSVariant(o1.EnumMembers(TJS_IGNOREPROP, &closure, nullptr));
            bool result = caller->match;
            caller->

                Release();

            return result;
        }
    }

    // 数字の場合は
    if((v1.Type() == tvtInteger || v1.Type() == tvtReal) &&
       (v2.Type() == tvtInteger || v2.Type() == tvtReal))
        return v1.NormalCompare(v2);

    return v1.DiscernCompare(v2);
}

//----------------------------------------------------------------------
// 全配列・辞書巡回
tjs_error ScriptsAdd::foreach(tTJSVariant *result, tjs_int numparams,
                              tTJSVariant * *param, iTJSDispatch2 * objthis) {
    if(numparams < 2)
        return TJS_E_BADPARAMCOUNT;
    tTJSVariantClosure &obj = param[0]->AsObjectClosureNoAddRef();
    tTJSVariantClosure &funcClosure = param[1]->AsObjectClosureNoAddRef();

    // 実行対象関数を選択
    // 無名関数なら this コンテキストで動作させる
    iTJSDispatch2 *func = funcClosure.Object;
    iTJSDispatch2 *functhis = funcClosure.ObjThis;
    if(functhis == 0) {
        functhis = objthis;
    }

    // 配列の場合
    if(obj.IsInstanceOf(0, nullptr, nullptr, TJS_W("Array"), nullptr) ==
       TJS_S_TRUE) {

        tTJSVariant key, value;
        tTJSVariant **paramList = new tTJSVariant *[numparams];
        paramList[0] = &key;
        paramList[1] = &value;
        for(tjs_int i = 2; i < numparams; i++)
            paramList[i] = param[i];

        tTJSVariant arrayCount;
        obj.PropGet(0, TJS_W("count"), &countHint, &arrayCount, nullptr);
        tjs_int count = arrayCount;

        tTJSVariant breakResult;
        for(tjs_int i = 0; i < count; i++) {
            key = i;
            breakResult.Clear();

            obj.PropGetByNum(TJS_IGNOREPROP, i, &value, nullptr);
            func->FuncCall(0, nullptr, nullptr, &breakResult, numparams,
                           paramList, functhis);
            if(breakResult.Type() != tvtVoid) {
                break;
            }
        }
        if(result) {
            *result = breakResult;
        }

        delete[] paramList;

    } else {

        tTJSVariant **paramList = new tTJSVariant *[numparams];
        for(tjs_int i = 2; i < numparams; i++)
            paramList[i] = param[i];

        DictIterateCaller *caller =
            new DictIterateCaller(func, functhis, paramList, numparams);
        tTJSVariantClosure closure(caller);
        obj.EnumMembers(TJS_IGNOREPROP, &closure, nullptr);
        if(result) {
            *result = caller->breakResult;
        }
        caller->

            Release();

        delete[] paramList;
    }
    return TJS_S_OK;
}

/**
 * octet の MD5ハッシュ値の取得
 * @param octet 対象オクテット
 * @return ハッシュ値（32文字の16進数ハッシュ文字列（小文字））
 */
tjs_error ScriptsAdd::getMD5HashString(tTJSVariant *result, tjs_int numparams,
                                       tTJSVariant **param,
                                       iTJSDispatch2 *objthis) {
    if(numparams < 1)
        return TJS_E_BADPARAMCOUNT;

    tTJSVariantOctet *octet = param[0]->AsOctetNoAddRef();

    TVP_md5_state_t st;
    TVP_md5_init(&st);
    TVP_md5_append(&st, octet->GetData(), (int)octet->GetLength());

    tjs_uint8 buffer[16];
    TVP_md5_finish(&st, buffer);

    tjs_char ret[32 + 1];
    const tjs_char *hex = TJS_W("0123456789abcdef");
    for(tjs_int i = 0; i < 16; i++) {
        ret[i * 2] = hex[(buffer[i] >> 4) & 0xF];
        ret[i * 2 + 1] = hex[(buffer[i]) & 0xF];
    }
    ret[32] = 0;
    if(result)
        *result = ttstr(ret);
    return TJS_S_OK;
}

//----------------------------------------------------------------------
// 辞書の要素を全cloneするCaller
class DictMemberCloneCaller : public tTJSDispatch {
public:
    DictMemberCloneCaller(iTJSDispatch2 *dict) : dict(dict){};

    virtual tjs_error FuncCall( // function invocation
        tjs_uint32 flag, // calling flag
        const tjs_char *membername, // member name ( nullptr for a
                                    // default member )
        tjs_uint32 *hint, // hint for the member name (in/out)
        tTJSVariant *result, // result
        tjs_int numparams, // number of parameters
        tTJSVariant **param, // parameters
        iTJSDispatch2 *objthis // object as "this"
    ) {
        tTJSVariant value = ScriptsAdd::clone(*param[2]);
        dict->PropSet(TJS_MEMBERENSURE | (tjs_int)*param[1],
                      param[0]->GetString(), 0, &value, dict);
        if(result)
            *result = true;
        return TJS_S_OK;
    }

protected:
    iTJSDispatch2 *dict;
};

//----------------------------------------------------------------------
// 構造体比較関数
tTJSVariant ScriptsAdd::clone(tTJSVariant obj) {
    // タイプがオブジェクトなら細かく判定
    if(obj.Type() == tvtObject) {

        tTJSVariantClosure &o1 = obj.AsObjectClosureNoAddRef();

        // Arrayの複製
        if(o1.IsInstanceOf(0, nullptr, nullptr, TJS_W("Array"), nullptr) ==
           TJS_S_TRUE) {
            iTJSDispatch2 *array = TJSCreateArrayObject();
            tTJSVariant o1Count;
            o1.PropGet(0, TJS_W("count"), &countHint, &o1Count, nullptr);
            tjs_int count = o1Count;
            tTJSVariant val;
            tTJSVariant *args[] = { &val };
            for(tjs_int i = 0; i < count; i++) {
                o1.PropGetByNum(TJS_IGNOREPROP, i, &val, nullptr);
                val = ScriptsAdd::clone(val);
                static tjs_uint addHint = 0;
                array->FuncCall(0, TJS_W("add"), &addHint, 0, 1, args, array);
            }
            tTJSVariant result(array, array);
            array->

                Release();

            return result;
        }

        // Dictionaryの複製
        if(o1.IsInstanceOf(0, nullptr, nullptr, TJS_W("Dictionary"), nullptr) ==
           TJS_S_TRUE) {
            iTJSDispatch2 *dict = TJSCreateDictionaryObject();
            DictMemberCloneCaller *caller = new DictMemberCloneCaller(dict);
            tTJSVariantClosure closure(caller);
            o1.EnumMembers(TJS_IGNOREPROP, &closure, nullptr);
            caller->Release();

            tTJSVariant result(dict, dict);
            dict->Release();

            return result;
        }

        // cloneメソッドの呼び出しに成功すればそれを返す
        tTJSVariant result;
        static tjs_uint cloneHint = 0;
        if(o1.FuncCall(0, TJS_W("clone"), &cloneHint, &result, 0, nullptr,
                       nullptr) == TJS_S_TRUE) {
            return result;
        }
    }

    return obj;
}

//----------------------------------------------------------------------
// フラグ指定つきプロパティ操作
tjs_error ScriptsAdd::propSet(tTJSVariant *result, tjs_int numparams,
                              tTJSVariant **param, iTJSDispatch2 *objthis) {
    if(numparams < 3)
        return TJS_E_BADPARAMCOUNT;
    tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();

    tjs_uint32 flag = (numparams > 3) ? (tjs_uint32)param[3]->operator tjs_int()
                                      : TJS_MEMBERENSURE;
    return ((param[1]->

             Type()

             != tvtInteger)
                ? Try_iTJSDispatch2_PropSet(
                      clo.Object, flag, param[1]->GetString(),
                      param[1]->GetHint(), param[2], clo.ObjThis)
                : Try_iTJSDispatch2_PropSetByNum(clo.Object, flag,
                                                 param[1]->operator tjs_int(),
                                                 param[2], clo.ObjThis));
}

tjs_error ScriptsAdd::propGet(tTJSVariant *result, tjs_int numparams,
                              tTJSVariant **param, iTJSDispatch2 *objthis) {
    if(numparams < 2)
        return TJS_E_BADPARAMCOUNT;
    tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();

    tjs_uint32 flag = (numparams > 2) ? (tjs_uint32)param[2]->operator tjs_int()
                                      : TJS_MEMBERMUSTEXIST;
    return ((param[1]->Type() != tvtInteger)
                ? Try_iTJSDispatch2_PropGet(
                      clo.Object, flag, param[1]->GetString(),
                      param[1]->GetHint(), result, clo.ObjThis)
                : Try_iTJSDispatch2_PropGetByNum(clo.Object, flag,
                                                 param[1]->operator tjs_int(),
                                                 result, clo.ObjThis));
}

//----------------------------------------------------------------------
// (const)つき辞書／配列を安全に評価
tjs_error ScriptsAdd::safeEvalStorage(tTJSVariant *result, tjs_int numparams,
                                      tTJSVariant **param,
                                      iTJSDispatch2 *objthis) {
    if(numparams < 1)
        return TJS_E_BADPARAMCOUNT;

    ttstr name = *param[0];

    ttstr modestr;
    if(numparams >= 2 && param[1]->Type() != tvtVoid)
        modestr = *param[1];

    iTJSDispatch2 *context = numparams >= 3 && param[2]->Type() != tvtVoid
        ? param[2]->AsObjectNoAddRef()
        : nullptr;

    ttstr shortname(TVPExtractStorageName(name));

    iTJSTextReadStream *stream = TVPCreateTextStreamForRead(name, modestr);
    ttstr buffer;
    try {
        stream->Read(buffer, 0);
    } catch(...) {
        stream->Destruct();
        throw;
    }
    stream->

        Destruct();

    /*
    ttstr content(TJS_W("(const)["));
    content += buffer;
    content += TJS_W("]");
    buffer = content;
     */
    tjs_int length = buffer.length();
    tjs_char *top =
        buffer.AppendBuffer(8 + 1); // [MEMO] "(const)[]".length == 9
    memmove(top + 8, top,
            sizeof(tjs_char) * length); // xxxxxxxx<buffer>x
    memcpy(top, TJS_W("(const)["),
           sizeof(tjs_char) * 8); // (const)[<buffer>x
    memcpy(top + 8 + length, TJS_W("]"),
           sizeof(tjs_char) * 1); // (const)[<buffer>]
    buffer.FixLen();
    // TVPAddLog(buffer);

    tTJSVariant temp;
    TVPExecuteExpression(buffer, shortname, 0, context, &temp);
    if(result) {
        tTJSVariantClosure clo;
        clo = temp.AsObjectClosureNoAddRef();
        if(clo.Object) {
            clo.PropGetByNum(TJS_IGNOREPROP, 0, result, nullptr);
        }
    }

    return TJS_S_OK;
}
//----------------------------------------------------------------------
NCB_ATTACH_CLASS(ScriptsAdd, Scripts) {
    RawCallback(TJS_W("getObjectKeys"), &ScriptsAdd::getKeys, TJS_STATICMEMBER);
    RawCallback(TJS_W("getObjectCount"), &ScriptsAdd::getCount,
                TJS_STATICMEMBER);
    NCB_METHOD(getObjectContext);
    NCB_METHOD(isNullContext);
    NCB_METHOD(equalStruct);
    NCB_METHOD(equalStructNumericLoose);
    RawCallback(TJS_W("foreach"), &ScriptsAdd::foreach, TJS_STATICMEMBER);
    RawCallback(TJS_W("getMD5HashString"), &ScriptsAdd::getMD5HashString,
                TJS_STATICMEMBER);
    NCB_METHOD(clone);

    RawCallback("propSet", &ScriptsAdd::propSet, TJS_STATICMEMBER);
    RawCallback("propGet", &ScriptsAdd::propGet, TJS_STATICMEMBER);
    Variant(TJS_W("pfMemberEnsure"), TJS_MEMBERENSURE);
    Variant(TJS_W("pfMemberMustExist"), TJS_MEMBERMUSTEXIST);
    Variant(TJS_W("pfIgnoreProp"), TJS_IGNOREPROP);
    Variant(TJS_W("pfHiddenMember"), TJS_HIDDENMEMBER);
    Variant(TJS_W("pfStaticMember"), TJS_STATICMEMBER);

    RawCallback(TJS_W("safeEvalStorage"), &ScriptsAdd::safeEvalStorage,
                TJS_STATICMEMBER);
};

NCB_ATTACH_FUNCTION(rehash, Scripts, TJSDoRehash);