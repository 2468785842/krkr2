//
// Created by LiDon on 2025/9/15.
// Aligned to libkrkr2.so D3DEmotePlayer architecture:
// EmotePlayer is a thin shell delegating all animation logic to an owned
// Player. Binary: D3DEmotePlayerNativeInstance(24b) → EmoteObject(40b) →
// Player(1496b)
//

#include <algorithm>
#include <stdexcept>

#include "EmotePlayer.h"
#include "RuntimeSupport.h"
#include "ncbind.hpp"
#include "psbfile/PSBFile.h"

#include "log/TVPLog.h"

namespace {

    enum class EmotePlayMode {
        None,
        MotionKey, // _currentfile != nullptr
        SingleCache, // cache.size()==1 && isMotion
        MultiCache // composite cache
    };

    ttstr readMetadataBaseField(const tTJSVariant &module, const ttstr &field) {
        if(module.Type() != tvtObject) {
            return {};
        }
        iTJSDispatch2 *root = module.AsObjectNoAddRef();
        if(!root) {
            return {};
        }

        tTJSVariant metadata;
        if(TJS_FAILED(
               root->PropGet(0, TJS_W("metadata"), nullptr, &metadata, root))) {
            return {};
        }
        if(metadata.Type() != tvtObject) {
            return {};
        }
        iTJSDispatch2 *metaObj = metadata.AsObjectNoAddRef();
        if(!metaObj) {
            return {};
        }

        tTJSVariant base;
        if(TJS_FAILED(
               metaObj->PropGet(0, TJS_W("base"), nullptr, &base, metaObj))) {
            return {};
        }
        if(base.Type() != tvtObject) {
            return {};
        }
        iTJSDispatch2 *baseObj = base.AsObjectNoAddRef();
        if(!baseObj) {
            return {};
        }

        tTJSVariant value;
        if(TJS_FAILED(
               baseObj->PropGet(0, field.c_str(), nullptr, &value, baseObj))) {
            return {};
        }
        if(value.Type() == tvtVoid) {
            return {};
        }
        return ttstr(value);
    }

    bool isMotionModule(const tTJSVariant &module) {
        // 参考 sdl3/（不编译）: emotefile::isMotion — false=emote, true=motion.
        // libkrkr2.so PSB root type: 0=motion, 1=emote.
        const auto snapshot = motion::detail::lookupModuleSnapshot(module);
        if(!snapshot || !snapshot->root) {
            return false;
        }
        const auto typeVal = (*snapshot->root)["type"];
        if(const auto num =
               std::dynamic_pointer_cast<const PSB::PSBNumber>(typeVal)) {
            return num->getValue<int>() == 0;
        }
        return false;
    }

    tTJSVariant boundModuleVariant(const motion::EmotePlayer &self) {
        const tTJSVariant module = self.getModule();
        if(module.Type() == tvtObject) {
            return module;
        }
        return self.getPlayer().getProject();
    }

} // namespace

namespace motion {

    EmotePlayer::EmotePlayer(ResourceManager rm) : _player(std::move(rm)) {}

    EmotePlayer::~EmotePlayer() = default;

    // --- Properties ---

    void EmotePlayer::setUseD3D(bool v) {
        _useD3D = v;
        _player.setUseD3D(v);
    }

    void EmotePlayer::setVisible(bool v) {
        _visible = v;
        _player.setVisible(v);
    }

    void EmotePlayer::setMeshDivisionRatio(double v) {
        _meshDivisionRatio = v;
        _player.setEmoteMeshDivisionRatio(v);
    }

    void EmotePlayer::setHairScale(double v) {
        _hairScale = v;
        _player.setHairScale(v);
        _modified = true;
    }

    void EmotePlayer::setPartsScale(double v) {
        _partsScale = v;
        _player.setPartsScale(v);
        _modified = true;
    }

    void EmotePlayer::setBustScale(double v) {
        _bustScale = v;
        _player.setBustScale(v);
        _modified = true;
    }

    bool EmotePlayer::getAnimating() const { return _player.getAllplaying(); }

    void EmotePlayer::setModule(tTJSVariant v) {
        _module = v;
        // Bridge loaded PSB snapshot into Player's animation pipeline.
        // Aligned to libkrkr2.so EmoteObject_init (sub_67DBAC):
        // After loading PSBs, the EmoteObject initializes its internal Player
        // with the loaded motion data.
        auto snapshot = detail::lookupModuleSnapshot(_module);
        if(snapshot) {
            _player.loadFromSnapshot(snapshot);
        }
    }

    tTJSVariant EmotePlayer::getModule() const { return _module; }

    void EmotePlayer::setMotionKey(ttstr v) {
        // 参考 sdl3/emoteplayerclass.cpp（不编译）: set_motionKey()
        _storageKey = v;
        _player.bindMotionModuleKey(v);
        const auto loaded = _player.getProject();
        if(loaded.Type() == tvtObject) {
            _module = loaded;
        }
        _modified = true;
    }

    void EmotePlayer::setMotion(ttstr v) {
        // motion 属性仅存 clip 名；play() 启动播放
        _clipLabel = v;
        _modified = true;
    }

    ttstr EmotePlayer::getMotion() const {
        if(!_clipLabel.IsEmpty()) {
            return _clipLabel;
        }
        return _player.getMotion();
    }

    // --- Methods ---

    // Aligned to libkrkr2.so sub_52FD84: create() is actually "destroy/reset"
    void EmotePlayer::create() {
        _module.Clear();
        _player.loadFromSnapshot(nullptr);
        _modified = true;
    }

    void EmotePlayer::load(tTJSVariant data) {
        _module = data;
        auto snapshot = detail::lookupModuleSnapshot(_module);
        if(snapshot) {
            _player.loadFromSnapshot(snapshot);
        }
        _modified = true;
    }

    tTJSVariant EmotePlayer::clone() {
        typedef ncbInstanceAdaptor<EmotePlayer> AdaptorT;

        auto *copy = new EmotePlayer(ResourceManager{});
        // Copy EmotePlayer-specific state
        copy->_module = _module;
        copy->_storageKey = _storageKey;
        copy->_clipLabel = _clipLabel;
        copy->_useD3D = _useD3D;
        copy->_smoothing = _smoothing;
        copy->_meshDivisionRatio = _meshDivisionRatio;
        copy->_queuing = _queuing;
        copy->_hairScale = _hairScale;
        copy->_partsScale = _partsScale;
        copy->_bustScale = _bustScale;
        copy->_bodyScale = _bodyScale;
        copy->_progress = _progress;
        copy->_modified = _modified;
        copy->_drawVisible = _drawVisible;
        copy->_drawOpacity = _drawOpacity;
        copy->_opengl = _opengl;
        copy->_visible = _visible;
        copy->_playCallback = _playCallback;
        copy->_isSelfClear = _isSelfClear;
        copy->_baseScale = _baseScale;
        copy->_userScale = _userScale;
        copy->_rot = _rot;
        copy->_coordX = _coordX;
        copy->_coordY = _coordY;
        copy->_mirrorBase = _mirrorBase;
        copy->_mirrorRequested = _mirrorRequested;
        copy->_mirrorChanged = _mirrorChanged;
        copy->_color = _color;

        // Load the same snapshot into the cloned Player
        auto snapshot = detail::lookupModuleSnapshot(_module);
        if(snapshot) {
            copy->_player.loadFromSnapshot(snapshot);
        }
        copy->_player.setUseD3D(copy->_useD3D);
        copy->_player.setHairScale(copy->_hairScale);
        copy->_player.setPartsScale(copy->_partsScale);
        copy->_player.setBustScale(copy->_bustScale);
        copy->_player.setEmoteMeshDivisionRatio(copy->_meshDivisionRatio);
        copy->_player.setVisible(copy->_visible);

        tTJSVariant result;
        if(iTJSDispatch2 *adaptor = AdaptorT::CreateAdaptor(copy)) {
            result = tTJSVariant(adaptor, adaptor);
            adaptor->Release();
        } else {
            delete copy;
        }
        return result;
    }

    void EmotePlayer::show() {
        _visible = true;
        _player.setVisible(true);
    }

    void EmotePlayer::hide() {
        _visible = false;
        _player.setVisible(false);
    }

    void EmotePlayer::assignState() {
        _player.setVisible(_visible && _drawVisible);
        _player.setUseD3D(_useD3D);
        _player.setHairScale(_hairScale);
        _player.setPartsScale(_partsScale);
        _player.setBustScale(_bustScale);
        _player.setEmoteMeshDivisionRatio(_meshDivisionRatio);
        _player.setEmoteCoord(_coordX, _coordY, 0.0, 0.0);
        const double finalScale =
            static_cast<double>(_baseScale) * static_cast<double>(_userScale);
        _player.setEmoteScale(finalScale, 0.0, 0.0);
        _player.setRotate(_rot, 0.0, 0.0);
        _modified = false;
    }

    void EmotePlayer::initPhysics(tTJSVariant rule) {
        if(rule.Type() != tvtVoid) {
            _player.setMetadata(rule);
        }
        _player.initPhysics(rule);
        _modified = true;
    }

    tTJSVariant EmotePlayer::serialize() { return _player.serialize(); }

    void EmotePlayer::unserialize(tTJSVariant data) {
        _player.unserialize(data);
        _modified = true;
    }

    // Aligned to libkrkr2.so sub_5302E4: delegates to Player's rotAnimator
    void EmotePlayer::setRot(double rot, double transition, double ease) {
        _rot = rot;
        _player.setRotate(rot, transition, ease);
        _modified = true;
    }

    void EmotePlayer::setRotate(double rot, double transition, double ease) {
        setRot(rot, transition, ease);
    }

    tjs_error EmotePlayer::setRotCompat(tTJSVariant *, tjs_int numparams,
                                        tTJSVariant **param,
                                        iTJSDispatch2 *objthis) {
        auto *self =
            ncbInstanceAdaptor<EmotePlayer>::GetNativeInstance(objthis, true);
        if(!self) {
            return TJS_E_INVALIDOBJECT;
        }
        if(numparams < 1 || !param[0]) {
            return TJS_E_INVALIDPARAM;
        }

        const double transition =
            (numparams >= 2 && param[1]) ? param[1]->AsReal() : 0.0;
        const double ease =
            (numparams >= 3 && param[2]) ? param[2]->AsReal() : 0.0;
        self->setRot(param[0]->AsReal(), transition, ease);
        return TJS_S_OK;
    }

    // Aligned to libkrkr2.so sub_53030C: binary returns hardcoded 0.0
    double EmotePlayer::getRot() { return 0.0; }

    // Aligned to libkrkr2.so sub_5301EC: delegates to Player's coordAnimator
    void EmotePlayer::setCoord(double x, double y, double transition,
                               double ease) {
        _coordX = x;
        _coordY = y;
        _player.setEmoteCoord(x, y, transition, ease);
        _modified = true;
    }

    tjs_error EmotePlayer::setCoordCompat(tTJSVariant *, tjs_int numparams,
                                          tTJSVariant **param,
                                          iTJSDispatch2 *objthis) {
        auto *self =
            ncbInstanceAdaptor<EmotePlayer>::GetNativeInstance(objthis, true);
        if(!self) {
            return TJS_E_INVALIDOBJECT;
        }
        if(numparams < 2 || !param[0] || !param[1]) {
            return TJS_E_INVALIDPARAM;
        }

        const double transition =
            (numparams >= 3 && param[2]) ? param[2]->AsReal() : 0.0;
        const double ease =
            (numparams >= 4 && param[3]) ? param[3]->AsReal() : 0.0;
        self->setCoord(param[0]->AsReal(), param[1]->AsReal(), transition,
                       ease);
        return TJS_S_OK;
    }

    // Aligned to libkrkr2.so sub_530260: finalScale = baseScale * userScale
    void EmotePlayer::setScale(double s, double transition, double ease) {
        _userScale = static_cast<float>(s);
        const double finalScale =
            static_cast<double>(_baseScale) * static_cast<double>(_userScale);
        _player.setEmoteScale(finalScale, transition, ease);
        _modified = true;
    }

    tjs_error EmotePlayer::setScaleCompat(tTJSVariant *, tjs_int numparams,
                                          tTJSVariant **param,
                                          iTJSDispatch2 *objthis) {
        auto *self =
            ncbInstanceAdaptor<EmotePlayer>::GetNativeInstance(objthis, true);
        if(!self) {
            return TJS_E_INVALIDOBJECT;
        }
        if(numparams < 1 || !param[0]) {
            return TJS_E_INVALIDPARAM;
        }

        const double transition =
            (numparams >= 2 && param[1]) ? param[1]->AsReal() : 0.0;
        const double ease =
            (numparams >= 3 && param[2]) ? param[2]->AsReal() : 0.0;
        self->setScale(param[0]->AsReal(), transition, ease);
        return TJS_S_OK;
    }

    // Aligned to libkrkr2.so sub_5302DC: binary returns hardcoded 1.0
    double EmotePlayer::getScale() { return 1.0; }

    void EmotePlayer::setMirror(bool mirror) {
        // Aligned to libkrkr2.so sub_671DB0:
        // wrapper stores requested mirror, derives a root-flip delta against a
        // baseline bit, forwards that effective flip to Player_setRootFlipX,
        // then triggers the large controller reset path.
        _mirrorRequested = mirror;
        _mirrorChanged = (_mirrorRequested != _mirrorBase);
        _player.setMirror(_mirrorChanged);
        _modified = true;
    }

    void EmotePlayer::setColor(tjs_int color, double transition, double ease) {
        _color = color;
        _player.setEmoteColor(static_cast<tjs_uint32>(color), transition, ease);
        _modified = true;
    }

    tjs_error EmotePlayer::setColorCompat(tTJSVariant *, tjs_int numparams,
                                          tTJSVariant **param,
                                          iTJSDispatch2 *objthis) {
        auto *self =
            ncbInstanceAdaptor<EmotePlayer>::GetNativeInstance(objthis, true);
        if(!self) {
            return TJS_E_INVALIDOBJECT;
        }
        if(numparams < 1 || !param[0]) {
            return TJS_E_INVALIDPARAM;
        }

        const double transition =
            (numparams >= 2 && param[1]) ? param[1]->AsReal() : 0.0;
        const double ease =
            (numparams >= 3 && param[2]) ? param[2]->AsReal() : 0.0;
        self->setColor(param[0]->AsInteger(), transition, ease);
        return TJS_S_OK;
    }
    // Aligned to libkrkr2.so sub_530320: binary returns hardcoded 0
    tjs_int EmotePlayer::getColor() { return 0; }

    // --- Variable system: delegates to Player ---
    // Aligned to libkrkr2.so sub_5305C8 → sub_671228:
    // wrapper forwards label/value/transition/ease into Player_setVariable.
    void EmotePlayer::setVariable(ttstr label, double value, double transition,
                                  double ease) {
        _player.setVariable(label, value, transition, ease);
        _modified = true;
    }

    tjs_error EmotePlayer::setVariableCompat(tTJSVariant *, tjs_int numparams,
                                             tTJSVariant **param,
                                             iTJSDispatch2 *objthis) {
        auto *self =
            ncbInstanceAdaptor<EmotePlayer>::GetNativeInstance(objthis, true);
        if(!self) {
            return TJS_E_INVALIDOBJECT;
        }
        if(numparams < 2 || !param[0] || !param[1]) {
            return TJS_E_INVALIDPARAM;
        }

        const double transition =
            (numparams >= 3 && param[2]) ? param[2]->AsReal() : 0.0;
        const double ease =
            (numparams >= 4 && param[3]) ? param[3]->AsReal() : 0.0;
        self->setVariable(ttstr(*param[0]), param[1]->AsReal(), transition,
                          ease);
        return TJS_S_OK;
    }

    double EmotePlayer::getVariable(ttstr label) {
        return _player.getVariable(label);
    }

    tTJSVariant EmotePlayer::getVariableFrameList(ttstr label) {
        return _player.getVariableFrameList(label);
    }

    tjs_int EmotePlayer::countVariables() { return _player.countVariables(); }

    ttstr EmotePlayer::getVariableLabelAt(tjs_int idx) {
        return _player.getVariableLabelAt(idx);
    }

    tjs_int EmotePlayer::countVariableFrameAt(tjs_int idx) {
        return _player.countVariableFrameAt(idx);
    }

    ttstr EmotePlayer::getVariableFrameLabelAt(tjs_int idx, tjs_int frameIdx) {
        return _player.getVariableFrameLabelAt(idx, frameIdx);
    }

    double EmotePlayer::getVariableFrameValueAt(tjs_int idx, tjs_int frameIdx) {
        return _player.getVariableFrameValueAt(idx, frameIdx);
    }

    // --- Wind/Force ---
    void EmotePlayer::startWind(double minAngle, double maxAngle,
                                double amplitude, double freqX, double freqY) {
        _player.startWind(minAngle, maxAngle, amplitude, freqX, freqY);
        _modified = true;
    }

    tjs_error EmotePlayer::startWindCompat(tTJSVariant *, tjs_int numparams,
                                           tTJSVariant **param,
                                           iTJSDispatch2 *objthis) {
        auto *self =
            ncbInstanceAdaptor<EmotePlayer>::GetNativeInstance(objthis, true);
        if(!self) {
            return TJS_E_INVALIDOBJECT;
        }
        if(numparams < 5 || !param[0] || !param[1] || !param[2] || !param[3] ||
           !param[4]) {
            return TJS_E_INVALIDPARAM;
        }

        self->startWind(param[0]->AsReal(), param[1]->AsReal(),
                        param[2]->AsReal(), param[3]->AsReal(),
                        param[4]->AsReal());
        return TJS_S_OK;
    }

    void EmotePlayer::stopWind() {
        _player.stopWind();
        _modified = true;
    }

    tjs_error EmotePlayer::stopWindCompat(tTJSVariant *, tjs_int,
                                          tTJSVariant **,
                                          iTJSDispatch2 *objthis) {
        auto *self =
            ncbInstanceAdaptor<EmotePlayer>::GetNativeInstance(objthis, true);
        if(!self) {
            return TJS_E_INVALIDOBJECT;
        }
        self->stopWind();
        return TJS_S_OK;
    }

    // --- Timeline methods: delegate to Player ---

    tjs_int EmotePlayer::countMainTimelines() {
        return _player.countMainTimelines();
    }

    ttstr EmotePlayer::getMainTimelineLabelAt(tjs_int idx) {
        return _player.getMainTimelineLabelAt(idx);
    }

    tTJSVariant EmotePlayer::getMainTimelineLabelList() {
        return _player.getMainTimelineLabelList();
    }

    tjs_int EmotePlayer::countDiffTimelines() {
        return _player.countDiffTimelines();
    }

    ttstr EmotePlayer::getDiffTimelineLabelAt(tjs_int idx) {
        return _player.getDiffTimelineLabelAt(idx);
    }

    tTJSVariant EmotePlayer::getDiffTimelineLabelList() {
        return _player.getDiffTimelineLabelList();
    }

    tjs_int EmotePlayer::countPlayingTimelines() {
        return _player.countPlayingTimelines();
    }

    ttstr EmotePlayer::getPlayingTimelineLabelAt(tjs_int idx) {
        return _player.getPlayingTimelineLabelAt(idx);
    }

    tjs_int EmotePlayer::getPlayingTimelineFlagsAt(tjs_int idx) {
        return _player.getPlayingTimelineFlagsAt(idx);
    }

    bool EmotePlayer::isLoopTimeline(ttstr label) {
        return _player.getLoopTimeline(label);
    }

    bool EmotePlayer::getLoopTimeline(ttstr label) {
        return isLoopTimeline(label);
    }

    tjs_int EmotePlayer::getTimelineTotalFrameCount(ttstr label) {
        return _player.getTimelineTotalFrameCount(label);
    }

    void EmotePlayer::playTimeline(ttstr label, tjs_int flags) {
        _player.playTimeline(label, flags);
        _modified = true;
    }

    bool EmotePlayer::isTimelinePlaying(ttstr label) {
        return _player.getTimelinePlaying(label);
    }

    bool EmotePlayer::getTimelinePlaying(ttstr label) {
        return isTimelinePlaying(label);
    }

    void EmotePlayer::stopTimeline(ttstr label) { _player.stopTimeline(label); }

    void EmotePlayer::setTimelineBlendRatio(ttstr label, double ratio) {
        _player.setTimelineBlendRatio(label, ratio);
    }

    double EmotePlayer::getTimelineBlendRatio(ttstr label) {
        return _player.getTimelineBlendRatio(label);
    }

    void EmotePlayer::fadeInTimeline(ttstr label, double duration,
                                     tjs_int flags) {
        // 参考 sdl3：零时长 fadeIn 等价于 playTimeline
        if(duration <= 0.0) {
            G_PluginLog.debug(
                "EmotePlayer::fadeInTimeline({}, {}): zero-duration "
                "→ playTimeline",
                label.AsStdString(), duration);
            playTimeline(label, flags);
            return;
        }
        _player.fadeInTimeline(label, duration, flags);
        _modified = true;
    }

    void EmotePlayer::fadeOutTimeline(ttstr label, double duration,
                                      tjs_int flags) {
        // 参考 sdl3：零时长 fadeOut 等价于 stopTimeline
        if(duration <= 0.0) {
            G_PluginLog.debug(
                "EmotePlayer::fadeOutTimeline({}, {}): zero-duration "
                "→ stopTimeline",
                label.AsStdString(), duration);
            stopTimeline(label);
            return;
        }
        _player.fadeOutTimeline(label, duration, flags);
        _modified = true;
    }

    tTJSVariant EmotePlayer::getPlayingTimelineInfoList() {
        return _player.getPlayingTimelineInfoList();
    }

    void EmotePlayer::setTimeline(ttstr label, bool loop) {
        // Player doesn't have an exact equivalent; use playTimeline + loop flag
        _player.playTimeline(label, 0);
    }

    bool EmotePlayer::play(ttstr label, tjs_int flags) {
        // 参考 sdl3/emoteplayerclass.cpp（不编译）: EmotePlayer::play()
        // 三种启动模式
        //   1) motionKey bound module
        //   2) single cached motion PSB (chara+motion path)
        //   3) multi-cache composite (addEmoteFile — not fully ported)
        if(label.IsEmpty()) {
            label = _clipLabel;
        }

        EmotePlayMode mode = EmotePlayMode::None;
        ttstr clipLookupLabel;
        bool selfClear = true;
        auto &rm = _player.getResourceManagerNative();
        const tTJSVariant module = boundModuleVariant(*this);

        if(_player.hasActiveMotion() || module.Type() == tvtObject) {
            if(!_player.hasActiveMotion() && module.Type() == tvtObject) {
                if(const auto snapshot = detail::lookupModuleSnapshot(module)) {
                    _player.loadFromSnapshot(snapshot);
                }
            }
            if(_player.hasActiveMotion()) {
                mode = EmotePlayMode::MotionKey;
                selfClear = true;
                const ttstr metaMotion =
                    readMetadataBaseField(module, TJS_W("motion"));
                clipLookupLabel = !metaMotion.IsEmpty() ? metaMotion : label;
                G_PluginLog.debug(
                    "EmotePlayer::play mode=MotionKey storageKey={} "
                    "clipLookup={} playLabel={}",
                    _storageKey.AsStdString(), clipLookupLabel.AsStdString(),
                    label.AsStdString());
            }
        }

        if(mode == EmotePlayMode::None) {
            const auto cached = rm.uniqueCachedModules();
            if(cached.size() == 1 && isMotionModule(cached.front().module)) {
                mode = EmotePlayMode::SingleCache;
                selfClear = true;
                const auto &entry = cached.front();
                if(!_player.hasActiveMotion()) {
                    _player.bindMotionModuleKey(ttstr(entry.key.c_str()));
                    _storageKey = ttstr(entry.key.c_str());
                    _module = entry.module;
                }
                clipLookupLabel = label;
                G_PluginLog.debug(
                    "EmotePlayer::play mode=SingleCache key={} chara={} "
                    "clipLookup={}",
                    entry.key, _player.getChara().AsStdString(),
                    clipLookupLabel.AsStdString());
            } else if(!cached.empty()) {
                // 参考 sdl3：multi-cache 分支 — 选主 PSB 后交叉链接其余缓存项
                mode = EmotePlayMode::MultiCache;
                selfClear = false;
                std::shared_ptr<detail::MotionSnapshot> primarySnapshot;
                for(const auto &entry : cached) {
                    const ttstr metaChara =
                        readMetadataBaseField(entry.module, TJS_W("chara"));
                    const ttstr metaMotion =
                        readMetadataBaseField(entry.module, TJS_W("motion"));
                    if(metaChara.IsEmpty() || metaMotion.IsEmpty()) {
                        continue;
                    }
                    const auto snapshot =
                        detail::lookupModuleSnapshot(entry.module);
                    if(!snapshot) {
                        continue;
                    }
                    if(!primarySnapshot) {
                        primarySnapshot = snapshot;
                        _player.bindMotionModuleKey(ttstr(entry.key.c_str()));
                        _storageKey = ttstr(entry.key.c_str());
                        _module = entry.module;
                        clipLookupLabel = metaMotion;
                        G_PluginLog.debug(
                            "EmotePlayer::play mode=MultiCache primary key={} "
                            "chara={} motion={}",
                            entry.key, metaChara.AsStdString(),
                            metaMotion.AsStdString());
                    }
                }
                if(primarySnapshot) {
                    for(const auto &entry : cached) {
                        const auto snapshot =
                            detail::lookupModuleSnapshot(entry.module);
                        if(snapshot &&
                           snapshot.get() != primarySnapshot.get()) {
                            _player.addEmoteFile(snapshot);
                        }
                    }
                    G_PluginLog.debug(
                        "EmotePlayer::play mode=MultiCache: linked {} attached "
                        "snapshot(s)",
                        primarySnapshot->attachedSnapshots.size());
                }
            }
        }

        if(clipLookupLabel.IsEmpty()) {
            if(label.IsEmpty()) {
                G_PluginLog.error(
                    "EmotePlayer::play(): no module/motion resolved in any "
                    "EmotePlayer play mode");
                throw std::runtime_error(
                    "motionplayer: EmotePlayer.play() could not resolve motion "
                    "module");
            }
            clipLookupLabel = label;
            G_PluginLog.warn(
                "EmotePlayer::play: no play mode matched; fallback to "
                "playLabel={}",
                label.AsStdString());
        }

        _clipLabel = label;
        _progress = 0.0;
        _isSelfClear = selfClear;

        // 参考 sdl3：play 开始时重置 tick / playing 状态
        _player.setTickCount(0.0);
        _player.setFrameLoopTime(0.0);
        _player.setSpeed(true);

        const bool started =
            _player.playMotionLike_0x6B2284(clipLookupLabel, flags);
        if(!started && !label.IsEmpty() && clipLookupLabel != label) {
            G_PluginLog.debug(
                "EmotePlayer::play: clipLookup={} failed; retry playLabel={}",
                clipLookupLabel.AsStdString(), label.AsStdString());
            const bool retryStarted =
                _player.playMotionLike_0x6B2284(label, flags);
            _player.setAllplaying(true);
            _modified = true;
            return retryStarted;
        }

        _player.setAllplaying(true);
        _modified = true;
        return started;
    }

    void EmotePlayer::draw(tTJSVariant target) {
        _player.draw(target);
        _modified = true;
    }

    tjs_error EmotePlayer::setDrawAffineTranslateMatrixCompat(
        tTJSVariant *result, tjs_int numparams, tTJSVariant **param,
        iTJSDispatch2 *objthis) {
        auto *self =
            ncbInstanceAdaptor<EmotePlayer>::GetNativeInstance(objthis, true);
        if(!self) {
            return TJS_E_INVALIDOBJECT;
        }
        return Player::setDrawAffineTranslateMatrixCompat(
            result, numparams, param, &self->_player);
    }

    void EmotePlayer::addPlayCallback() { _playCallback = true; }

    void EmotePlayer::skip() {
        // libkrkr2.so sub_66EB8C；参考 sdl3 skipToSync（不编译）
        G_PluginLog.debug("EmotePlayer::skip(): delegating to skipToSync() "
                          "(参考 sdl3/emoteplayerclass.cpp，不编译)");
        skipToSync();
    }

    void EmotePlayer::skipToSync() {
        _player.skipToSync();
        _modified = true;
    }

    // Aligned to libkrkr2.so sub_6818B4 wrapper path:
    // pass() is parameterless (manual.tjs / AffineSourceMotion sync); time
    // advancement is progress(tickStep) called separately by the script.
    void EmotePlayer::pass() {
        _player.releaseSyncWait();
        _modified = true;
    }

    void EmotePlayer::progress(double dt) {
        if(dt < 0.0 || dt > 60000.0) {
            G_PluginLog.warn(
                "EmotePlayer::progress({}): clamping abnormal dt to 0", dt);
            dt = 0.0;
        }
        _progress += dt;
        _player.progressMsLike_0x6D2A54(dt);
        _modified = true;
    }

    tjs_error EmotePlayer::progressCompat(tTJSVariant *, tjs_int numparams,
                                          tTJSVariant **param,
                                          iTJSDispatch2 *objthis) {
        auto *self =
            ncbInstanceAdaptor<EmotePlayer>::GetNativeInstance(objthis, true);
        if(!self) {
            return TJS_E_INVALIDOBJECT;
        }

        double delta = 0.0;
        if(numparams > 0 && param[0] && param[0]->Type() != tvtVoid) {
            delta = param[0]->AsReal();
        }
        self->progress(delta);
        return TJS_S_OK;
    }

    // Aligned to libkrkr2.so sub_672D58: routes by label to bust/h/parts
    void EmotePlayer::setOuterForce(double x, double y) {
        setOuterForce(TJS_W("bust"), x, y, 0.0, 0.0);
    }

    void EmotePlayer::setOuterForce(ttstr label, double x, double y,
                                    double transition, double ease) {
        _player.setOuterForce(label, x, y, transition, ease);
        _modified = true;
    }

    tjs_error EmotePlayer::setOuterForceCompat(tTJSVariant *, tjs_int numparams,
                                               tTJSVariant **param,
                                               iTJSDispatch2 *objthis) {
        auto *self =
            ncbInstanceAdaptor<EmotePlayer>::GetNativeInstance(objthis, true);
        if(!self) {
            return TJS_E_INVALIDOBJECT;
        }
        if(numparams < 3 || !param[0] || !param[1] || !param[2]) {
            return TJS_E_INVALIDPARAM;
        }

        const ttstr label(*param[0]);
        const double transition =
            (numparams >= 4 && param[3]) ? param[3]->AsReal() : 0.0;
        const double ease =
            (numparams >= 5 && param[4]) ? param[4]->AsReal() : 0.0;
        self->setOuterForce(label, param[1]->AsReal(), param[2]->AsReal(),
                            transition, ease);
        return TJS_S_OK;
    }

    tTJSVariant EmotePlayer::getOuterForce() {
        return _player.getOuterForce(TJS_W("bust"));
    }

    bool EmotePlayer::contains(double x, double y) {
        if(!_visible) {
            return false;
        }

        // Use local coordinate state for AABB test.
        // Aligned to libkrkr2.so sub_690DF0: supports circle/rect/quad;
        // we use AABB approximation for now.
        const double scale = static_cast<double>(_baseScale * _userScale);
        const double width = _player.getActiveMotionWidth();
        const double height = _player.getActiveMotionHeight();
        if(width <= 0.0 || height <= 0.0) {
            return false;
        }

        const auto scaledWidth = width * scale;
        const auto scaledHeight = height * scale;
        return x >= _coordX && x <= (_coordX + scaledWidth) && y >= _coordY &&
            y <= (_coordY + scaledHeight);
    }

    bool EmotePlayer::contains(ttstr label, double x, double y) {
        if(!_visible || label.IsEmpty()) {
            return false;
        }
        return _player.hitTestLayer(label, x, y);
    }

    tjs_error EmotePlayer::containsCompat(tTJSVariant *result,
                                          tjs_int numparams,
                                          tTJSVariant **param,
                                          iTJSDispatch2 *objthis) {
        auto *self =
            ncbInstanceAdaptor<EmotePlayer>::GetNativeInstance(objthis, true);
        if(!self) {
            return TJS_E_INVALIDOBJECT;
        }
        if(!result) {
            return TJS_E_INVALIDPARAM;
        }

        if(numparams >= 3 && param[0] && param[1] && param[2]) {
            *result = tTJSVariant(self->contains(
                ttstr(*param[0]), param[1]->AsReal(), param[2]->AsReal()));
            return TJS_S_OK;
        }
        if(numparams >= 2 && param[0] && param[1]) {
            *result = tTJSVariant(
                self->contains(param[0]->AsReal(), param[1]->AsReal()));
            return TJS_S_OK;
        }
        return TJS_E_INVALIDPARAM;
    }

} // namespace motion
