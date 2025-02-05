#include <ncbind.hpp>
#include "combase.h"
#include "EventIntf.h"
#include "ScriptMgnIntf.h"
#include "psdclass.h"

// -----------------------------------------------------------------------------
// ストレージ機能
// -----------------------------------------------------------------------------
#define BASENAME TJS_W("psd")

/**
 * PSDストレージ
 */
class PSDStorage : public iTVPStorageMedia, tTVPCompactEventCallbackIntf {
public:
    /**
     * コンストラクタ
     */
    PSDStorage() : refCount(1) {}

    /**
     * デストラクタ
     */
    virtual ~PSDStorage() { clearCache(); }

    /*
     * PSDオブジェクト参照を追加
     * @param filename 参照ファイル名
     * @param psd PSDオブジェクト
     */
    void add(ttstr filename, PSD *psd) { psdMap[filename] = psd; }

    /**
     * PSDオブジェクト参照の消去要求
     * @param psd PSDオブジェクト
     */
    void remove(PSD *psd) {
        PSDMap::iterator it = psdMap.begin();
        while(it != psdMap.end()) {
            if(it->second == psd) {
                it = psdMap.erase(it);
            } else {
                it++;
            }
        }
    }

    /**
     * キャッシュ情報をクリアする
     */
    void clearCache() { cache.Clear(); }

public:
    // -----------------------------------
    // iTVPStorageMedia Intefaces
    // -----------------------------------

    virtual void AddRef() { refCount++; };

    virtual void Release() {
        if(refCount == 1) {
            delete this;
        } else {
            refCount--;
        }
    };

    // returns media name like "file", "http" etc.
    virtual void GetName(ttstr &name) { name = BASENAME; }

    //	virtual ttstr IsCaseSensitive() = 0;
    // returns whether this media is case sensitive or not

    // normalize domain name according with the media's rule
    virtual void NormalizeDomainName(ttstr &name) {
        // nothing to do
    }

    // normalize path name according with the media's rule
    virtual void NormalizePathName(ttstr &name) {
        // nothing to do
    }

    // check file existence
    virtual bool CheckExistentStorage(const ttstr &name) {
        ttstr fname;
        PSD *psd = getPSD(name, fname);
        if(psd) {
            bool ret = psd->CheckExistentStorage(fname);
            return ret;
        }
        return false;
    }

    // open a storage and return a iTJSBinaryStream instance.
    // name does not contain in-archive storage name but
    // is normalized.
    virtual tTJSBinaryStream *Open(const ttstr &name, tjs_uint32 flags) {
        if(flags == TJS_BS_READ) { // 読み込みのみ
            ttstr fname;
            PSD *psd = getPSD(name, fname);
            if(psd) {
                IStream *stream = psd->openLayerImage(fname);
                if(stream) {

                    tTJSBinaryStream *ret =
                        TVPCreateBinaryStreamAdapter(stream);
                    stream->Release();
                    return ret;
                }
            }
        }
        TVPThrowExceptionMessage(TJS_W("%1:cannot open psdfile"), name);
        return nullptr;
    }

    // list files at given place
    virtual void GetListAt(const ttstr &name, iTVPStorageLister *lister) {
        ttstr fname;
        PSD *psd = getPSD(name, fname);
        if(psd) {
            psd->GetListAt(fname, lister);
        }
    }

    // basically the same as above,
    // check wether given name is easily accessible from local OS
    // filesystem. if true, returns local OS native name. otherwise
    // returns an empty string.
    virtual void GetLocallyAccessibleName(ttstr &name) { name = ""; }

public:
    // -----------------------------------
    // tTVPCompactEventCallbackIntf
    // -----------------------------------
    virtual void OnCompact(tjs_int level) {
        if(level > TVP_COMPACT_LEVEL_MINIMIZE) {
            clearCache();
        }
    }

protected:
    /*
     * ファイル名に合致した PSD 情報を取得
     * @param name ファイル名
     * @param fname ファイル名を返す
     * @return PSD情報
     */
    PSD *getPSD(ttstr name, ttstr &fname) {
        // 小文字で正規化
        name.ToLowerCase();
        // ドメイン名とそれ以降を分離
        tTJSString dname;
        const tjs_char *p = name.c_str();
        const tjs_char *q;
        if((q = TJS_strchr(p, '/'))) {
            dname = ttstr(p, q - p);
            fname = ttstr(q + 1);
        } else {
            TVPThrowExceptionMessage(TJS_W("invalid path:%1"), name);
        }

        PSD *psd = 0;

        // 直近のキャッシュが合致する場合はそれを返す
        if(cache.Type() == tvtObject &&
           (psd = ncbInstanceAdaptor<PSD>::GetNativeInstance(
                cache.AsObjectNoAddRef())) &&
           psd->dname == dname) {
            return psd;
        }

        // PSDオブジェクトの弱参照から探す
        PSDMap::iterator it = psdMap.find(dname);
        if(it != psdMap.end()) {
            // 既存データがある
            cache = it->second->getSelf();
            return it->second;
        }

        // 自分でopenしてそのままキャッシュとして持つ
        TVPExecuteExpression(TJS_W("new PSD()"), &cache);
        psd = ncbInstanceAdaptor<PSD>::GetNativeInstance(
            cache.AsObjectNoAddRef());
        if(psd) {
            if(psd->load(dname.c_str())) {
                return psd;
            } else {
                clearCache();
            }
        }

        return 0;
    }

private:
    tjs_uint refCount; //< リファレンスカウント

    // PSDオブジェクトのキャッシュ参照
    tTJSVariant cache;

    // PSDオブジェクトの弱参照
    typedef std::map<ttstr, PSD *> PSDMap;
    PSDMap psdMap;
};

static PSDStorage *psdStorage = 0;

// 弱参照追加
void PSD::addToStorage(const ttstr &filename) {
    // 登録用ベース名を生成
    const tjs_char *p = filename.c_str();
    const tjs_char *q;
    if((q = TJS_strchr(p, '/'))) {
        dname = ttstr(q + 1);
    } else {
        dname = filename;
    }
    // 小文字で正規化
    dname.ToLowerCase();
    if(psdStorage != nullptr) {
        psdStorage->add(dname, this);
    }
}

// 弱参照解除
void PSD::removeFromStorage() {
    if(psdStorage != nullptr) {
        psdStorage->remove(this);
    }
}

void PSD::clearStorageCache() {
    if(psdStorage != nullptr) {
        psdStorage->clearCache();
    }
}

// --------------------------------------------------------------------

void initStorage() {
    if(psdStorage == nullptr) {
        psdStorage = new PSDStorage();
        TVPRegisterStorageMedia(psdStorage);
        TVPAddCompactEventHook((tTVPCompactEventCallbackIntf *)psdStorage);
    }
}

void doneStorage() {
    if(psdStorage != nullptr) {
        TVPRemoveCompactEventHook((tTVPCompactEventCallbackIntf *)psdStorage);
        TVPUnregisterStorageMedia(psdStorage);
        psdStorage->Release();
        psdStorage = nullptr;
    }
}

NCB_PRE_REGIST_CALLBACK(initStorage);
NCB_POST_UNREGIST_CALLBACK(doneStorage);
