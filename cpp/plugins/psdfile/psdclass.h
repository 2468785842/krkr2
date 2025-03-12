#ifndef __PSDCLASS_H__
#define __PSDCLASS_H__

#define NCB_MODULE_NAME TJS_W("psd.dll")

// #define LOAD_MEMORY
#include "typedefine.h"
#include "tp_stub.h"
#include "psdparse/psdfile.h"
#include "ncbind.hpp"

class PSDStorage;
class PSDIterator;

class PSD : public psd::PSDFile {
    friend class PSDStorage;
    friend class PSDIterator;

public:
    /**
     * コンストラクタ
     */
    PSD(iTJSDispatch2 *objthis);

    /**
     * デストラクタ
     */
    ~PSD();

    /**
     * 内包データの消去
     */
    virtual void clearData();

    /**
     * インスタンス生成ファクトリ
     */
    static tjs_error factory(PSD **result, tjs_int numparams,
                             tTJSVariant **params, iTJSDispatch2 *objthis);

    /**
     * 生成時の自己オブジェクトを取得
     */
    tTJSVariant getSelf();

    /**
     * PSD画像のロード
     * @param filename ファイル名
     * @return ロードに成功したら true
     */
    bool load(const tjs_char *filename);

    static void clearStorageCache();

#define INTGETTER(tag)                                                         \
    int get_##tag() { return isLoaded ? header.tag : -1; }

    INTGETTER(width);
    INTGETTER(height);
    INTGETTER(channels);
    INTGETTER(depth);
    int get_color_mode() { return isLoaded ? header.mode : -1; }
    int get_layer_count() { return isLoaded ? (int)layerList.size() : -1; }

public:
    /**
     * レイヤ種別の取得
     * @param no レイヤ番号
     * @return レイヤ種別
     */
    int getLayerType(int no);

    /**
     * レイヤ名称の取得
     * @param no レイヤ番号
     * @return レイヤ種別
     */
    ttstr getLayerName(int no);

    /**
     * レイヤ情報の取得
     * @param no レイヤ番号
     * @return レイヤ情報が格納された辞書
     */
    tTJSVariant getLayerInfo(int no);

    /**
     * レイヤデータの読み出し(内部処理)
     * @param layer 読み出し先レイヤ
     * @param no レイヤ番号
     * @param imageMode イメージモード
     */
    void _getLayerData(tTJSVariant layer, int no, psd::ImageMode imageMode);

    /**
     * レイヤデータの読み出し
     * @param layer 読み出し先レイヤ
     * @param no レイヤ番号
     */
    void getLayerData(tTJSVariant layer, int no);

    /**
     * レイヤデータの読み出し(生イメージ)
     * @param layer 読み出し先レイヤ
     * @param no レイヤ番号
     */
    void getLayerDataRaw(tTJSVariant layer, int no);

    /**
     * レイヤデータの読み出し(マスクのみ)
     * @param layer 読み出し先レイヤ
     * @param no レイヤ番号
     */
    void getLayerDataMask(tTJSVariant layer, int no);

    /**
     * スライスデータの読み出し
     * @return スライス情報辞書 %[ top, left, bottom, right, slices:[
     * %[ id, group_id, left, top, bottom, right ], ... ] ]
     * スライス情報がない場合は void を返す
     */
    tTJSVariant getSlices();

    /**
     * ガイドデータの読み出し
     * @return ガイド情報辞書 %[ vertical:[ x1, x2, ... ],
     * horizontal:[ y1, y2,
     * ... ] ] ガイド情報がない場合は void を返す
     */
    tTJSVariant getGuides();

    /**
     * 合成結果の取得。取得領域は画像全体サイズ内におさまってる必要があります
     * 注意：PSDファイル自体に合成済み画像が存在しない場合は取得に失敗します
     *
     * @param layer 格納先レイヤ(width,heightサイズに調整される)
     * @return 取得に成功したら true
     */
    bool getBlend(tTJSVariant layer);

    /**
     * レイヤーカンプ
     */
    tTJSVariant getLayerComp();

protected:
    iTJSDispatch2 *objthis; ///< 自己オブジェクト情報の参照
    ttstr dname; ///< 登録用ベース名

#ifdef LOAD_MEMORY
    HGLOBAL hBuffer; // オンメモリ保持用ハンドル
    bool loadMemory(const ttstr &filename);
    void clearMemory();
#else
    // ストリームから読み込み
    IStream *pStream;
    tTVInteger mStreamSize;
    bool loadStream(const ttstr &filename);
    void clearStream();
    unsigned char &getStreamValue(const tTVInteger &pos);
    void copyToBuffer(uint8_t *buf, tTVInteger pos, int size);

    //< PSDファイル読み込みキャッシュ用バッファ
    tTVInteger mBufferPos;
    ULONG mBufferSize;
    unsigned char mBuffer[4 * 1024];
#endif

    /**
     * レイヤ番号が適切かどうか判定
     * @param no レイヤ番号
     */
    void checkLayerNo(int no);

    /**
     * 名前の取得
     * @param layレイヤ情報
     */
    static ttstr layname(psd::LayerInfo &lay);

    // ------------------------------------------------------------
    // ストレージレイヤ参照用インターフェース
    // ------------------------------------------------------------

protected:
    // ストレージ情報登録
    void addToStorage(const ttstr &filename);
    void removeFromStorage();

    bool storageStarted; //< ストレージ用の情報初期化済みフラグ

    // レイヤ名を返す
    static ttstr path_layname(psd::LayerInfo &lay);

    // レイヤのパス名を返す
    static ttstr pathname(psd::LayerInfo &lay);

    // ストレージ処理用データの初期化
    void startStorage();

    /*
     * 指定した名前のレイヤの存在チェック
     * @param name パスを含むレイヤ名
     * @param layerIdxRet レイヤインデックス番号を返す
     */
    bool CheckExistentStorage(const ttstr &filename, int *layerIdxRet = 0);

    /*
     * 指定したパスにあるファイル名一覧の取得
     * @param pathname パス名
     * @param lister リスト取得用インターフェース
     */
    void GetListAt(const ttstr &pathname, iTVPStorageLister *lister);

    /*
     * 指定した名前のレイヤの画像ファイルをストリームで返す
     * @param name パスを含むレイヤ名
     * @return ファイルストリーム
     */
    IStream *openLayerImage(const ttstr &name);

    // パス名記録用

    typedef std::map<int, int>
        LayerIdIdxMap; // layerId とレイヤ情報インデックスのマップ
    LayerIdIdxMap layerIdIdxMap;

    typedef std::map<ttstr, int> NameIdxMap; //< レイヤ名とlayerId のマップ
    typedef std::map<ttstr, NameIdxMap> PathMap; //< パス別のレイヤ名一覧
    PathMap pathMap;
};

#endif
