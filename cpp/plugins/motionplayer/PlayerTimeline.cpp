// PlayerTimeline.cpp — timeline queries and playback raw callbacks
// Split out for maintainability.
//
#include <algorithm>

#include "PlayerInternal.h"
#include "ncbind.hpp"

using namespace motion::internal;

namespace motion {
    void Player::skipToSync() {
        ensureMotionLoaded();
        if(!_runtime || !_runtime->activeMotion) {
            G_PluginLog.warn("Player::skipToSync(): no active motion");
            return;
        }

        // 参考 sdl3/emoteplayerclass.cpp（不编译）: skipToSync / getSyncTime
        // metadata because libkrkr2.so stores it separately from timeline end.
        double syncTime = 0.0;
        for(const auto &[label, binding] :
            _runtime->activeMotion->timelineControlByLabel) {
            syncTime = std::max(syncTime, binding.lastTime);
            if(binding.loopEnd >= binding.loopBegin) {
                syncTime = std::max(syncTime, binding.loopEnd);
            }
            (void)label;
        }
        if(const auto *clip = selectActiveClip()) {
            syncTime = std::max(syncTime, clip->totalFrames);
        }
        G_PluginLog.debug(
            "Player::skipToSync(): seeking timelines to syncTime={:.3f} "
            "(参考 sdl3 getSyncTime；libkrkr2.so 可能不同)",
            syncTime);

        for(auto &[_, state] : _runtime->timelines) {
            state.currentTime = syncTime;
            if(state.totalFrames > 0.0 &&
               state.currentTime >= state.totalFrames && !state.loop) {
                state.playing = false;
            }
        }
        if(const auto it =
               std::remove_if(_runtime->playingTimelineLabels.begin(),
                              _runtime->playingTimelineLabels.end(),
                              [this](const std::string &label) {
                                  const auto found =
                                      _runtime->timelines.find(label);
                                  return found == _runtime->timelines.end() ||
                                      !found->second.playing;
                              });
           it != _runtime->playingTimelineLabels.end()) {
            _runtime->playingTimelineLabels.erase(
                it, _runtime->playingTimelineLabels.end());
        }
        _frameLoopTime = syncTime;
        _clampedEvalTime = syncTime;
        _syncWaiting = false;
        _syncActive = false;
        _allplaying = !_runtime->playingTimelineLabels.empty();
    }

    bool Player::getTimelinePlaying(ttstr label) {
        ensureMotionLoaded();
        const auto key = detail::narrow(label);
        if(const auto it = _runtime->timelines.find(key);
           it != _runtime->timelines.end()) {
            return it->second.playing;
        }
        return false;
    }

    tjs_int Player::countMainTimelines() {
        ensureMotionLoaded();
        return _runtime->activeMotion
            ? static_cast<tjs_int>(
                  _runtime->activeMotion->mainTimelineLabels.size())
            : 0;
    }

    ttstr Player::getMainTimelineLabelAt(tjs_int idx) {
        ensureMotionLoaded();
        if(!_runtime->activeMotion || idx < 0 ||
           static_cast<size_t>(idx) >=
               _runtime->activeMotion->mainTimelineLabels.size()) {
            return {};
        }
        return detail::widen(_runtime->activeMotion->mainTimelineLabels[idx]);
    }

    tTJSVariant Player::getMainTimelineLabelList() {
        ensureMotionLoaded();
        if(!_runtime->activeMotion) {
            return detail::makeArray({});
        }
        return detail::makeArray(detail::stringsToVariants(
            _runtime->activeMotion->mainTimelineLabels));
    }

    tjs_int Player::countDiffTimelines() {
        ensureMotionLoaded();
        return _runtime->activeMotion
            ? static_cast<tjs_int>(
                  _runtime->activeMotion->diffTimelineLabels.size())
            : 0;
    }

    ttstr Player::getDiffTimelineLabelAt(tjs_int idx) {
        ensureMotionLoaded();
        if(!_runtime->activeMotion || idx < 0 ||
           static_cast<size_t>(idx) >=
               _runtime->activeMotion->diffTimelineLabels.size()) {
            return {};
        }
        return detail::widen(_runtime->activeMotion->diffTimelineLabels[idx]);
    }

    tTJSVariant Player::getDiffTimelineLabelList() {
        ensureMotionLoaded();
        if(!_runtime->activeMotion) {
            return detail::makeArray({});
        }
        return detail::makeArray(detail::stringsToVariants(
            _runtime->activeMotion->diffTimelineLabels));
    }

    bool Player::getLoopTimeline(ttstr label) {
        ensureMotionLoaded();
        if(!_runtime->activeMotion) {
            return false;
        }
        const auto key = detail::narrow(label);
        if(const auto controlIt =
               _runtime->activeMotion->timelineControlByLabel.find(key);
           controlIt != _runtime->activeMotion->timelineControlByLabel.end()) {
            // 参考 sdl3/emoteplayerclass.cpp（不编译）: getLoopTimeline()
            // returns timelineControl.lastTime < 0.
            return controlIt->second.lastTime < 0.0;
        }
        if(const auto it = _runtime->activeMotion->loopTimelines.find(key);
           it != _runtime->activeMotion->loopTimelines.end()) {
            return it->second;
        }
        return false;
    }

    tjs_int Player::countPlayingTimelines() {
        ensureMotionLoaded();
        return static_cast<tjs_int>(_runtime->playingTimelineLabels.size());
    }

    ttstr Player::getPlayingTimelineLabelAt(tjs_int idx) {
        ensureMotionLoaded();
        if(idx >= 0 &&
           static_cast<size_t>(idx) < _runtime->playingTimelineLabels.size()) {
            return detail::widen(_runtime->playingTimelineLabels[idx]);
        }
        return {};
    }

    tjs_int Player::getPlayingTimelineFlagsAt(tjs_int idx) {
        ensureMotionLoaded();
        if(idx >= 0 &&
           static_cast<size_t>(idx) < _runtime->playingTimelineLabels.size()) {
            const auto &label = _runtime->playingTimelineLabels[idx];
            if(const auto it = _runtime->timelines.find(label);
               it != _runtime->timelines.end()) {
                return it->second.flags;
            }
        }
        return 0;
    }

    tjs_int Player::getTimelineTotalFrameCount(ttstr label) {
        ensureMotionLoaded();
        const auto key = detail::narrow(label);
        if(_runtime->activeMotion) {
            if(const auto controlIt =
                   _runtime->activeMotion->timelineControlByLabel.find(key);
               controlIt !=
               _runtime->activeMotion->timelineControlByLabel.end()) {
                const auto &binding = controlIt->second;
                if(binding.loopEnd >= binding.loopBegin) {
                    // 参考 sdl3: loopEnd - loopBegin + 1
                    return static_cast<tjs_int>(binding.loopEnd -
                                                binding.loopBegin + 1.0);
                }
            }
        }
        if(const auto it = _runtime->timelines.find(key);
           it != _runtime->timelines.end()) {
            return static_cast<tjs_int>(it->second.totalFrames);
        }
        if(_runtime->activeMotion) {
            if(const auto it =
                   _runtime->activeMotion->timelineTotalFrames.find(key);
               it != _runtime->activeMotion->timelineTotalFrames.end()) {
                return static_cast<tjs_int>(it->second);
            }
        }
        return 0;
    }

    void Player::playTimeline(ttstr label, tjs_int flags) {
        ensureMotionLoaded();
        if(!_runtime->activeMotion) {
            return;
        }
        if(_runtime->timelines.empty()) {
            detail::primeTimelineStates(_runtime->timelines,
                                        *_runtime->activeMotion);
        }

        const auto key = detail::narrow(label);
        auto it = _runtime->timelines.find(key);
        if(it == _runtime->timelines.end()) {
            return;
        }

        // Aligned to libkrkr2.so Player_playTimeline (0x672F70):
        // parallel flag first clears the playing-timeline list.
        if((flags & 1) != 0) {
            stopTimeline(TJS_W(""));
        }

        if(!label.IsEmpty()) {
            if(std::find(_runtime->playingTimelineLabels.begin(),
                         _runtime->playingTimelineLabels.end(),
                         key) == _runtime->playingTimelineLabels.end()) {
                _runtime->playingTimelineLabels.push_back(key);
            }
        }

        it->second.flags = flags;
        it->second.playing = true;
        it->second.currentTime = 0.0;
        it->second.blendRatio = 1.0;
        it->second.blendAnimator = {};
        it->second.blendAutoStop = false;
        it->second.controlInitialized = false;
        it->second.controlLastAppliedTime = 0.0;
        it->second.controlFrameCursor.clear();
        it->second.controlTrackValues.clear();
        it->second.controlTrackAnimators.clear();
        if(const auto controlIt =
               _runtime->activeMotion->timelineControlByLabel.find(key);
           controlIt != _runtime->activeMotion->timelineControlByLabel.end()) {
            resetTimelineControlStateLike_0x671A50(it->second,
                                                   controlIt->second, 0.0);
        }
        _allplaying = !_runtime->playingTimelineLabels.empty();
    }

    void Player::stopTimeline(ttstr label) {
        const auto key = detail::narrow(label);
        if(label.IsEmpty()) {
            for(auto &[_, state] : _runtime->timelines) {
                state.playing = false;
                state.blendRatio = 1.0;
                state.blendAnimator = {};
                state.blendAutoStop = false;
                state.controlInitialized = false;
                state.controlFrameCursor.clear();
                state.controlTrackValues.clear();
                state.controlTrackAnimators.clear();
            }
            _runtime->playingTimelineLabels.clear();
        } else {
            if(const auto it = _runtime->timelines.find(key);
               it != _runtime->timelines.end()) {
                it->second.playing = false;
                it->second.blendRatio = 1.0;
                it->second.blendAnimator = {};
                it->second.blendAutoStop = false;
                it->second.controlInitialized = false;
                it->second.controlFrameCursor.clear();
                it->second.controlTrackValues.clear();
                it->second.controlTrackAnimators.clear();
            }
            if(const auto it =
                   std::remove(_runtime->playingTimelineLabels.begin(),
                               _runtime->playingTimelineLabels.end(), key);
               it != _runtime->playingTimelineLabels.end()) {
                _runtime->playingTimelineLabels.erase(
                    it, _runtime->playingTimelineLabels.end());
            }
        }

        _allplaying = !_runtime->playingTimelineLabels.empty();
    }

    void Player::setTimelineBlendRatio(ttstr label, double ratio) {
        ensureMotionLoaded();
        if(_runtime->timelines.empty() && _runtime->activeMotion) {
            detail::primeTimelineStates(_runtime->timelines,
                                        *_runtime->activeMotion);
        }

        const auto key = detail::narrow(label);
        auto &state = _runtime->timelines[key];
        state.label = key;
        state.blendRatio = ratio;
        state.blendAnimator = {};
        state.blendAutoStop = false;
    }

    double Player::getTimelineBlendRatio(ttstr label) {
        const auto key = detail::narrow(label);
        if(const auto it = _runtime->timelines.find(key);
           it != _runtime->timelines.end()) {
            return it->second.blendRatio;
        }
        return 1.0;
    }

    void Player::fadeInTimeline(ttstr label, double duration, tjs_int flags) {
        const auto key = detail::narrow(label);
        const bool alreadyPlaying =
            std::find(_runtime->playingTimelineLabels.begin(),
                      _runtime->playingTimelineLabels.end(),
                      key) != _runtime->playingTimelineLabels.end();
        if(!alreadyPlaying) {
            playTimeline(label, 3);
            setTimelineBlendLike_0x6735AC(key, false, 0.0, 0.0, 0.0);
        }
        setTimelineBlendLike_0x6735AC(key, false, 1.0, duration, 0.0);
    }

    void Player::fadeOutTimeline(ttstr label, double duration, tjs_int) {
        setTimelineBlendLike_0x6735AC(detail::narrow(label), true, 0.0,
                                      duration, 0.0);
    }

    tTJSVariant Player::getPlayingTimelineInfoList() {
        ensureMotionLoaded();
        return detail::makeArray(timelineInfoVariants(*_runtime));
    }

    bool Player::playMotionLike_0x6B2284(ttstr label, tjs_int flags) {
        if(!_runtime->activeMotion && _project.Type() == tvtObject) {
            if(const auto snapshot = detail::lookupModuleSnapshot(_project)) {
                activateMotion(*_runtime, snapshot);
                syncVariableKeysFromActiveMotion();
            }
        }

        auto commitRequestedMotionLike_0x6B2380 = [&]() {
            if(label.IsEmpty() || !_runtime->activeMotion) {
                return;
            }
            if((flags & PlayFlagStealth) != 0) {
                _stealthMotion = label;
                return;
            }
            // Player_playImpl @ 0x6B2284 writes player+976 before
            // Player_initNonEmoteMotion @ 0x6B365C. Player_getMotion_ncb
            // @ 0x6D9544 then exposes that same slot to TJS.
            _motionKey = label;
        };

        ensureMotionLoaded();
        commitRequestedMotionLike_0x6B2380();
        initNonEmoteMotionLike_0x6B365C(static_cast<std::uint32_t>(flags));
        if(_runtime->activeMotion && _runtime->timelines.empty()) {
            detail::primeTimelineStates(_runtime->timelines,
                                        *_runtime->activeMotion);
        }

        if(!label.IsEmpty() && !_runtime->activeMotion) {
            setMotion(label);
            ensureMotionLoaded();
            commitRequestedMotionLike_0x6B2380();
            initNonEmoteMotionLike_0x6B365C(static_cast<std::uint32_t>(flags));
            if(_runtime->activeMotion && _runtime->timelines.empty()) {
                detail::primeTimelineStates(_runtime->timelines,
                                            *_runtime->activeMotion);
            }
        }

        if(!_runtime->activeMotion) {
            return false;
        }

        if((flags & PlayFlagForce) != 0) {
            stopTimeline(TJS_W(""));
        }

        const bool chainMode = (flags & PlayFlagChain) != 0;
        const auto playOne = [&](const std::string &timelineLabel) {
            auto &state = _runtime->timelines[timelineLabel];
            state.label = timelineLabel;
            state.flags = flags;
            state.blendRatio = 1.0;
            state.playing = true;
            if(!chainMode) {
                state.currentTime = 0.0;
                state.controlInitialized = false;
                state.controlLastAppliedTime = 0.0;
                state.controlFrameCursor.clear();
                state.controlTrackValues.clear();
                state.controlTrackAnimators.clear();
            }
            if(std::find(_runtime->playingTimelineLabels.begin(),
                         _runtime->playingTimelineLabels.end(),
                         timelineLabel) ==
               _runtime->playingTimelineLabels.end()) {
                _runtime->playingTimelineLabels.push_back(timelineLabel);
            }
            if(state.totalFrames <= 0.0 && _runtime->activeMotion) {
                const auto it =
                    _runtime->activeMotion->timelineTotalFrames.find(
                        timelineLabel);
                if(it != _runtime->activeMotion->timelineTotalFrames.end()) {
                    state.totalFrames = it->second;
                }
            }
        };

        bool started = false;
        if(!label.IsEmpty()) {
            const auto key = detail::narrow(label);
            if(_runtime->timelines.find(key) != _runtime->timelines.end()) {
                playOne(key);
                started = true;
            }
        }

        if(!started) {
            const auto &primary =
                !_runtime->activeMotion->mainTimelineLabels.empty()
                ? _runtime->activeMotion->mainTimelineLabels
                : _runtime->activeMotion->diffTimelineLabels;
            for(const auto &timelineLabel : primary) {
                playOne(timelineLabel);
                started = true;
            }
        }

        _allplaying = !_runtime->playingTimelineLabels.empty();
        return started;
    }

    tjs_error Player::playCompat(tTJSVariant *result, tjs_int numparams,
                                 tTJSVariant **param, iTJSDispatch2 *objthis) {
        if(result) {
            result->Clear();
        }

        auto *self =
            ncbInstanceAdaptor<Player>::GetNativeInstance(objthis, true);
        if(!self) {
            return TJS_E_INVALIDOBJECT;
        }

        ttstr label;
        tjs_int flags = 0;
        if(numparams > 0 && param[0] && param[0]->Type() != tvtVoid) {
            if(param[0]->Type() == tvtInteger || param[0]->Type() == tvtReal) {
                flags = param[0]->AsInteger();
            } else {
                label = *param[0];
            }
        }
        if(numparams > 1 && param[1] && param[1]->Type() != tvtVoid) {
            flags = param[1]->AsInteger();
        }

        if(!self->_runtime->activeMotion &&
           self->_project.Type() == tvtObject) {
            if(const auto snapshot =
                   detail::lookupModuleSnapshot(self->_project)) {
                activateMotion(*self->_runtime, snapshot);
                self->syncVariableKeysFromActiveMotion();
            }
        }

        auto commitRequestedMotionLike_0x6B2380 = [&]() {
            if(label.IsEmpty() || !self->_runtime->activeMotion) {
                return;
            }
            if((flags & PlayFlagStealth) != 0) {
                self->_stealthMotion = label;
                return;
            }
            // Player_playImpl @ 0x6B2284 writes player+976 before
            // Player_initNonEmoteMotion @ 0x6B365C. Player_getMotion_ncb
            // @ 0x6D9544 then exposes that same slot to TJS.
            self->_motionKey = label;
        };

        // Aligned to libkrkr2.so Player_playImpl (0x6B2284) ->
        // Player_initNonEmoteMotion (0x6B365C): after loadMotion, the binary
        // synchronously calls Player_buildNodeTree (0x6B51F0) and
        // Player_initVariables (0x6CD750) before setting any playing state.
        // No lazy gate exists in the binary.
        self->ensureMotionLoaded();
        commitRequestedMotionLike_0x6B2380();
        self->initNonEmoteMotionLike_0x6B365C(
            static_cast<std::uint32_t>(flags));
        if(self->_runtime->activeMotion && self->_runtime->timelines.empty()) {
            detail::primeTimelineStates(self->_runtime->timelines,
                                        *self->_runtime->activeMotion);
        }

        if(!label.IsEmpty() && !self->_runtime->activeMotion) {
            self->setMotion(label);
            self->ensureMotionLoaded();
            commitRequestedMotionLike_0x6B2380();
            self->initNonEmoteMotionLike_0x6B365C(
                static_cast<std::uint32_t>(flags));
            if(self->_runtime->activeMotion &&
               self->_runtime->timelines.empty()) {
                detail::primeTimelineStates(self->_runtime->timelines,
                                            *self->_runtime->activeMotion);
            }
        }

        if(!self->_runtime->activeMotion) {
            if(result) {
                *result = tTJSVariant(false);
            }
            return TJS_S_OK;
        }

        if((flags & PlayFlagForce) != 0) {
            self->stopTimeline(TJS_W(""));
        }

        // Aligned to libkrkr2.so Player_initNonEmoteMotion (0x6B3A8C):
        //   if ((flags & 2) == 0) {                // non-Chain
        //       Player+456 = fmin(Player+1128, 0); // reset lastTime
        //       Player+1120 = 0;                   // reset time counter
        //       Player+480  = 257;                 // playing state
        //   }
        // Chain mode (bit 1) preserves the prior play position; non-Chain
        // resets time to the motion's origin. At the per-timeline level we
        // mirror this by gating currentTime / control cursor reset on
        // non-Chain. Label/flags/blendRatio/playing stay unconditional
        // because the binary always stores the new motion state.
        const bool chainMode = (flags & PlayFlagChain) != 0;
        const auto playOne = [&](const std::string &timelineLabel) {
            auto &state = self->_runtime->timelines[timelineLabel];
            state.label = timelineLabel;
            state.flags = flags;
            state.blendRatio = 1.0;
            state.playing = true;
            if(!chainMode) {
                state.currentTime = 0.0;
                state.controlInitialized = false;
                state.controlLastAppliedTime = 0.0;
                state.controlFrameCursor.clear();
                state.controlTrackValues.clear();
                state.controlTrackAnimators.clear();
            }
            if(std::find(self->_runtime->playingTimelineLabels.begin(),
                         self->_runtime->playingTimelineLabels.end(),
                         timelineLabel) ==
               self->_runtime->playingTimelineLabels.end()) {
                self->_runtime->playingTimelineLabels.push_back(timelineLabel);
            }
            // Ensure totalFrames is set (may be 0 if timeline wasn't primed)
            if(state.totalFrames <= 0.0 && self->_runtime->activeMotion) {
                auto it =
                    self->_runtime->activeMotion->timelineTotalFrames.find(
                        timelineLabel);
                if(it !=
                   self->_runtime->activeMotion->timelineTotalFrames.end()) {
                    state.totalFrames = it->second;
                }
            }
        };

        bool started = false;
        if(!label.IsEmpty()) {
            const auto key = detail::narrow(label);
            if(self->_runtime->timelines.find(key) !=
               self->_runtime->timelines.end()) {
                playOne(key);
                started = true;
            }
        }

        if(!started) {
            const auto &primary =
                !self->_runtime->activeMotion->mainTimelineLabels.empty()
                ? self->_runtime->activeMotion->mainTimelineLabels
                : self->_runtime->activeMotion->diffTimelineLabels;
            for(const auto &timelineLabel : primary) {
                playOne(timelineLabel);
                started = true;
            }
        }

        self->_allplaying = !self->_runtime->playingTimelineLabels.empty();

        if(self->_runtime->activeMotion &&
           detail::logoChainTraceEnabled(self->_runtime->activeMotion)) {
            std::string playingLabels;
            for(const auto &timelineLabel :
                self->_runtime->playingTimelineLabels) {
                if(!playingLabels.empty()) {
                    playingLabels += ",";
                }
                playingLabels += timelineLabel;
            }
            detail::logoChainTraceLogf(
                self->_runtime->activeMotion->path, "playCompat", "0x6B2284",
                self->_clampedEvalTime,
                "label={} flags={} started={} timelineCount={} "
                "playingLabels={} allplaying={}",
                detail::narrow(label), flags, started ? 1 : 0,
                self->_runtime->timelines.size(),
                playingLabels.empty() ? std::string("<none>") : playingLabels,
                self->_allplaying ? 1 : 0);
        }

        if(result) {
            *result = tTJSVariant(started);
        }
        return TJS_S_OK;
    }

    tjs_error Player::isPlayingCompat(tTJSVariant *result, tjs_int,
                                      tTJSVariant **, iTJSDispatch2 *objthis) {
        auto *self =
            ncbInstanceAdaptor<Player>::GetNativeInstance(objthis, true);
        if(!self) {
            return TJS_E_INVALIDOBJECT;
        }

        const bool playing = !self->_runtime->playingTimelineLabels.empty();
        self->_allplaying = playing;
        if(result) {
            *result = tTJSVariant(playing);
        }
        return TJS_S_OK;
    }

    tjs_error Player::stopCompat(tTJSVariant *result, tjs_int numparams,
                                 tTJSVariant **param, iTJSDispatch2 *objthis) {
        auto *self =
            ncbInstanceAdaptor<Player>::GetNativeInstance(objthis, true);
        if(!self) {
            return TJS_E_INVALIDOBJECT;
        }

        // Aligned to libkrkr2.so Player_stop (0x6D9A30):
        // Binary simply clears the Player-level playing flag (player+1099).
        // Timeline state is left intact; TJS polls `playing` for edge-triggered
        // stop detection and may still inspect the final motion pose afterward.
        self->_allplaying = false;

        if(result) {
            *result = tTJSVariant(true);
        }
        return TJS_S_OK;
    }

} // namespace motion
