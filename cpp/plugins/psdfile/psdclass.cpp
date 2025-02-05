#include "PluginStub.h"
#include "GraphicsLoaderIntf.h"
#include "MsgIntf.h"
#include "psdclass.h"

using namespace psd;

const auto BMPEXT = TJS_W(".bmp");

// ncb.typeconv: cast: enum->int
NCB_TYPECONV_CAST_INTEGER(psd::LayerType);
NCB_TYPECONV_CAST_INTEGER(psd::BlendMode);

static int convBlendMode(psd::BlendMode mode) {
    switch(mode) {
        case psd::BLEND_MODE_NORMAL: // 'norm' = normal
            return ltPsNormal;
        case psd::BLEND_MODE_DARKEN: // 'dark' = darken
            return ltPsDarken;
        case psd::BLEND_MODE_MULTIPLY: // 'mul ' = multiply
            return ltPsMultiplicative;
        case psd::BLEND_MODE_COLOR_BURN: // 'idiv' = color burn
            return ltPsColorBurn;
        case psd::BLEND_MODE_LINEAR_BURN: // 'lbrn' = linear burn
            return ltPsSubtractive;
        case psd::BLEND_MODE_LIGHTEN: // 'lite' = lighten
            return ltPsLighten;
        case psd::BLEND_MODE_SCREEN: // 'scrn' = screen
            return ltPsScreen;
        case psd::BLEND_MODE_COLOR_DODGE: // 'div ' = color dodge
            return ltPsColorDodge;
        case psd::BLEND_MODE_LINEAR_DODGE: // 'lddg' = linear dodge
            return ltPsAdditive;
        case psd::BLEND_MODE_OVERLAY: // 'over' = overlay
            return ltPsOverlay;
        case psd::BLEND_MODE_SOFT_LIGHT: // 'sLit' = soft light
            return ltPsSoftLight;
        case psd::BLEND_MODE_HARD_LIGHT: // 'hLit' = hard light
            return ltPsHardLight;
        case psd::BLEND_MODE_DIFFERENCE: // 'diff' = difference
            return ltPsDifference;
        case psd::BLEND_MODE_EXCLUSION: // 'smud' = exclusion
            return ltPsExclusion;
        case psd::BLEND_MODE_DISSOLVE: // 'diss' = dissolve
        case psd::BLEND_MODE_VIVID_LIGHT: // 'vLit' = vivid light
        case psd::BLEND_MODE_LINEAR_LIGHT: // 'lLit' = linear light
        case psd::BLEND_MODE_PIN_LIGHT: // 'pLit' = pin light
        case psd::BLEND_MODE_HARD_MIX: // 'hMix' = hard mix
        case psd::BLEND_MODE_DARKER_COLOR:
        case psd::BLEND_MODE_LIGHTER_COLOR:
        case psd::BLEND_MODE_SUBTRACT:
        case psd::BLEND_MODE_DIVIDE:
            // not supported;
            break;
    }
    return ltPsNormal;
}

/**
 * C文字列処理用
 */
class NarrowString {
private:
    tjs_nchar *_data;

public:
    NarrowString(const ttstr &str) : _data(nullptr) {
        tjs_int len = str.GetNarrowStrLen();
        if(len > 0) {
            _data = new tjs_nchar[len + 1];
            str.ToNarrowStr(_data, len + 1);
        }
    }

    ~NarrowString() { delete[] _data; }

    const tjs_nchar *data() { return _data; }

    operator const char *() const { return (const char *)_data; }
};

/**
 * コンストラクタ
 */
PSD::PSD(iTJSDispatch2 *objthis) :
    objthis(objthis)
#ifdef LOAD_MEMORY
    ,
    hBuffer(0)
#else
    ,
    pStream(nullptr), mStreamSize(0), mBufferPos(0), mBufferSize(0)
#endif
    ,
    storageStarted(false){};

/**
 * デストラクタ
 */
PSD::~PSD() {
    clearData(); // ここで呼ばないと delete
                 // 時には親のほうでは仮想関数がよばれない
};

/**
 * インスタンス生成ファクトリ
 */
tjs_error PSD::factory(PSD **result, tjs_int numparams, tTJSVariant **params,
                       iTJSDispatch2 *objthis) {
    *result = new PSD(objthis);
    return TJS_S_OK;
}

/**
 * 生成時の自己オブジェクトを取得
 */
tTJSVariant PSD::getSelf() { return tTJSVariant(objthis, objthis); }

/**
 * PSD画像のロード
 * @param filename ファイル名
 * @return ロードに成功したら true
 */
bool PSD::load(const tjs_char *filename) {
    ttstr file = TVPGetPlacedPath(filename);
    if(!file.length()) {
        // 見つからなかったのでローカルパスとみなして読み込む
        psd::PSDFile::load(NarrowString(filename));
    } else {
#ifdef LOAD_MEMORY
        if(!wcschr(file.c_str(), '>')) {
            // ローカルファイルなので直接読み込む
            TVPGetLocalName(file);
            psd::PSDFile::load(NarrowString(file));
        } else {
            // メモリに読み込んでロード
            loadMemory(file);
        }
#else
        // ストリームとしてロード
        loadStream(file);
#endif
    }
    if(isLoaded) {
        addToStorage(filename);
    }
    return isLoaded;
}

void PSD::clearData() {
    removeFromStorage();
    layerIdIdxMap.clear();
    pathMap.clear();
    storageStarted = false;

    psd::PSDFile::clearData();
#ifdef LOAD_MEMORY
    clearMemory();
#else
    clearStream();
#endif
}

/**
 * レイヤ番号が適切かどうか判定
 * @param no レイヤ番号
 */
void PSD::checkLayerNo(int no) {
    if(!isLoaded) {
        TVPThrowExceptionMessage(TJS_W("no data"));
    }
    if(no < 0 || no >= get_layer_count()) {
        TVPThrowExceptionMessage(TJS_W("not such layer"));
    }
}

/**
 * 名前の取得
 * @param layレイヤ情報
 */
ttstr PSD::layname(psd::LayerInfo &lay) {
    ttstr ret;
    if(!lay.layerNameUnicode.empty()) {
        ret = ttstr(lay.layerNameUnicode.c_str());
    } else {
        ret = ttstr(lay.layerName.c_str());
    }
    return ret;
}

/**
 * レイヤ種別の取得
 * @param no レイヤ番号
 * @return レイヤ種別
 */
int PSD::getLayerType(int no) {
    checkLayerNo(no);
    return (int)layerList[no].layerType;
}

/**
 * レイヤ名称の取得
 * @param no レイヤ番号
 * @return レイヤ種別
 */
ttstr PSD::getLayerName(int no) {
    checkLayerNo(no);
    return layname(layerList[no]);
}

/**
 * レイヤ情報の取得
 * @param no レイヤ番号
 * @return レイヤ情報が格納された辞書
 */
tTJSVariant PSD::getLayerInfo(int no) {
    checkLayerNo(no);
    psd::LayerInfo &lay = layerList[no];
    tTJSVariant result;
    ncbDictionaryAccessor dict;
    if(dict.IsValid()) {
#define SETPROP(dict, obj, prop) dict.SetValue(TJS_W(#prop), obj.prop)
        SETPROP(dict, lay, top);
        SETPROP(dict, lay, left);
        SETPROP(dict, lay, bottom);
        SETPROP(dict, lay, right);
        SETPROP(dict, lay, width);
        SETPROP(dict, lay, height);
        SETPROP(dict, lay, opacity);
        SETPROP(dict, lay, fill_opacity);
        bool mask = false;
        for(auto &channel : lay.channels) {
            if(channel.isMaskChannel()) {
                mask = true;
                break;
            }
        }
        dict.SetValue(TJS_W("mask"), mask);
        dict.SetValue(TJS_W("type"), convBlendMode(lay.blendMode));
        dict.SetValue(TJS_W("layer_type"), lay.layerType);
        dict.SetValue(TJS_W("blend_mode"), lay.blendMode);
        dict.SetValue(TJS_W("visible"), lay.isVisible());
        dict.SetValue(TJS_W("name"), layname(lay));

        // additional information
        SETPROP(dict, lay, clipping);
        dict.SetValue(TJS_W("layer_id"), lay.layerId);
        dict.SetValue(TJS_W("obsolete"), lay.isObsolete());
        dict.SetValue(TJS_W("transparency_protected"),
                      lay.isTransparencyProtected());
        dict.SetValue(TJS_W("pixel_data_irrelevant"),
                      lay.isPixelDataIrrelevant());

        // レイヤーカンプ
        if(!lay.layerComps.empty()) {
            ncbDictionaryAccessor compDict;
            if(compDict.IsValid()) {
                for(auto &layerComp : lay.layerComps) {
                    ncbDictionaryAccessor tmp;
                    if(tmp.IsValid()) {
                        psd::LayerCompInfo &comp = layerComp.second;
                        tmp.SetValue(TJS_W("id"), comp.id);
                        tmp.SetValue(TJS_W("offset_x"), comp.offsetX);
                        tmp.SetValue(TJS_W("offset_y"), comp.offsetY);
                        tmp.SetValue(TJS_W("enable"), comp.isEnabled);
                        compDict.SetValue((tjs_int32)comp.id,
                                          tmp.GetDispatch());
                    }
                }
                dict.SetValue(TJS_W("layer_comp"), compDict.GetDispatch());
            }
        }

        // SETPROP(dict, lay, adjustment_valid); //
        // 調整レイヤーかどうか？レイヤタイプで判別可能 SETPROP(dict,
        // lay, fill_opacity); SETPROP(dict, lay, layer_name_id);
        // SETPROP(dict, lay, layer_version); SETPROP(dict, lay,
        // blend_clipped); SETPROP(dict, lay, blend_interior);
        // SETPROP(dict, lay, knockout); SETPROP(dict, lay,
        // transparency); // lspf(protection)のもの SETPROP(dict, lay,
        // composite); SETPROP(dict, lay, position_respectively);
        // SETPROP(dict, lay, sheet_color); SETPROP(dict, lay,
        // reference_point_x); //
        // 塗りつぶしレイヤ（パターン）のオフセット SETPROP(dict, lay,
        // reference_point_y); //
        // 塗りつぶしレイヤ（パターン）のオフセット SETPROP(dict, lay,
        // transparency_shapes_layer); SETPROP(dict, lay,
        // layer_mask_hides_effects); SETPROP(dict, lay,
        // vector_mask_hides_effects); SETPROP(dict, lay,
        // divider_type); SETPROP(dict, lay, divider_blend_mode);

        // group layer はスクリプト側では layer_id
        // 参照で引くようにする
        if(lay.parent != nullptr)
            dict.SetValue(TJS_W("group_layer_id"), lay.parent->layerId);

        result = dict;
    }

    return result;
}

/**
 * レイヤデータの読み出し(内部処理)
 * @param layer 読み出し先レイヤ
 * @param no レイヤ番号
 * @param imageMode イメージモード
 */
void PSD::_getLayerData(tTJSVariant layer, int no, psd::ImageMode imageMode) {
    if(!layer.AsObjectNoAddRef()->IsInstanceOf(0, 0, 0, TJS_W("Layer"),
                                               nullptr)) {
        TVPThrowExceptionMessage(TJS_W("not layer"));
    }
    checkLayerNo(no);

    psd::LayerInfo &lay = layerList[no];
    psd::LayerMask &mask = lay.extraData.layerMask;

    if(lay.layerType != psd::LAYER_TYPE_NORMAL &&
       !(lay.layerType == psd::LAYER_TYPE_FOLDER &&
         imageMode == psd::IMAGE_MODE_MASK)) {
        TVPThrowExceptionMessage(TJS_W("invalid layer type"));
    }

    int left, top, width, height, opacity, fill_opacity, type;

    bool dummyMask = false;
    if(imageMode == psd::IMAGE_MODE_MASK) {
        left = mask.left;
        top = mask.top;
        width = mask.width;
        height = mask.height;
        opacity = 255;
        fill_opacity = 255;
        type = ltPsNormal;
        if(width == 0 || height == 0) {
            left = top = 0;
            width = height = 1;
            dummyMask = true;
        }
    } else {
        left = lay.left;
        top = lay.top;
        width = lay.width;
        height = lay.height;
        opacity = lay.opacity;
        fill_opacity = lay.fill_opacity;
        type = convBlendMode(lay.blendMode);
    }
    if(width <= 0 || height <= 0) {
        // サイズ０のレイヤはロードできない
        return;
    }

    ncbPropAccessor obj(layer);
    obj.SetValue(TJS_W("left"), left);
    obj.SetValue(TJS_W("top"), top);
    obj.SetValue(TJS_W("opacity"), opacity);
    obj.SetValue(TJS_W("fill_opacity"), fill_opacity);
    obj.SetValue(TJS_W("width"), width);
    obj.SetValue(TJS_W("height"), height);
    obj.SetValue(TJS_W("type"), type);
    obj.SetValue(TJS_W("visible"), lay.isVisible());
    obj.SetValue(TJS_W("imageLeft"), 0);
    obj.SetValue(TJS_W("imageTop"), 0);
    obj.SetValue(TJS_W("imageWidth"), width);
    obj.SetValue(TJS_W("imageHeight"), height);
    obj.SetValue(TJS_W("name"), layname(lay));

    if(imageMode == psd::IMAGE_MODE_MASK)
        obj.SetValue(TJS_W("defaultMaskColor"), mask.defaultColor);

    // 画像データのコピー
    unsigned char *buffer = (unsigned char *)obj.GetValue(
        TJS_W("mainImageBufferForWrite"), ncbTypedefs::Tag<tjs_intptr_t>());
    int pitch = obj.GetValue(TJS_W("mainImageBufferPitch"),
                             ncbTypedefs::Tag<tjs_int>());
    if(dummyMask) {
        buffer[0] = buffer[1] = buffer[2] = mask.defaultColor;
        buffer[3] = 255;
    } else {
        getLayerImage(lay, buffer, psd::BGRA_LE, pitch, imageMode);
    }
}

/**
 * レイヤデータの読み出し
 * @param layer 読み出し先レイヤ
 * @param no レイヤ番号
 */
void PSD::getLayerData(tTJSVariant layer, int no) {
    _getLayerData(layer, no, psd::IMAGE_MODE_MASKEDIMAGE);
}

/**
 * レイヤデータの読み出し(生イメージ)
 * @param layer 読み出し先レイヤ
 * @param no レイヤ番号
 */
void PSD::getLayerDataRaw(tTJSVariant layer, int no) {
    _getLayerData(layer, no, psd::IMAGE_MODE_IMAGE);
}

/**
 * レイヤデータの読み出し(マスクのみ)
 * @param layer 読み出し先レイヤ
 * @param no レイヤ番号
 */
void PSD::getLayerDataMask(tTJSVariant layer, int no) {
    _getLayerData(layer, no, psd::IMAGE_MODE_MASK);
}

/**
 * スライスデータの読み出し
 * @return スライス情報辞書 %[ top, left, bottom, right, slices:[ %[
 * id, group_id, left, top, bottom, right ], ... ] ]
 * スライス情報がない場合は void を返す
 */
tTJSVariant PSD::getSlices() {
    if(!isLoaded)
        TVPThrowExceptionMessage(TJS_W("no data"));
    tTJSVariant result;
    ncbDictionaryAccessor dict;
    ncbArrayAccessor arr;
    if(slice.isEnabled) {
        if(dict.IsValid()) {
            psd::SliceResource &sr = slice;
            dict.SetValue(TJS_W("top"), sr.boundingTop);
            dict.SetValue(TJS_W("left"), sr.boundingLeft);
            dict.SetValue(TJS_W("bottom"), sr.boundingBottom);
            dict.SetValue(TJS_W("right"), sr.boundingRight);
            dict.SetValue(TJS_W("name"), ttstr(sr.groupName.c_str()));
            if(arr.IsValid()) {
                for(int i = 0; i < (int)sr.slices.size(); i++) {
                    ncbDictionaryAccessor tmp;
                    if(tmp.IsValid()) {
                        psd::SliceItem &item = sr.slices[i];
                        tmp.SetValue(TJS_W("id"), item.id);
                        tmp.SetValue(TJS_W("group_id"), item.groupId);
                        tmp.SetValue(TJS_W("origin"), item.origin);
                        tmp.SetValue(TJS_W("type"), item.type);
                        tmp.SetValue(TJS_W("left"), item.left);
                        tmp.SetValue(TJS_W("top"), item.top);
                        tmp.SetValue(TJS_W("right"), item.right);
                        tmp.SetValue(TJS_W("bottom"), item.bottom);
                        tmp.SetValue(TJS_W("color"),
                                     ((item.colorA << 24) |
                                      (item.colorR << 16) | (item.colorG << 8) |
                                      item.colorB));
                        tmp.SetValue(TJS_W("cell_text_is_html"),
                                     item.isCellTextHtml);
                        tmp.SetValue(TJS_W("horizontal_alignment"),
                                     item.horizontalAlign);
                        tmp.SetValue(TJS_W("vertical_alignment"),
                                     item.verticalAlign);
                        tmp.SetValue(TJS_W("associated_layer_id"),
                                     item.associatedLayerId);
                        tmp.SetValue(TJS_W("name"), ttstr(item.name.c_str()));
                        tmp.SetValue(TJS_W("url"), ttstr(item.url.c_str()));
                        tmp.SetValue(TJS_W("target"),
                                     ttstr(item.target.c_str()));
                        tmp.SetValue(TJS_W("message"),
                                     ttstr(item.message.c_str()));
                        tmp.SetValue(TJS_W("alt_tag"),
                                     ttstr(item.altTag.c_str()));
                        tmp.SetValue(TJS_W("cell_text"),
                                     ttstr(item.cellText.c_str()));
                        arr.SetValue((tjs_int32)i, tmp.GetDispatch());
                    }
                }
                dict.SetValue(TJS_W("slices"), arr.GetDispatch());
            }
            result = dict;
        }
    }
    return result;
}

/**
 * ガイドデータの読み出し
 * @return ガイド情報辞書 %[ vertical:[ x1, x2, ... ], horizontal:[
 * y1, y2, ... ] ] ガイド情報がない場合は void を返す
 */
tTJSVariant PSD::getGuides() {
    if(!isLoaded)
        TVPThrowExceptionMessage(TJS_W("no data"));
    tTJSVariant result;
    ncbDictionaryAccessor dict;
    ncbArrayAccessor vert, horz;
    if(gridGuide.isEnabled) {
        psd::GridGuideResource gg = gridGuide;
        if(dict.IsValid() && vert.IsValid() && horz.IsValid()) {
            dict.SetValue(TJS_W("horz_grid"), gg.horizontalGrid);
            dict.SetValue(TJS_W("vert_grid"), gg.verticalGrid);
            dict.SetValue(TJS_W("vertical"), vert.GetDispatch());
            dict.SetValue(TJS_W("horizontal"), horz.GetDispatch());
            for(int i = 0, v = 0, h = 0; i < (int)gg.guides.size(); i++) {
                if(gg.guides[i].direction == 0) {
                    vert.SetValue(v++, gg.guides[i].location);
                } else {
                    horz.SetValue(h++, gg.guides[i].location);
                }
            }
            result = dict;
        }
    }
    return result;
}

/**
 * 合成結果の取得。取得領域は画像全体サイズ内におさまってる必要があります
 * 注意：PSDファイル自体に合成済み画像が存在しない場合は取得に失敗します
 *
 * @param layer 格納先レイヤ(width,heightサイズに調整される)
 * @return 取得に成功したら true
 */
bool PSD::getBlend(tTJSVariant layer) {
    if(!layer.AsObjectNoAddRef()->IsInstanceOf(0, 0, 0, TJS_W("Layer"),
                                               nullptr)) {
        TVPThrowExceptionMessage(TJS_W("not layer"));
    }

    // 合成結果を生成
    if(imageData) {

        // 格納先を調整
        ncbPropAccessor obj(layer);
        obj.SetValue(TJS_W("width"), get_width());
        obj.SetValue(TJS_W("height"), get_height());
        obj.SetValue(TJS_W("imageLeft"), 0);
        obj.SetValue(TJS_W("imageTop"), 0);
        obj.SetValue(TJS_W("imageWidth"), get_width());
        obj.SetValue(TJS_W("imageHeight"), get_height());

        // 画像データのコピー
        unsigned char *buffer = (unsigned char *)obj.GetValue(
            TJS_W("mainImageBufferForWrite"), ncbTypedefs::Tag<tjs_intptr_t>());
        int pitch = obj.GetValue(TJS_W("mainImageBufferPitch"),
                                 ncbTypedefs::Tag<tjs_int>());
        getMergedImage(buffer, psd::BGRA_LE, pitch);

        return true;
    }

    return false;
}

/**
 * レイヤーカンプ
 */
tTJSVariant PSD::getLayerComp() {
    if(!isLoaded)
        TVPThrowExceptionMessage(TJS_W("no data"));
    tTJSVariant result;
    ncbDictionaryAccessor dict;
    ncbArrayAccessor arr;
    int compNum = layerComps.size();
    if(compNum > 0) {
        if(dict.IsValid()) {
            dict.SetValue(TJS_W("last_applied_id"), lastAppliedCompId);
            if(arr.IsValid()) {
                for(int i = 0; i < compNum; i++) {
                    ncbDictionaryAccessor tmp;
                    if(tmp.IsValid()) {
                        psd::LayerComp &comp = layerComps[i];
                        tmp.SetValue(TJS_W("id"), comp.id);
                        tmp.SetValue(TJS_W("record_visibility"),
                                     comp.isRecordVisibility);
                        tmp.SetValue(TJS_W("record_position"),
                                     comp.isRecordPosition);
                        tmp.SetValue(TJS_W("record_appearance"),
                                     comp.isRecordAppearance);
                        tmp.SetValue(TJS_W("name"), ttstr(comp.name.c_str()));
                        tmp.SetValue(TJS_W("comment"),
                                     ttstr(comp.comment.c_str()));
                        arr.SetValue((tjs_int32)i, tmp.GetDispatch());
                    }
                }
                dict.SetValue(TJS_W("comps"), arr.GetDispatch());
            }
            result = dict;
        }
    }
    return result;
}

// レイヤ名を返す
ttstr PSD::path_layname(psd::LayerInfo &lay) {
    ttstr ret = layname(lay);
    // 正規化
    ttstr from = "/";
    ttstr to = "_";
    ret.Replace(from, to, true);
    ret.ToLowerCase();
    return ret;
}

// レイヤのパス名を返す
ttstr PSD::pathname(psd::LayerInfo &lay) {
    ttstr name = "";
    psd::LayerInfo *p = lay.parent;
    while(p) {
        name = path_layname(*p) + "/" + name;
        p = p->parent;
    }
    return ttstr("root/") + name;
}

// ストレージ処理用データの初期化
void PSD::startStorage() {
    if(!storageStarted) {
        storageStarted = true;
        // レイヤ検索用の情報を生成
        int count = (int)layerList.size();
        for(int i = count - 1; i >= 0; i--) {
            psd::LayerInfo &lay = layerList[i];
            if(lay.layerType == psd::LAYER_TYPE_NORMAL) {
                pathMap[pathname(lay)][path_layname(lay)] = i;
                layerIdIdxMap[lay.layerId] = i;
            }
        }
    }
}

bool checkAllNum(const tjs_char *p) {
    while(*p != '\0') {
        if(!(*p >= '0' && *p <= '9')) {
            return false;
        }
        p++;
    }
    return true;
}

/*
 * 指定した名前のレイヤの存在チェック
 * @param name パスを含むレイヤ名
 * @param layerIdxRet レイヤインデックス番号を返す
 */
bool PSD::CheckExistentStorage(const ttstr &filename, int *layerIdxRet) {
    startStorage();

    // ルート部を取得
    const tjs_char *p = filename.c_str();

    // id指定の場合
    if(TJS_strncmp(p, TJS_W("id/"), 3) == 0) {

        p += 3;

        // 拡張子を除去して判定
        const tjs_char *q;
        if(!(q = TJS_strchr(p, '/')) &&
           ((q = TJS_strchr(p, '.')) && (TJS_strcmp(q, BMPEXT) == 0))) {
            ttstr name = ttstr(p, q - p);
            q = name.c_str();
            if(checkAllNum(q)) { // 文字混入禁止
                int id = TJS_atoi(q);
                auto n = layerIdIdxMap.find(id);
                if(n != layerIdIdxMap.end()) {
                    if(layerIdxRet)
                        *layerIdxRet = n->second;
                    return true;
                }
            }
        }

    } else {

        // パスを分離
        ttstr pname, fname;
        // 最後の/を探す
        const tjs_char *q;
        if((q = TJS_strchr(p, '/'))) {
            pname = ttstr(p, q - p + 1);
            fname = ttstr(q + 1);
        } else {
            return false;
        }

        // 拡張子分離
        ttstr basename;
        p = fname.c_str();
        // 最初の . を探す
        if((q = TJS_strchr(p, '.')) && (TJS_strcmp(q, BMPEXT) == 0)) {
            basename = ttstr(p, q - p);
        } else {
            return false;
        }

        // 名前を探す
        auto n = pathMap.find(pname);
        if(n != pathMap.end()) {
            const NameIdxMap &names = n->second;
            auto m = names.find(basename);
            if(m != names.end()) {
                if(layerIdxRet)
                    *layerIdxRet = m->second;
                return true;
            }
        }
    }

    return false;
}

/*
 * 指定したパスにあるファイル名一覧の取得
 * @param pathname パス名
 * @param lister リスト取得用インターフェース
 */
void PSD::GetListAt(const ttstr &pathname, iTVPStorageLister *lister) {
    startStorage();

    // ID一覧から名前を生成
    if(pathname == "id/") {
        auto it = layerIdIdxMap.begin();
        while(it != layerIdIdxMap.end()) {
            ttstr name = ttstr(it->first);
            lister->Add(name + BMPEXT);
            it++;
        }
        return;
    }

    // パス登録情報から名前を生成
    auto n = pathMap.find(pathname);
    if(n != pathMap.end()) {
        const NameIdxMap &names = n->second;
        auto it = names.begin();
        while(it != names.end()) {
            ttstr name = it->first;
            lister->Add(name + BMPEXT);
            it++;
        }
    }
}

/*
 * 指定した名前のレイヤの画像ファイルをストリームで返す
 * @param name パスを含むレイヤ名
 * @return ファイルストリーム
 */
IStream *PSD::openLayerImage(const ttstr &name) {
    static int n = 0;

    int layerIdx;
    if(CheckExistentStorage(name, &layerIdx)) {
        if(layerIdx < (int)layerList.size()) {
            psd::LayerInfo &lay = layerList[layerIdx];

            if(lay.layerType != psd::LAYER_TYPE_NORMAL || lay.width <= 0 ||
               lay.height <= 0) {
                return nullptr;
            }
            int width = lay.width;
            int height = lay.height;
            int pitch = width * 4;

            int hsize = sizeof(TVP_WIN_BITMAPFILEHEADER);
            int isize = hsize + sizeof(TVP_WIN_BITMAPINFOHEADER);
            int size = isize + pitch * height;

            // グローバルヒープにBMP画像を作成してストリームとして返す
            auto *handle = reinterpret_cast<uint8_t *>(malloc(size));
            if(handle) {

                TVP_WIN_BITMAPFILEHEADER bfh{};
                bfh.bfType = 'B' + ('M' << 8);
                bfh.bfSize = size;
                bfh.bfReserved1 = 0;
                bfh.bfReserved2 = 0;
                bfh.bfOffBits = isize;
                memcpy(handle, &bfh, sizeof bfh);

                TVP_WIN_BITMAPINFOHEADER bih{};
                bih.biSize = sizeof(bih);
                bih.biWidth = width;
                bih.biHeight = height;
                bih.biPlanes = 1;
                bih.biBitCount = 32;
                bih.biCompression = BI_RGB;
                bih.biSizeImage = 0;
                bih.biXPelsPerMeter = 0;
                bih.biYPelsPerMeter = 0;
                bih.biClrUsed = 0;
                bih.biClrImportant = 0;
                memcpy(handle + hsize, &bih, sizeof bih);
                getLayerImage(lay, handle + isize + pitch * (height - 1),
                              psd::BGRA_LE, -pitch,
                              psd::IMAGE_MODE_MASKEDIMAGE);

                return pStream;
            }
            free(handle);
        }
    }
    return nullptr;
}

NCB_REGISTER_CLASS(PSD) {

    Factory(&ClassT::factory);

    Variant(TJS_W("color_mode_bitmap"), (tjs_int64)psd::COLOR_MODE_BITMAP);
    Variant(TJS_W("color_mode_grayscale"),
            (tjs_int64)psd::COLOR_MODE_GRAYSCALE);
    Variant(TJS_W("color_mode_indexed"), (tjs_int64)psd::COLOR_MODE_INDEXED);
    Variant(TJS_W("color_mode_rgb"), (tjs_int64)psd::COLOR_MODE_RGB);
    Variant(TJS_W("color_mode_cmyk"), (tjs_int64)psd::COLOR_MODE_CMYK);
    Variant(TJS_W("color_mode_multichannel"),
            (tjs_int64)psd::COLOR_MODE_MULTICHANNEL);
    Variant(TJS_W("color_mode_duotone"), (tjs_int64)psd::COLOR_MODE_DUOTONE);
    Variant(TJS_W("color_mode_lab"), (tjs_int64)psd::COLOR_MODE_LAB);

    Variant(TJS_W("blend_mode_normal"), (tjs_int64)psd::BLEND_MODE_NORMAL);
    Variant(TJS_W("blend_mode_dissolve"), (tjs_int64)psd::BLEND_MODE_DISSOLVE);
    Variant(TJS_W("blend_mode_darken"), (tjs_int64)psd::BLEND_MODE_DARKEN);
    Variant(TJS_W("blend_mode_multiply"), (tjs_int64)psd::BLEND_MODE_MULTIPLY);
    Variant(TJS_W("blend_mode_color_burn"),
            (tjs_int64)psd::BLEND_MODE_COLOR_BURN);
    Variant(TJS_W("blend_mode_linear_burn"),
            (tjs_int64)psd::BLEND_MODE_LINEAR_BURN);
    Variant(TJS_W("blend_mode_lighten"), (tjs_int64)psd::BLEND_MODE_LIGHTEN);
    Variant(TJS_W("blend_mode_screen"), (tjs_int64)psd::BLEND_MODE_SCREEN);
    Variant(TJS_W("blend_mode_color_dodge"),
            (tjs_int64)psd::BLEND_MODE_COLOR_DODGE);
    Variant(TJS_W("blend_mode_linear_dodge"),
            (tjs_int64)psd::BLEND_MODE_LINEAR_DODGE);
    Variant(TJS_W("blend_mode_overlay"), (tjs_int64)psd::BLEND_MODE_OVERLAY);
    Variant(TJS_W("blend_mode_soft_light"),
            (tjs_int64)psd::BLEND_MODE_SOFT_LIGHT);
    Variant(TJS_W("blend_mode_hard_light"),
            (tjs_int64)psd::BLEND_MODE_HARD_LIGHT);
    Variant(TJS_W("blend_mode_vivid_light"),
            (tjs_int64)psd::BLEND_MODE_VIVID_LIGHT);
    Variant(TJS_W("blend_mode_linear_light"),
            (tjs_int64)psd::BLEND_MODE_LINEAR_LIGHT);
    Variant(TJS_W("blend_mode_pin_light"),
            (tjs_int64)psd::BLEND_MODE_PIN_LIGHT);
    Variant(TJS_W("blend_mode_hard_mix"), (tjs_int64)psd::BLEND_MODE_HARD_MIX);
    Variant(TJS_W("blend_mode_difference"),
            (tjs_int64)psd::BLEND_MODE_DIFFERENCE);
    Variant(TJS_W("blend_mode_exclusion"),
            (tjs_int64)psd::BLEND_MODE_EXCLUSION);
    Variant(TJS_W("blend_mode_hue"), (tjs_int64)psd::BLEND_MODE_HUE);
    Variant(TJS_W("blend_mode_saturation"),
            (tjs_int64)psd::BLEND_MODE_SATURATION);
    Variant(TJS_W("blend_mode_color"), (tjs_int64)psd::BLEND_MODE_COLOR);
    Variant(TJS_W("blend_mode_luminosity"),
            (tjs_int64)psd::BLEND_MODE_LUMINOSITY);
    Variant(TJS_W("blend_mode_pass_through"),
            (tjs_int64)psd::BLEND_MODE_PASS_THROUGH);

    // NOTE libpsd 非互換モード
    Variant(TJS_W("blend_mode_darker_color"),
            (tjs_int64)psd::BLEND_MODE_DARKER_COLOR);
    Variant(TJS_W("blend_mode_lighter_color"),
            (tjs_int64)psd::BLEND_MODE_LIGHTER_COLOR);
    Variant(TJS_W("blend_mode_subtract"), (tjs_int64)psd::BLEND_MODE_SUBTRACT);
    Variant(TJS_W("blend_mode_divide"), (tjs_int64)psd::BLEND_MODE_DIVIDE);

    // NOTE この定数はlibpsd互換ではありません(folderまでは互換)
    Variant(TJS_W("layer_type_normal"), (tjs_int64)psd::LAYER_TYPE_NORMAL);
    Variant(TJS_W("layer_type_hidden"), (tjs_int64)psd::LAYER_TYPE_HIDDEN);
    Variant(TJS_W("layer_type_folder"), (tjs_int64)psd::LAYER_TYPE_FOLDER);
    Variant(TJS_W("layer_type_adjust"), (tjs_int64)psd::LAYER_TYPE_ADJUST);
    Variant(TJS_W("layer_type_fill"), (tjs_int64)psd::LAYER_TYPE_FILL);

    NCB_METHOD(load);

#define INTPROP(name) Property(TJS_W(#name), &Class::get_##name, 0)

    INTPROP(width);
    INTPROP(height);
    INTPROP(channels);
    INTPROP(depth);
    INTPROP(color_mode);
    INTPROP(layer_count);

    NCB_METHOD(getLayerType);
    NCB_METHOD(getLayerName);
    NCB_METHOD(getLayerInfo);
    NCB_METHOD(getLayerData);
    NCB_METHOD(getLayerDataRaw);
    NCB_METHOD(getLayerDataMask);

    NCB_METHOD(getSlices);
    NCB_METHOD(getGuides);
    NCB_METHOD(getBlend);
    NCB_METHOD(getLayerComp);

    NCB_METHOD(clearStorageCache);
}