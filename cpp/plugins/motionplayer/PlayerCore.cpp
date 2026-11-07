// PlayerCore.cpp — Constructor, setMotion, serialize, core properties
// Split from Player.cpp for maintainability.
//
#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>

#include "PlayerInternal.h"
#include "SourceCache.h"
#include "ncbind.hpp"

using namespace motion::internal;

namespace {
    std::string lowerAscii(std::string value) {
        for(char &ch : value) {
            ch =
                static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        return value;
    }

    std::uint32_t swapPackedRbLike_0x6CD710(std::uint32_t packedColor) {
        return (packedColor & 0xFF00FF00u) | ((packedColor >> 16) & 0xFFu) |
            ((packedColor & 0xFFu) << 16);
    }
} // namespace

namespace motion {

    std::unordered_map<std::string, Player::VariableAnimatorState> *
    Player::controllerAnimatorBucketLike_0x671228(int type) {
        switch(type) {
            case 4:
                return &_type4ControllerAnimators;
            case 5:
                return &_type5ControllerAnimators;
            case 6:
                return &_type6ControllerAnimators;
            case 7:
                return &_type7ControllerAnimators;
            case 8:
                return &_type8ControllerAnimators;
            default:
                return nullptr;
        }
    }

    const std::unordered_map<std::string, Player::VariableAnimatorState> *
    Player::controllerAnimatorBucketLike_0x671228(int type) const {
        switch(type) {
            case 4:
                return &_type4ControllerAnimators;
            case 5:
                return &_type5ControllerAnimators;
            case 6:
                return &_type6ControllerAnimators;
            case 7:
                return &_type7ControllerAnimators;
            case 8:
                return &_type8ControllerAnimators;
            default:
                return nullptr;
        }
    }

    Player::VariableAnimatorState *
    Player::findControllerAnimatorStateLike_0x671228(const std::string &label) {
        const auto findInBucket =
            [&label](auto &bucket) -> VariableAnimatorState * {
            if(const auto it = bucket.find(label); it != bucket.end()) {
                return &it->second;
            }
            return nullptr;
        };

        if(auto *state = findInBucket(_type4ControllerAnimators)) {
            return state;
        }
        if(auto *state = findInBucket(_type5ControllerAnimators)) {
            return state;
        }
        if(auto *state = findInBucket(_type6ControllerAnimators)) {
            return state;
        }
        if(auto *state = findInBucket(_type8ControllerAnimators)) {
            return state;
        }
        return findInBucket(_type7ControllerAnimators);
    }

    const Player::VariableAnimatorState *
    Player::findControllerAnimatorStateLike_0x671228(
        const std::string &label) const {
        const auto findInBucket =
            [&label](const auto &bucket) -> const VariableAnimatorState * {
            if(const auto it = bucket.find(label); it != bucket.end()) {
                return &it->second;
            }
            return nullptr;
        };

        if(const auto *state = findInBucket(_type4ControllerAnimators)) {
            return state;
        }
        if(const auto *state = findInBucket(_type5ControllerAnimators)) {
            return state;
        }
        if(const auto *state = findInBucket(_type6ControllerAnimators)) {
            return state;
        }
        if(const auto *state = findInBucket(_type8ControllerAnimators)) {
            return state;
        }
        return findInBucket(_type7ControllerAnimators);
    }

    void Player::eraseControllerAnimatorStateLike_0x671228(
        const std::string &label) {
        _type4ControllerAnimators.erase(label);
        _type5ControllerAnimators.erase(label);
        _type6ControllerAnimators.erase(label);
        _type7ControllerAnimators.erase(label);
        _type8ControllerAnimators.erase(label);
    }

    void Player::clearControllerAnimatorStateLike_0x671228() {
        _type4ControllerAnimators.clear();
        _type5ControllerAnimators.clear();
        _type6ControllerAnimators.clear();
        _type7ControllerAnimators.clear();
        _type8ControllerAnimators.clear();
    }

    void Player::setSelectorEnabled(bool v) {
        if(_selectorEnabled == v) {
            return;
        }
        _selectorEnabled = v;
        syncSelectorControlsLike_0x670D1C();
    }

    tjs_int Player::getColorWeight() const {
        return static_cast<tjs_int>(
            swapPackedRbLike_0x6CD710(_colorWeightPacked));
    }

    void Player::setColorWeight(tjs_int v) {
        _colorWeightPacked =
            swapPackedRbLike_0x6CD710(static_cast<std::uint32_t>(v));
    }

    tjs_int Player::getMaskMode() const { return _maskMode; }

    void Player::setMaskMode(tjs_int v) { _maskMode = v; }

    void Player::setIndependentLayerInherit(bool v) {
        if(_independentLayerInherit == v) {
            return;
        }

        _independentLayerInherit = v;
        if(!_runtime) {
            return;
        }

        // libkrkr2.so 0x6CC9D4 compares player+1097 and marks node+1584 dirty.
        for(auto &node : _runtime->nodes) {
            node.accumulated.dirty = true;
        }
    }

    Player::Player(ResourceManager rm, Player *parentPlayer) :
        _runtime(detail::makePlayerRuntime()),
        _resourceManagerNative(std::move(rm)), _parentPlayer(parentPlayer) {
        G_PluginLog.debug("Motion.Player constructor called");
        using ResourceManagerAdaptor = ncbInstanceAdaptor<ResourceManager>;
        if(auto *dispatch = ResourceManagerAdaptor::CreateAdaptor(
               new ResourceManager(_resourceManagerNative))) {
            _resourceManager = tTJSVariant(dispatch, dispatch);
            dispatch->Release();
        }
        // Aligned to libkrkr2.so SourceCache constructor/owner lifetime
        // (0x6A78F4): Player stores a TJS SourceCache object and calls through
        // that dispatch for source resolution rather than owning a map
        // directly.
        using SourceCacheAdaptor = ncbInstanceAdaptor<SourceCache>;
        auto *sourceCache = new SourceCache();
        sourceCache->bindRuntime(_runtime.get(), &_resourceManagerNative);
        if(auto *dispatch = SourceCacheAdaptor::CreateAdaptor(sourceCache)) {
            _runtime->sourceCacheNative = sourceCache;
            _runtime->sourceCacheObject = tTJSVariant(dispatch, dispatch);
            sourceCache->setSelfObject(_runtime->sourceCacheObject);
            dispatch->Release();
        } else {
            delete sourceCache;
        }
        // Aligned to sub_6A88CC (0x6A8988): create TJS Math.RandomGenerator
        // and store at player+992. Child Players inherit via sub_6CED30.
        try {
            TVPExecuteExpression(TJS_W("new Math.RandomGenerator()"),
                                 &_tjsRandomGenerator);
        } catch(...) {
            G_PluginLog.warn("Player: failed to create Math.RandomGenerator");
        }
    }

    Player::~Player() = default;

    bool Player::getPlaying() const {
        // Player_getPlaying @ 0x6D9794: return byte player+1099.
        static int traceCount = 0;
        if(_runtime && detail::logoChainTraceEnabled(_runtime->activeMotion) &&
           traceCount < 80) {
            ++traceCount;
            detail::logoChainTraceLogf(
                _runtime->activeMotion->path, "getPlaying", "0x6D9794",
                _clampedEvalTime, "value={} timelineCount={} playingLabels={}",
                _allplaying ? 1 : 0, _runtime->timelines.size(),
                _runtime->playingTimelineLabels.size());
        }
        return _allplaying;
    }

    bool Player::getAllplaying() const {
        // Player_getAllplaying @ 0x6CCE34: child Motion players can keep the
        // aggregate playing state true after the owner-level flag is clear.
        static int traceCount = 0;
        if(_runtime) {
            for(const auto &node : _runtime->nodes) {
                if(auto *child = node.getChildPlayer()) {
                    if(child->getAllplaying()) {
                        if(detail::logoChainTraceEnabled(
                               _runtime->activeMotion) &&
                           traceCount < 80) {
                            ++traceCount;
                            detail::logoChainTraceLogf(
                                _runtime->activeMotion->path, "getAllplaying",
                                "0x6CCE34", _clampedEvalTime,
                                "value=1 reason=child nodeIndex={} "
                                "localPlaying={} labels={}",
                                node.index, _allplaying ? 1 : 0,
                                _runtime->playingTimelineLabels.size());
                        }
                        return true;
                    }
                }
            }
        }
        if(_runtime && detail::logoChainTraceEnabled(_runtime->activeMotion) &&
           traceCount < 80) {
            ++traceCount;
            detail::logoChainTraceLogf(
                _runtime->activeMotion->path, "getAllplaying", "0x6CCE34",
                _clampedEvalTime, "value={} reason=local labels={}",
                _allplaying ? 1 : 0, _runtime->playingTimelineLabels.size());
        }
        return _allplaying;
    }

    // Aligned to libkrkr2.so Player_getRootX (0x6D98A8) / Player_setRootX
    // (0x6CD028):
    //   sub_6CD028: if (root.delta.posX != v) { root.delta.posX = v;
    //   root.delta.dirty = 1; } — writes node+1592 (delta.posX) and sets
    //   node+1584 (delta.dirty).
    double Player::getX() const {
        if(_runtime && !_runtime->nodes.empty())
            return _runtime->nodes[0].delta.posX;
        return _hasPendingRootPos ? _pendingRootX : 0.0;
    }
    void Player::setX(double v) {
        _pendingRootX = v;
        _hasPendingRootPos = true;
        if(_runtime && !_runtime->nodes.empty()) {
            auto &root = _runtime->nodes[0];
            if(root.delta.posX != v) {
                root.delta.posX = v;
                root.delta.dirty = true;
            }
        }
    }
    // Aligned to libkrkr2.so Player_getRootY (0x6D98B4) / Player_setRootY
    // (0x6CD048): same shape as setRootX but at node+1600 (delta.posY).
    double Player::getY() const {
        if(_runtime && !_runtime->nodes.empty())
            return _runtime->nodes[0].delta.posY;
        return _hasPendingRootPos ? _pendingRootY : 0.0;
    }
    void Player::setY(double v) {
        _pendingRootY = v;
        _hasPendingRootPos = true;
        if(_runtime && !_runtime->nodes.empty()) {
            auto &root = _runtime->nodes[0];
            if(root.delta.posY != v) {
                root.delta.posY = v;
                root.delta.dirty = true;
            }
        }
    }

    // Aligned to libkrkr2.so EmoteObject_init (sub_67DBAC):
    // Sets activeMotion directly from a pre-loaded snapshot, bypassing file
    // I/O. Used by EmotePlayer.setModule() to bridge loaded PSB data into the
    // Player pipeline.
    void
    Player::loadFromSnapshot(std::shared_ptr<detail::MotionSnapshot> snapshot) {
        _runtime->activeMotion.reset();
        _runtime->timelines.clear();
        _runtime->playingTimelineLabels.clear();
        _runtime->drawAffineMatrix = { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 };
        _variableKeys.Clear();
        _variableValues.clear();
        _variableAnimators.clear();
        clearControllerAnimatorStateLike_0x671228();
        _evalResultValues.clear();
        _evalResultList.clear();
        _evalResultListIndex.clear();
        _mirrorPositiveCache.clear();
        _mirrorNegativeCache.clear();

        if(snapshot) {
            snapshot->attachedSnapshots.clear();
            activateMotion(*_runtime, snapshot, &_resourceManagerNative);
            syncVariableKeysFromActiveMotion();
        }
    }

    void
    Player::addEmoteFile(std::shared_ptr<detail::MotionSnapshot> snapshot) {
        if(!_runtime || !_runtime->activeMotion || !snapshot) {
            return;
        }
        if(snapshot.get() == _runtime->activeMotion.get()) {
            return;
        }

        auto &attached = _runtime->activeMotion->attachedSnapshots;
        for(const auto &existing : attached) {
            if(existing.get() == snapshot.get()) {
                return;
            }
        }
        attached.push_back(std::move(snapshot));
        detail::mergeAttachedSnapshotResources(*_runtime->activeMotion,
                                               *attached.back());
        G_PluginLog.debug(
            "Player::addEmoteFile({}): attached to primary path={} (count={})",
            attached.back()->path, _runtime->activeMotion->path,
            attached.size());
    }

    void Player::bindMotionModuleKey(ttstr storageKey) {
        // 参考 sdl3/emoteplayerclass.cpp（不编译）: set_motionKey()
        //   _currentfile = _resourceManager->GetPlayerByName(motionKey);
        // Does NOT start playback — that is play()'s job.
        const auto loaded = _resourceManagerNative.findLoadedModule(storageKey);
        if(loaded.Type() != tvtObject) {
            G_PluginLog.warn(
                "Player::bindMotionModuleKey({}): module not in "
                "ResourceManager cache; call ResourceManager.load() first",
                storageKey.AsStdString());
            return;
        }

        _project = loaded;
        if(const auto snapshot = detail::lookupModuleSnapshot(loaded)) {
            loadFromSnapshot(snapshot);
            G_PluginLog.debug(
                "Player::bindMotionModuleKey({}): bound snapshot path={}",
                storageKey.AsStdString(), snapshot->path);
            return;
        }

        // Fallback: resolve via file path (decompiled libkrkr2.so path).
        if(const auto snapshot =
               resolveMotion(*_runtime, storageKey, &_resourceManagerNative)) {
            activateMotion(*_runtime, snapshot, &_resourceManagerNative);
            syncVariableKeysFromActiveMotion();
            G_PluginLog.debug(
                "Player::bindMotionModuleKey({}): resolved snapshot path={}",
                storageKey.AsStdString(), snapshot->path);
            return;
        }

        G_PluginLog.error(
            "Player::bindMotionModuleKey({}): loaded object has no motion "
            "snapshot",
            storageKey.AsStdString());
        throw std::runtime_error(
            "motionplayer: motionKey module has no parseable motion snapshot");
    }

    double Player::getActiveMotionWidth() const {
        return _runtime->activeMotion ? _runtime->activeMotion->width : 0.0;
    }

    double Player::getActiveMotionHeight() const {
        return _runtime->activeMotion ? _runtime->activeMotion->height : 0.0;
    }

    void Player::setMotion(ttstr v) {
        if(_motionKey == v) {
            return;
        }
        _motionKey = v;
        _runtime->activeMotion.reset();
        _runtime->timelines.clear();
        _runtime->playingTimelineLabels.clear();
        _runtime->drawAffineMatrix = { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 };
        _variableKeys.Clear();
        _variableValues.clear();
        _variableAnimators.clear();
        clearControllerAnimatorStateLike_0x671228();
        _evalResultValues.clear();
        _evalResultList.clear();
        _evalResultListIndex.clear();
        _mirrorPositiveCache.clear();
        _mirrorNegativeCache.clear();
        if(ensureMotionLoaded()) {
            initNonEmoteMotionLike_0x6B365C(0);
        }
    }

    // Aligned to libkrkr2.so 0x681CAC → 0x6B0F10:
    // motion setter calls objthis.onFindMotion({chara, motion}) to let
    // TJS participate in path resolution before loading the PSB.
    tjs_error Player::setMotionCompat(tTJSVariant *result, tjs_int numparams,
                                      tTJSVariant **param,
                                      iTJSDispatch2 *objthis) {
        auto *self =
            ncbInstanceAdaptor<Player>::GetNativeInstance(objthis, true);
        if(!self)
            return TJS_E_INVALIDOBJECT;

        ttstr motionValue;
        if(numparams > 0 && param[0] && param[0]->Type() != tvtVoid) {
            motionValue = *param[0];
        }

        if(self->_motionKey == motionValue) {
            return TJS_S_OK;
        }

        // Build dict {chara, motion} and call objthis.onFindMotion(dict)
        // Aligned to libkrkr2.so Player_loadMotion_guess (0x6B0F10)
        tTJSVariant dictVar =
            detail::makeDictionary({ { "chara", tTJSVariant(self->_chara) },
                                     { "motion", tTJSVariant(motionValue) } });
        tTJSVariant onFindResult;
        tTJSVariant *args[] = { &dictVar };
        tjs_error hr = objthis->FuncCall(0, TJS_W("onFindMotion"), nullptr,
                                         &onFindResult, 1, args, objthis);

        // Read back (possibly modified) chara and motion from result
        if(TJS_SUCCEEDED(hr) && onFindResult.Type() == tvtObject) {
            iTJSDispatch2 *resObj = onFindResult.AsObjectNoAddRef();
            if(resObj) {
                tTJSVariant charaVal, motionVal;
                if(TJS_SUCCEEDED(resObj->PropGet(TJS_MEMBERMUSTEXIST,
                                                 TJS_W("chara"), nullptr,
                                                 &charaVal, resObj)) &&
                   charaVal.Type() != tvtVoid) {
                    self->_chara = ttstr(charaVal);
                }
                if(TJS_SUCCEEDED(resObj->PropGet(TJS_MEMBERMUSTEXIST,
                                                 TJS_W("motion"), nullptr,
                                                 &motionVal, resObj)) &&
                   motionVal.Type() != tvtVoid) {
                    motionValue = ttstr(motionVal);
                }
            }
        }

        // Reset state and load
        self->_motionKey = motionValue;
        self->_runtime->activeMotion.reset();
        self->_runtime->timelines.clear();
        self->_runtime->playingTimelineLabels.clear();
        self->_runtime->drawAffineMatrix = { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 };
        self->_variableKeys.Clear();
        self->_variableValues.clear();
        if(self->ensureMotionLoaded()) {
            self->initNonEmoteMotionLike_0x6B365C(0);
        }

        return TJS_S_OK;
    }

    tjs_error Player::getMotionCompat(tTJSVariant *result, tjs_int,
                                      tTJSVariant **, iTJSDispatch2 *objthis) {
        auto *self =
            ncbInstanceAdaptor<Player>::GetNativeInstance(objthis, true);
        if(!self)
            return TJS_E_INVALIDOBJECT;
        // Player_getMotion_ncb @ 0x6D9544 returns native player+976.
        // _motionKey is the local mirror of that getter-visible slot.
        if(result)
            *result = tTJSVariant(self->_motionKey);
        return TJS_S_OK;
    }

    bool Player::ensureMotionLoaded() {
        if(_runtime->activeMotion) {
            return true;
        }

        const auto motionKey = detail::narrow(_motionKey);
        const bool motionKeyLooksLikeStorage =
            motionKey.find('/') != std::string::npos ||
            motionKey.find('\\') != std::string::npos ||
            motionKey.find('.') != std::string::npos;

        if(_project.Type() == tvtObject) {
            if(const auto snapshot = detail::lookupModuleSnapshot(_project)) {
                activateMotion(*_runtime, snapshot, &_resourceManagerNative);
                syncVariableKeysFromActiveMotion();
                return true;
            }
        }

        if(motionKeyLooksLikeStorage) {
            if(const auto snapshot = resolveMotion(*_runtime, _motionKey,
                                                   &_resourceManagerNative)) {
                activateMotion(*_runtime, snapshot, &_resourceManagerNative);
                syncVariableKeysFromActiveMotion();
                return true;
            }
        }

        if(const auto loaded = _resourceManagerNative.getLastLoadedModule();
           loaded.Type() == tvtObject) {
            if(const auto snapshot = detail::lookupModuleSnapshot(loaded)) {
                activateMotion(*_runtime, snapshot, &_resourceManagerNative);
                syncVariableKeysFromActiveMotion();
                return true;
            }
        }

        if(_motionKey.IsEmpty()) {
            return false;
        }

        if(const auto snapshot =
               resolveMotion(*_runtime, _motionKey, &_resourceManagerNative)) {
            activateMotion(*_runtime, snapshot, &_resourceManagerNative);
            syncVariableKeysFromActiveMotion();
            return true;
        }

        return false;
    }

    void Player::initNonEmoteMotionLike_0x6B365C(std::uint32_t playFlags) {
        if(!_runtime || !_runtime->activeMotion) {
            return;
        }
        // e-mote3 PSB 常为 type=0（motion）但含 variableList；与 type=1
        // 走同一初始化路径。
        if(_runtime->isEmoteMode ||
           !_runtime->activeMotion->variableLabels.empty()) {
            initEmoteMotionLike_0x6B3A8C(playFlags);
            return;
        }

        const auto *clip = selectActiveClip();
        _runtime->activeClip = clip;

        resetNodeTreeForBuildLike_0x6B56F8();
        _runtime->parameterEntries.clear();
        _runtime->parameterEntryById.clear();
        _runtime->defaultParameterEntry = {};
        _runtime->defaultParameterEntry.rangeScale = 1.0;
        _runtime->defaultParameterEntry.mode = 0;
        _runtime->defaultParameterEntryPtr = nullptr;
        _runtime->defaultParameterEntryIndex = -1;

        if(clip != nullptr) {
            _loopTime = clip->loopTime;
            _cachedTotalFrames = clip->totalFrames;
        }

        const auto motionObject = clip ? clip->motionObject : nullptr;
        if(motionObject) {
            const auto parameterizeValue = (*motionObject)["parameterize"];
            if(auto parameterizeObject =
                   std::dynamic_pointer_cast<const PSB::PSBDictionary>(
                       parameterizeValue)) {
                appendParameterEntryLike_0x6B1718(parameterizeObject);
                finalizeParameterTableLike_0x6B1ECC();
                if(!_runtime->parameterEntries.empty()) {
                    _runtime->defaultParameterEntryIndex = 0;
                    _runtime->defaultParameterEntryPtr =
                        &_runtime->parameterEntries.front();
                }
            } else {
                parseParameterListLike_0x6B202C((*motionObject)["parameter"]);
                if(auto numeric = std::dynamic_pointer_cast<PSB::PSBNumber>(
                       parameterizeValue)) {
                    int index = 0;
                    switch(numeric->numberType) {
                        case PSB::PSBNumberType::Float:
                            index =
                                static_cast<int>(numeric->getValue<float>());
                            break;
                        case PSB::PSBNumberType::Double:
                            index =
                                static_cast<int>(numeric->getValue<double>());
                            break;
                        case PSB::PSBNumberType::Int:
                            index = numeric->getValue<int>();
                            break;
                        case PSB::PSBNumberType::Long:
                        default:
                            index = static_cast<int>(
                                numeric->getValue<tjs_int64>());
                            break;
                    }
                    if(index < 0 ||
                       static_cast<size_t>(index) >=
                           _runtime->parameterEntries.size()) {
                        throw std::out_of_range("parameter id out of range.");
                    }
                    _runtime->defaultParameterEntryIndex = index;
                    _runtime->defaultParameterEntryPtr =
                        &_runtime->parameterEntries[static_cast<size_t>(index)];
                }
            }
        }

        buildNodeTree();
        initVariables();
        seedEmoteVariableDefaultsLike_sdl3();
        syncParameterEntriesFromVariablesLike_sdl3();
        logEmoteInitDiagnosticsOnce();

        if((playFlags & PlayFlagChain) == 0) {
            _frameLoopTime = 0.0;
            _clampedEvalTime = std::min(_cachedTotalFrames, 0.0);
            _queuing = true;
            _allplaying = true;
        }
        _allplaying = true;
    }

    void Player::initEmoteMotionLike_0x6B3A8C(std::uint32_t playFlags) {
        if(!_runtime || !_runtime->activeMotion) {
            return;
        }

        const auto *clip = selectActiveClip();
        _runtime->activeClip = clip;

        resetNodeTreeForBuildLike_0x6B56F8();
        _runtime->defaultParameterEntry = {};
        _runtime->defaultParameterEntryPtr = nullptr;
        _runtime->defaultParameterEntryIndex = -1;

        const bool motionChanged = !_runtime->activeMotion ||
            _runtime->cachedParameterMotionPath != _runtime->activeMotion->path;
        if(motionChanged) {
            _runtime->parameterEntries.clear();
            _runtime->parameterEntryById.clear();
            _runtime->cachedParameterMotionPath = _runtime->activeMotion
                ? _runtime->activeMotion->path
                : std::string{};
            _runtime->emoteDiagLogged = false;
            _runtime->emoteFirstEvalDiagLogged = false;
        }

        if(clip != nullptr) {
            _loopTime = clip->loopTime;
            _cachedTotalFrames = clip->totalFrames;
        } else if(_runtime->activeMotion) {
            _cachedTotalFrames = 0.0;
            for(const auto &[label, binding] :
                _runtime->activeMotion->timelineControlByLabel) {
                _cachedTotalFrames =
                    std::max(_cachedTotalFrames, binding.lastTime);
                if(binding.loopEnd >= binding.loopBegin) {
                    _cachedTotalFrames =
                        std::max(_cachedTotalFrames, binding.loopEnd);
                }
                (void)label;
            }
        }

        if(_runtime->parameterEntries.empty()) {
            loadMotionParameterTableLike_sdl3();
        }

        buildNodeTree();
        if(_runtime->defaultParameterEntryIndex >= 0) {
            const int defaultIndex = _runtime->defaultParameterEntryIndex;
            for(size_t i = 1; i < _runtime->nodes.size(); ++i) {
                auto &node = _runtime->nodes[i];
                if(node.parameterizeIndex >= 0) {
                    continue;
                }
                node.parameterizeIndex = defaultIndex;
                if(static_cast<size_t>(defaultIndex) <
                   _runtime->parameterEntries.size()) {
                    node.parameterEntry =
                        &_runtime->parameterEntries[static_cast<size_t>(
                            defaultIndex)];
                }
            }
        }
        initVariables();
        seedEmoteVariableDefaultsLike_sdl3();
        syncParameterEntriesFromVariablesLike_sdl3();
        logEmoteInitDiagnosticsOnce();

        if((playFlags & PlayFlagChain) == 0) {
            _frameLoopTime = 0.0;
            _clampedEvalTime = 0.0;
            _queuing = true;
        }
        _speed = true;
        _allplaying = true;

        if(_runtime->nodes.size() <= 1) {
            G_PluginLog.warn(
                "Player::initEmoteMotionLike_0x6B3A8C: node tree empty for "
                "{} — check clip.layer[] index resolution (C-1)",
                _runtime->activeMotion->path);
        }
    }

    void Player::loadMotionParameterTableLike_sdl3() {
        if(!_runtime || !_runtime->activeMotion) {
            return;
        }

        const auto loadFromObject =
            [this](
                const std::shared_ptr<const PSB::PSBDictionary> &obj) -> bool {
            if(!obj) {
                return false;
            }
            const auto parameterizeValue = (*obj)["parameterize"];
            if(auto parameterizeObject =
                   std::dynamic_pointer_cast<const PSB::PSBDictionary>(
                       parameterizeValue)) {
                appendParameterEntryLike_0x6B1718(parameterizeObject);
                finalizeParameterTableLike_0x6B1ECC();
                if(!_runtime->parameterEntries.empty()) {
                    _runtime->defaultParameterEntryIndex = 0;
                    _runtime->defaultParameterEntryPtr =
                        &_runtime->parameterEntries.front();
                }
                return true;
            }

            parseParameterListLike_0x6B202C((*obj)["parameter"]);
            if(_runtime->parameterEntries.empty()) {
                return false;
            }
            if(auto numeric = std::dynamic_pointer_cast<PSB::PSBNumber>(
                   parameterizeValue)) {
                int index = 0;
                switch(numeric->numberType) {
                    case PSB::PSBNumberType::Float:
                        index = static_cast<int>(numeric->getValue<float>());
                        break;
                    case PSB::PSBNumberType::Double:
                        index = static_cast<int>(numeric->getValue<double>());
                        break;
                    case PSB::PSBNumberType::Int:
                        index = numeric->getValue<int>();
                        break;
                    case PSB::PSBNumberType::Long:
                    default:
                        index =
                            static_cast<int>(numeric->getValue<tjs_int64>());
                        break;
                }
                if(index >= 0 &&
                   static_cast<size_t>(index) <
                       _runtime->parameterEntries.size()) {
                    _runtime->defaultParameterEntryIndex = index;
                    _runtime->defaultParameterEntryPtr =
                        &_runtime->parameterEntries[static_cast<size_t>(index)];
                }
            } else if(!_runtime->parameterEntries.empty()) {
                _runtime->defaultParameterEntryIndex = 0;
                _runtime->defaultParameterEntryPtr =
                    &_runtime->parameterEntries.front();
            }
            return true;
        };

        auto &motion = *_runtime->activeMotion;
        const detail::MotionClip *preferred = nullptr;
        const detail::MotionClip *largest = nullptr;
        size_t largestCount = 0;

        for(const auto &motionClip : motion.clipList) {
            if(!motionClip.motionObject) {
                continue;
            }
            const auto paramList = std::dynamic_pointer_cast<PSB::PSBList>(
                (*motionClip.motionObject)["parameter"]);
            const size_t count = paramList ? paramList->size() : 0;
            if(count > largestCount) {
                largestCount = count;
                largest = &motionClip;
            }
            if(motionClip.label == "頭部変形基礎" ||
               motionClip.label == "全体構造") {
                if(count > 0) {
                    preferred = &motionClip;
                }
            }
        }

        const detail::MotionClip *clip = selectActiveClip();
        if(preferred) {
            loadFromObject(preferred->motionObject);
        } else if(largest) {
            loadFromObject(largest->motionObject);
        } else if(clip && clip->motionObject) {
            loadFromObject(clip->motionObject);
        }

        if(_runtime->parameterEntries.empty()) {
            if(!loadFromObject(motion.root) && motion.root) {
                const auto content =
                    std::dynamic_pointer_cast<const PSB::PSBDictionary>(
                        (*motion.root)["content"]);
                loadFromObject(content);
            }
        }
    }

    void Player::seedEmoteVariableDefaultsLike_sdl3() {
        if(!_runtime || !_runtime->activeMotion) {
            return;
        }
        // 参考 sdl3 emotemetadata::_varList 初始化为 0（不编译）。
        for(const auto &label : _runtime->activeMotion->variableLabels) {
            if(_variableValues.find(label) == _variableValues.end()) {
                _variableValues[label] = 0.0;
            }
        }
        for(const auto &[label, frames] :
            _runtime->activeMotion->variableFrames) {
            if(frames.empty() ||
               _variableValues.find(label) != _variableValues.end()) {
                continue;
            }
            _variableValues[label] = frames.front().value;
        }
    }

    void Player::logEmoteInitDiagnosticsOnce() {
        if(!_runtime || !_runtime->activeMotion) {
            return;
        }
        const bool emoteLike = _runtime->isEmoteMode ||
            !_runtime->parameterEntries.empty() ||
            !_runtime->activeMotion->variableLabels.empty();
        if(!emoteLike) {
            return;
        }
        const auto *clip = _runtime->activeClip;
        if(clip && clip->label != "全体構造" && clip->label != "頭部変形基礎" &&
           _runtime->nodes.size() < 40) {
            return;
        }
        if(_runtime->emoteDiagLogged &&
           _runtime->emoteDiagMotionPath == _runtime->activeMotion->path) {
            return;
        }
        _runtime->emoteDiagLogged = true;
        _runtime->emoteDiagMotionPath = _runtime->activeMotion->path;
        int paramNodeCount = 0;
        int sourcedCount = 0;
        int stencilZeroCount = 0;
        int paramNodeEmptySrc = 0;
        std::string paramNodeSample;
        for(size_t i = 1; i < _runtime->nodes.size(); ++i) {
            const auto &node = _runtime->nodes[i];
            if(node.hasSource) {
                ++sourcedCount;
            }
            if(node.hasSource && node.stencilType == 0) {
                ++stencilZeroCount;
            }
            if(node.parameterizeIndex < 0) {
                continue;
            }
            ++paramNodeCount;
            if(node.hasSource && node.interpolatedCache.src.empty()) {
                ++paramNodeEmptySrc;
            }
            if(node.parameterizeIndex >= 0 &&
               static_cast<size_t>(node.parameterizeIndex) >=
                   _runtime->parameterEntries.size()) {
                ++paramNodeEmptySrc;
            }
            if(paramNodeSample.size() < 240) {
                if(!paramNodeSample.empty()) {
                    paramNodeSample += ", ";
                }
                double tick = 0.0;
                if(node.parameterEntry) {
                    tick = node.parameterEntry->value;
                }
                paramNodeSample += fmt::format(
                    "{}[p{} tick={:.2f} src={} stencil={}]",
                    node.layerName.empty() ? "<none>" : node.layerName,
                    node.parameterizeIndex, tick, node.hasSource ? "y" : "n",
                    node.stencilType);
            }
        }

        std::string paramTableSample;
        for(size_t i = 0; i < _runtime->parameterEntries.size() &&
            paramTableSample.size() < 240;
            ++i) {
            const auto &entry = _runtime->parameterEntries[i];
            if(!paramTableSample.empty()) {
                paramTableSample += ", ";
            }
            paramTableSample += fmt::format("{}={:.2f}", entry.id, entry.value);
        }

        G_PluginLog.warn(
            "emote init diag: emoteMode={} path={} clip={} nodes={} "
            "layerList={} "
            "params={} paramNodes={} sourced={} stencil0={} "
            "paramOOR={} paramTable=[{}] paramNodesSample=[{}]",
            _runtime->isEmoteMode ? 1 : 0, _runtime->activeMotion->path,
            clip ? clip->label : std::string("<none>"), _runtime->nodes.size(),
            _runtime->activeMotion->layerList.size(),
            _runtime->parameterEntries.size(), paramNodeCount, sourcedCount,
            stencilZeroCount, paramNodeEmptySrc, paramTableSample,
            paramNodeSample);
        if(_runtime->parameterEntries.empty()) {
            G_PluginLog.warn(
                "emote init diag: parameter table empty for {} — check "
                "clip/root parameter[] (对照 sdl3 emotemotion::parameter)",
                _runtime->activeMotion->path);
        }
    }

    void Player::logEmoteFirstEvalDiagnosticsOnce() {
        if(!_runtime || !_runtime->activeMotion) {
            return;
        }
        const bool emoteLike = _runtime->isEmoteMode ||
            !_runtime->parameterEntries.empty() ||
            !_runtime->activeMotion->variableLabels.empty();
        if(!emoteLike) {
            return;
        }
        const auto *clip = _runtime->activeClip;
        if(clip && clip->label != "頭部変形基礎" &&
           _runtime->nodes.size() < 80) {
            return;
        }
        if(_runtime->emoteFirstEvalDiagLogged &&
           _runtime->emoteDiagMotionPath == _runtime->activeMotion->path) {
            return;
        }
        _runtime->emoteFirstEvalDiagLogged = true;
        _runtime->emoteDiagMotionPath = _runtime->activeMotion->path;

        int inactiveActive = 0;
        int paramNodeCount = 0;
        int emptySrcCount = 0;
        int hiddenCount = 0;
        std::string sample;
        std::string faceSample;
        for(size_t i = 1; i < _runtime->nodes.size(); ++i) {
            const auto &node = _runtime->nodes[i];
            const auto &name = node.layerName;
            const bool facePart = name.find("mouth") != std::string::npos ||
                name.find("Mouth") != std::string::npos ||
                name.find("nose") != std::string::npos ||
                name.find("Nose") != std::string::npos ||
                name.find("eye") != std::string::npos ||
                name.find("Eye") != std::string::npos ||
                name.find("口") != std::string::npos ||
                name.find("鼻") != std::string::npos ||
                name.find("目") != std::string::npos;
            if(node.parameterizeIndex < 0) {
                if(facePart && faceSample.size() < 200) {
                    if(!faceSample.empty()) {
                        faceSample += ", ";
                    }
                    faceSample += fmt::format("{}[p=-1 src={} draw={}]",
                                              name.empty() ? "<none>" : name,
                                              node.interpolatedCache.src.empty()
                                                  ? "n"
                                                  : node.interpolatedCache.src,
                                              node.drawFlag ? "y" : "n");
                }
                continue;
            }
            ++paramNodeCount;
            if(node.hasSource && node.interpolatedCache.src.empty()) {
                ++emptySrcCount;
            }
            if(!node.drawFlag) {
                ++hiddenCount;
            }
            if(!node.accumulated.active) {
                ++inactiveActive;
            }
            if(facePart && faceSample.size() < 320) {
                if(!faceSample.empty()) {
                    faceSample += ", ";
                }
                std::string paramId;
                double tick = 0.0;
                if(node.parameterEntry) {
                    paramId = node.parameterEntry->id;
                    tick = node.parameterEntry->value;
                }
                faceSample += fmt::format(
                    "{}[p{} id={} tick={:.2f} src={} draw={}]",
                    name.empty() ? "<none>" : name, node.parameterizeIndex,
                    paramId.empty() ? "?" : paramId, tick,
                    node.interpolatedCache.src.empty()
                        ? "n"
                        : node.interpolatedCache.src,
                    node.drawFlag ? "y" : "n");
            }
            if(sample.size() < 320) {
                if(!sample.empty()) {
                    sample += ", ";
                }
                std::string paramId;
                double tick = 0.0;
                if(node.parameterEntry) {
                    paramId = node.parameterEntry->id;
                    tick = node.parameterEntry->value;
                }
                sample += fmt::format(
                    "{}[p{} id={} tick={:.2f} src={} draw={} stencil={}]",
                    node.layerName.empty() ? "<none>" : node.layerName,
                    node.parameterizeIndex, paramId.empty() ? "?" : paramId,
                    tick,
                    node.interpolatedCache.src.empty()
                        ? "n"
                        : node.interpolatedCache.src,
                    node.drawFlag ? "y" : "n", node.stencilType);
            }
        }

        G_PluginLog.warn(
            "emote first eval: emoteMode={} path={} clip={} paramNodes={} "
            "emptySrc={} hidden={} inactiveActive={} face=[{}] sample=[{}]",
            _runtime->isEmoteMode ? 1 : 0, _runtime->activeMotion->path,
            clip ? clip->label : std::string("<none>"), paramNodeCount,
            emptySrcCount, hiddenCount, inactiveActive, faceSample, sample);
        if(emptySrcCount > 0 || hiddenCount > 0) {
            G_PluginLog.warn(
                "emote first eval: {} param nodes missing src, {} hidden, {} "
                "inactive accumulated.active (口/眼: delta.visibleOverride / "
                "parent.visible / stencilType)",
                emptySrcCount, hiddenCount, inactiveActive);
        }
    }

    void Player::syncVariableKeysFromActiveMotion() {
        if(!_runtime->activeMotion) {
            _variableKeys = detail::makeArray({});
            return;
        }

        _variableKeys = detail::makeArray(
            detail::stringsToVariants(_runtime->activeMotion->variableLabels));
        syncSelectorControlsLike_0x670D1C();
    }

    void Player::syncSelectorControlsLike_0x670D1C() {
        const auto *activeMotion = _runtime->activeMotion.get();
        if(!activeMotion) {
            return;
        }

        const auto removeRuntimeState = [this](const std::string &label) {
            if(label.empty()) {
                return;
            }
            _variableAnimators.erase(label);
            eraseControllerAnimatorStateLike_0x671228(label);
            _variableValues.erase(label);
            _evalResultValues.erase(label);
            removeEvalResultSlotLike_Reset(label);
        };

        for(const auto &[selectorLabel, binding] :
            activeMotion->selectorControls) {
            removeRuntimeState(selectorLabel);
            for(const auto &option : binding.options) {
                removeRuntimeState(option.label);
            }

            if(!_selectorEnabled) {
                continue;
            }

            // Aligned to libkrkr2.so sub_670D1C:
            // selector-enabled path resets each selector controller and
            // immediately applies sub_6680B0(..., index=0, transition=0,
            // ease=0).
            setVariable(detail::widen(selectorLabel), 0.0, 0.0, 0.0);
        }

        _emoteDirty = true;
    }

    const detail::TimelineState *
    Player::primaryTimelineStateLike_0x66F80C() const {
        if(!_runtime->activeMotion) {
            return nullptr;
        }

        const auto &primaryLabels =
            !_runtime->activeMotion->mainTimelineLabels.empty()
            ? _runtime->activeMotion->mainTimelineLabels
            : _runtime->activeMotion->diffTimelineLabels;
        for(const auto &label : primaryLabels) {
            if(const auto it = _runtime->timelines.find(label);
               it != _runtime->timelines.end()) {
                return &it->second;
            }
        }

        if(!_motionKey.IsEmpty()) {
            if(const auto it =
                   _runtime->timelines.find(detail::narrow(_motionKey));
               it != _runtime->timelines.end()) {
                return &it->second;
            }
        }

        return !_runtime->timelines.empty()
            ? &(_runtime->timelines.begin()->second)
            : nullptr;
    }

    void Player::resetControllerStateLike_0x66EB8C() {
        // Aligned to libkrkr2.so sub_66EB8C:
        // the binary performs a broad controller/reset sweep after wrapper-side
        // setMirror(). Keep the local reset focused on runtime controller
        // state, eval sinks, and root-node dirty propagation.
        _variableAnimators.clear();
        clearControllerAnimatorStateLike_0x671228();
        _evalResultValues.clear();
        _evalResultList.clear();
        _evalResultListIndex.clear();
        _mirrorPositiveCache.clear();
        _mirrorNegativeCache.clear();

        if(_runtime && !_runtime->nodes.empty()) {
            auto &root = _runtime->nodes.front();
            // Aligned to libkrkr2.so Player_setRootFlipX (0x6CD068):
            // writes node+1587 (delta.flipX), sets node+1584 (delta.dirty).
            root.delta.flipX = _rootFlipX;
            root.delta.dirty = true;
            root.interpolatedCache.flipX = _rootFlipX;
        }

        if(_selectorEnabled) {
            syncSelectorControlsLike_0x670D1C();
        }
        _emoteDirty = true;
    }

    const detail::MotionClip *Player::selectActiveClip() const {
        if(!_runtime->activeMotion) {
            return nullptr;
        }

        const auto &motion = *_runtime->activeMotion;
        const auto selectByLabel =
            [&motion](const std::string &label) -> const detail::MotionClip * {
            if(label.empty()) {
                return nullptr;
            }
            const auto it = motion.clipIndexByLabel.find(label);
            if(it == motion.clipIndexByLabel.end())
                return nullptr;
            const int idx = it->second;
            if(idx < 0 || idx >= static_cast<int>(motion.clipList.size()))
                return nullptr;
            return &motion.clipList[idx];
        };

        // Aligned to libkrkr2.so Player_playImpl (0x6B2284):
        // the requested motion/timeline label is stored on the player before
        // the non-emote init path rebuilds content/node state. In the local
        // architecture, this is the closest equivalent to the binary's
        // selected content object, so prefer _motionKey before falling back to
        // the playing-timeline list or primary label ordering.
        if(!_motionKey.IsEmpty()) {
            if(const auto *clip = selectByLabel(detail::narrow(_motionKey))) {
                return clip;
            }
        }

        for(const auto &label : _runtime->playingTimelineLabels) {
            if(const auto *clip = selectByLabel(label)) {
                return clip;
            }
        }

        const auto &primaryLabels = !motion.mainTimelineLabels.empty()
            ? motion.mainTimelineLabels
            : motion.diffTimelineLabels;
        for(const auto &label : primaryLabels) {
            if(const auto *clip = selectByLabel(label)) {
                return clip;
            }
        }

        // Fallback — aligned to libkrkr2.so Player_initNonEmoteMotion reading
        // priority[0].content at 0x6B38FC when no explicit selection exists.
        if(motion.clipList.size() == 1) {
            return &motion.clipList.front();
        }
        if(!motion.clipList.empty()) {
            return &motion.clipList.front();
        }

        return nullptr;
    }

    const std::vector<std::string> &Player::activeSourceCandidates() const {
        static const std::vector<std::string> empty;
        if(!_runtime->activeMotion) {
            return empty;
        }

        if(const auto *clip = selectActiveClip();
           clip && !clip->sourceCandidates.empty()) {
            return clip->sourceCandidates;
        }

        return _runtime->activeMotion->sourceCandidates;
    }

    tTJSVariant Player::getVariableKeys() {
        ensureMotionLoaded();
        if(_variableKeys.Type() == tvtVoid) {
            return detail::makeArray({});
        }
        return _variableKeys;
    }

    void Player::setProgressCompat(double v) {
        ensureMotionLoaded();
        const auto progress = std::clamp(v, 0.0, 1.0);
        _runtime->playingTimelineLabels.clear();
        for(auto &[_, state] : _runtime->timelines) {
            if(state.totalFrames > 0.0) {
                state.currentTime = state.totalFrames * progress;
            } else {
                state.currentTime = progress;
            }
            if(progress >= 1.0 && !state.loop) {
                state.playing = false;
            }
            state.controlInitialized = false;
            state.controlLastAppliedTime = state.currentTime;
            state.controlFrameCursor.clear();
            state.controlTrackValues.clear();
            state.controlTrackAnimators.clear();
            if(state.playing) {
                _runtime->playingTimelineLabels.push_back(state.label);
            }
        }
        _allplaying = !_runtime->playingTimelineLabels.empty();
    }

    double Player::getProgressCompat() const {
        bool sawTimeline = false;
        bool anyPlaying = false;
        double progress = 0.0;

        for(const auto &[_, state] : _runtime->timelines) {
            sawTimeline = true;
            anyPlaying = anyPlaying || state.playing;
            if(state.totalFrames > 0.0) {
                progress =
                    std::max(progress,
                             std::clamp(state.currentTime / state.totalFrames,
                                        0.0, 1.0));
            } else if(!state.playing) {
                progress = std::max(progress, 1.0);
            }
        }

        if(!sawTimeline) {
            return _allplaying ? 0.0 : 1.0;
        }
        if(!anyPlaying) {
            return 1.0;
        }
        return progress;
    }

    // --- Core methods ---
    // Aligned to libkrkr2.so sub_6BA7B8 at 0x6BA7B8:
    // 1. sub_A0F5E0(v9, a1+992) — read TJS dispatch from player+992
    // 2. FuncCall(obj, 0, L"random", ...) — call "random" method
    // 3. Convert result variant to double (case 2→real, case 4→int→double, case
    // 5→raw)
    //
    // player+992 is initialized once via "new Math.RandomGenerator()"
    // (sub_6A88CC at 0x6A8988). Child Players inherit the same object from
    // parent (sub_6CED30 at 0x6CED30: a1+992 = a2).
    double Player::random() {
        if(_tjsRandomGenerator.Type() == tvtObject) {
            iTJSDispatch2 *obj = _tjsRandomGenerator.AsObjectNoAddRef();
            if(obj) {
                tTJSVariant result;
                static tjs_uint32 hint = 0;
                tjs_error hr = obj->FuncCall(0, TJS_W("random"), &hint, &result,
                                             0, nullptr, obj);
                if(TJS_SUCCEEDED(hr))
                    return static_cast<double>(result);
            }
        }
        return 0.0;
    }

    // Aligned to libkrkr2.so sub_6709AC at 0x6709AC:
    // initPhysics(min, max, amp, freq1, freq2) creates a Spring physics object
    // at player+1128 (1564 bytes, sub_670AFC). Stores params at
    // player+1136..1152. The Spring physics system drives emote hair/bust/parts
    // oscillation. Full spring simulation not yet implemented for web port —
    // store params only.
    void Player::initPhysics(tTJSVariant metadata) {
        // 参考 sdl3/emoteplayerclass.cpp initPhysics()（不编译）；libkrkr2.so
        // 弹簧参数待补全 player+1128 = physics object (not created)
        // player+1136..1152 = min, max, amplitude, freq1, freq2
    }
    tTJSVariant Player::serialize() {
        ensureMotionLoaded();

        std::vector<std::pair<std::string, tTJSVariant>> variables;
        std::unordered_set<std::string> seenVariables;
        if(_runtime->activeMotion) {
            for(const auto &label : _runtime->activeMotion->variableLabels) {
                seenVariables.insert(label);
                variables.emplace_back(label,
                                       getVariable(detail::widen(label)));
            }
        }
        for(const auto &[label, value] : _variableValues) {
            if(seenVariables.insert(label).second) {
                variables.emplace_back(label, value);
            }
        }

        return detail::makeDictionary({
            { "chara", _chara },
            { "motion", _motionKey },
            { "tickcount", getTickCount() },
            { "speed", _speed },
            { "outline", tTJSVariant(_outline) },
            { "variables", detail::makeDictionary(variables) },
            { "timelines", getPlayingTimelineInfoList() },
        });
    }

    void Player::unserialize(tTJSVariant data) {
        if(data.Type() != tvtObject || data.AsObjectNoAddRef() == nullptr) {
            return;
        }

        tTJSVariant value;
        if(getObjectProperty(data, TJS_W("chara"), value) &&
           value.Type() != tvtVoid) {
            _chara = value;
        }

        if(getObjectProperty(data, TJS_W("motion"), value) &&
           value.Type() != tvtVoid) {
            _motionKey = value;
            ensureMotionLoaded();
        }

        if(getObjectProperty(data, TJS_W("tickcount"), value) &&
           value.Type() != tvtVoid) {
            setTickCount(value.AsReal());
        }

        if(getObjectProperty(data, TJS_W("speed"), value) &&
           value.Type() != tvtVoid) {
            _speed = value.AsReal();
        }

        if(getObjectProperty(data, TJS_W("outline"), value) &&
           value.Type() != tvtVoid) {
            _outline = ttstr(value);
        }

        if(getObjectProperty(data, TJS_W("variables"), value) &&
           value.Type() == tvtObject && value.AsObjectNoAddRef() != nullptr) {
            DictionaryEnumerator callback;
            tTJSVariantClosure closure(&callback, nullptr);
            value.AsObjectNoAddRef()->EnumMembers(TJS_IGNOREPROP, &closure,
                                                  value.AsObjectNoAddRef());
            for(const auto &[label, stored] : callback.entries) {
                if(stored.Type() != tvtVoid) {
                    setVariable(label, stored.AsReal());
                }
            }
        }

        bool restoredTimelines = false;
        if(getObjectProperty(data, TJS_W("timelines"), value) &&
           value.Type() == tvtObject && value.AsObjectNoAddRef() != nullptr) {
            ensureMotionLoaded();
            if(_runtime->activeMotion && _runtime->timelines.empty()) {
                detail::primeTimelineStates(_runtime->timelines,
                                            *_runtime->activeMotion);
            }
            _runtime->playingTimelineLabels.clear();

            const auto count = getObjectCount(value);
            for(tjs_int index = 0; index < count; ++index) {
                tTJSVariant item;
                if(!getArrayItem(value, index, item) ||
                   item.Type() != tvtObject ||
                   item.AsObjectNoAddRef() == nullptr) {
                    continue;
                }

                tTJSVariant labelValue;
                if(!getObjectProperty(item, TJS_W("label"), labelValue) ||
                   labelValue.Type() == tvtVoid) {
                    continue;
                }

                const auto key = detail::narrow(labelValue);
                auto it = _runtime->timelines.find(key);
                if(it == _runtime->timelines.end()) {
                    continue;
                }

                restoredTimelines = true;
                it->second.playing = true;
                _runtime->playingTimelineLabels.push_back(key);
                it->second.controlInitialized = false;
                it->second.controlLastAppliedTime = it->second.currentTime;
                it->second.controlFrameCursor.clear();
                it->second.controlTrackValues.clear();
                it->second.controlTrackAnimators.clear();

                tTJSVariant flagsValue;
                if(getObjectProperty(item, TJS_W("flags"), flagsValue) &&
                   flagsValue.Type() != tvtVoid) {
                    it->second.flags = flagsValue.AsInteger();
                }

                tTJSVariant currentTimeValue;
                if(getObjectProperty(item, TJS_W("currentTime"),
                                     currentTimeValue) &&
                   currentTimeValue.Type() != tvtVoid) {
                    it->second.currentTime = currentTimeValue.AsReal();
                }

                tTJSVariant blendRatioValue;
                if(getObjectProperty(item, TJS_W("blendRatio"),
                                     blendRatioValue) &&
                   blendRatioValue.Type() != tvtVoid) {
                    it->second.blendRatio = blendRatioValue.AsReal();
                }
            }
        }

        if(!restoredTimelines && ensureMotionLoaded()) {
            if(_runtime->timelines.empty()) {
                detail::primeTimelineStates(_runtime->timelines,
                                            *_runtime->activeMotion);
            }
            const auto &primary =
                !_runtime->activeMotion->mainTimelineLabels.empty()
                ? _runtime->activeMotion->mainTimelineLabels
                : _runtime->activeMotion->diffTimelineLabels;
            for(const auto &label : primary) {
                playTimeline(detail::widen(label), PlayFlagForce);
            }
        }

        _allplaying = !_runtime->playingTimelineLabels.empty();
    }

    // Aligned to libkrkr2.so D3DEmotePlayer_setCoord (0x5301EC):
    // store the coord animator payload on Player and keep root x/y in sync.
    void Player::setEmoteCoord(double x, double y, double transition,
                               double ease) {
        _emoteCoordState.x = x;
        _emoteCoordState.y = y;
        _emoteCoordState.transition = transition;
        _emoteCoordState.ease = ease;
        setX(x);
        setY(y);
        _emoteDirty = true;
    }

    // Aligned to libkrkr2.so D3DEmotePlayer_setScale (0x530260):
    // the wrapper multiplies baseScale * userScale, then forwards the final
    // scalar plus transition/ease to the inner Player scale animator.
    void Player::setEmoteScale(double scale, double transition, double ease) {
        (void)transition;
        (void)ease;
        _emoteScaleState.value = scale;
        _emoteScaleState.transition = transition;
        _emoteScaleState.ease = ease;
        // 对齐 setX/setY：脚本 setScale 须写入根节点 delta，否则立绘保持 PSB
        // 1:1 尺寸（在 Layer 上显得过大/过近）。
        if(_runtime && !_runtime->nodes.empty()) {
            auto &root = _runtime->nodes[0];
            if(root.delta.scaleX != scale || root.delta.scaleY != scale) {
                root.delta.scaleX = scale;
                root.delta.scaleY = scale;
                root.delta.dirty = true;
            }
        }
        _emoteDirty = true;
    }

    // Aligned to libkrkr2.so D3DEmotePlayer_setRot (0x5302E4):
    // read player+1161, set player+1162=1, then forward rot/transition/ease
    // to the Player rot animator sink.
    void Player::setRotate(double rot, double transition, double ease) {
        (void)transition;
        (void)ease;
        _rotateAngle = rot;
        _emoteRotState.value = rot;
        _emoteRotState.transition = transition;
        _emoteRotState.ease = ease;
        if(_runtime && !_runtime->nodes.empty()) {
            auto &root = _runtime->nodes[0];
            if(root.delta.angle != rot) {
                root.delta.angle = rot;
                root.delta.dirty = true;
            }
        }
        _emoteDirty = true;
    }

    // Aligned to libkrkr2.so D3DEmotePlayer_setColor (0x530314):
    // unpack AARRGGBB into four float byte values and forward them to the
    // Player color animator sink together with transition/ease.
    void Player::setEmoteColor(tjs_uint32 color, double transition,
                               double ease) {
        _emoteColorState.packed = color;
        _emoteColorState.rgbaBytes[0] =
            static_cast<float>(static_cast<std::uint8_t>(color));
        _emoteColorState.rgbaBytes[1] =
            static_cast<float>(static_cast<std::uint8_t>(color >> 8));
        _emoteColorState.rgbaBytes[2] =
            static_cast<float>(static_cast<std::uint8_t>(color >> 16));
        _emoteColorState.rgbaBytes[3] =
            static_cast<float>(static_cast<std::uint8_t>(color >> 24));
        _emoteColorState.transition = transition;
        _emoteColorState.ease = ease;
        _emoteDirty = true;
    }

    void Player::setMirror(bool mirror) {
        // Aligned to libkrkr2.so Player_setRootFlipX (0x6CD068):
        // update the synthetic root node's flipX flag and mark it dirty.
        if(_rootFlipX == mirror && _mirrorEvalEnabled == mirror) {
            return;
        }

        _rootFlipX = mirror;
        _mirrorEvalEnabled = mirror;
        resetControllerStateLike_0x66EB8C();
    }

    void Player::setEmoteMeshDivisionRatio(double v) {
        _emoteMeshDivisionRatio = v;
        _emoteMeshDivisionRatioDup = v;
    }

    // Aligned to libkrkr2.so:
    // sub_681F20: player+1184 = a2
    void Player::setHairScale(double s) { _hairScale = s; }
    // sub_681F28: player+1192 = a2
    void Player::setPartsScale(double s) { _partsScale = s; }
    // sub_681F30: player+1200 = a2
    void Player::setBustScale(double s) { _bustScale = s; }

    // Aligned to D3DEmotePlayer_startWind (0x530680) -> Player_startWind
    // (0x6709AC): normalize amplitude, optionally destroy/rebuild wind
    // simulator state, then store min/max/amplitude/freq and reset the active
    // counter.
    void Player::startWind(double minAngle, double maxAngle, double amplitude,
                           double freqX, double freqY) {
        const double absAmplitude = std::abs(amplitude);
        const double normalizedMin = amplitude >= 0.0 ? minAngle : maxAngle;
        const double normalizedMax = amplitude >= 0.0 ? maxAngle : minAngle;

        if(absAmplitude == 0.0 || normalizedMin == normalizedMax ||
           (freqX == 0.0 && freqY == 0.0)) {
            stopWind();
            return;
        }

        const bool rebuild = !_windState.active ||
            _windState.minAngle != normalizedMin ||
            _windState.maxAngle != normalizedMax;
        if(rebuild) {
            _windState = {};
            _windState.active = true;
        }

        _windState.active = true;
        _windState.minAngle = normalizedMin;
        _windState.maxAngle = normalizedMax;
        _windState.amplitude = absAmplitude;
        _windState.freqX = freqX;
        _windState.freqY = freqY;
        const double direction =
            _windState.prevPhase > _windState.phase ? -1.0 : 1.0;
        const double ratio =
            _emoteMeshDivisionRatio != 0.0 ? _emoteMeshDivisionRatio : 1.0;
        _windState.scaledAmplitude = direction * (absAmplitude / ratio);
        _windState.counter = 0;
        _emoteDirty = true;
    }

    // Aligned to sub_681A38: delete wind simulator and clear player+1128.
    void Player::stopWind() {
        _windState = {};
        _emoteDirty = true;
    }

    // Aligned to D3DEmotePlayer_setOuterForce (0x530A8C) ->
    // Player_setOuterForce (0x672D58): case-insensitive label dispatch for
    // "bust", "h", and "parts", carrying transition/ease through the sink.
    void Player::setOuterForce(ttstr label, double x, double y,
                               double transition, double ease) {
        const auto key = lowerAscii(detail::narrow(label));
        OuterForceState *target = nullptr;
        if(key == "bust") {
            target = &_bustOuterForce;
        } else if(key == "h") {
            target = &_hairOuterForce;
        } else if(key == "parts") {
            target = &_partsOuterForce;
        } else {
            return;
        }

        target->active = true;
        target->x = x;
        target->y = y;
        target->transition = transition;
        target->ease = ease;
        _emoteDirty = true;
    }

    tTJSVariant Player::getOuterForce(ttstr label) {
        const auto key = lowerAscii(detail::narrow(label));
        const OuterForceState *target = nullptr;
        if(key == "bust") {
            target = &_bustOuterForce;
        } else if(key == "h") {
            target = &_hairOuterForce;
        } else if(key == "parts") {
            target = &_partsOuterForce;
        } else {
            return tTJSVariant();
        }

        return detail::makeDictionary({
            { "x", target->x },
            { "y", target->y },
        });
    }

    // Aligned to libkrkr2.so sub_681EF8 at 0x681EF8:
    // Stores translate (x,y) to runtime+144/148 (cameraOffsetX/Y).
    // The full 6-param matrix version is handled by
    // setDrawAffineTranslateMatrixCompat.
    void Player::setDrawAffineTranslateMatrix(tTJSVariant) {
        // Single-param variant: compat handler does the real work via
        // NCB_METHOD_RAW
    }

    tTJSVariant Player::getCameraOffset() { return _cameraPosition; }

    void Player::setCameraOffset(tTJSVariant offset) {
        _cameraPosition = offset;
        // Aligned to libkrkr2.so sub_6D9A38: setCameraOffset(x, y)
        // Stores as float at Player+144/148. NCB passes a Point with x,y.
        if(offset.Type() == tvtObject) {
            auto *obj = offset.AsObjectNoAddRef();
            if(obj) {
                tTJSVariant xv, yv;
                if(obj->PropGet(0, TJS_W("x"), nullptr, &xv, obj) == TJS_S_OK)
                    _cameraOffsetX = static_cast<float>(xv.AsReal());
                if(obj->PropGet(0, TJS_W("y"), nullptr, &yv, obj) == TJS_S_OK)
                    _cameraOffsetY = static_cast<float>(yv.AsReal());
            }
        }
    }

    void Player::modifyRoot(tTJSVariant data) { _project = data; }

    void Player::debugPrint() {
        G_PluginLog.info(
            "motionKey={}, motions={}, sources={}, timelines={}",
            _motionKey.AsStdString(), _runtime->motionsByKey.size(),
            _runtime->sourceCacheNative ? _runtime->sourceCacheNative->size()
                                        : 0,
            _runtime->timelines.size());
    }


} // namespace motion
