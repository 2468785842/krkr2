//
// Internal helpers for motionplayer/emoteplayer runtime state.
//

#include "RuntimeSupport.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <iterator>
#include <mutex>
#include <optional>
#include <sstream>
#include <unordered_set>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#include "StorageIntf.h"
#include "psbfile/PSBMediaRegistry.h"
#include "tjsArray.h"
#include "tjsDictionary.h"

#include "log/TVPLog.h"

namespace motion::detail {

    namespace {

        std::mutex &snapshotRegistryMutex() {
            static std::mutex mutex;
            return mutex;
        }

        std::unordered_map<const iTJSDispatch2 *,
                           std::shared_ptr<MotionSnapshot>> &
        snapshotRegistry() {
            static std::unordered_map<const iTJSDispatch2 *,
                                      std::shared_ptr<MotionSnapshot>>
                registry;
            return registry;
        }

        struct LogoChainTraceSession {
            std::uint64_t sequence = 0;
            std::string motionPath;
            std::string motionName;
            std::string firstBadStage;
            std::string firstBadExpected;
            std::string firstBadActual;
            std::string upstreamLastGoodStage;
            std::string likelyRootCause;
            bool summaryEmitted = false;
        };

        std::mutex &logoTraceMutex() {
            static std::mutex mutex;
            return mutex;
        }

        std::unordered_map<std::string, LogoChainTraceSession> &
        logoTraceSessions() {
            static std::unordered_map<std::string, LogoChainTraceSession>
                sessions;
            return sessions;
        }

        std::string lowercase(std::string value) {
            std::transform(value.begin(), value.end(), value.begin(),
                           [](unsigned char ch) {
                               return static_cast<char>(std::tolower(ch));
                           });
            return value;
        }

        bool pathContainsToken(const std::vector<std::string> &path,
                               const std::string &token) {
            const auto loweredToken = lowercase(token);
            return std::any_of(path.begin(), path.end(),
                               [&loweredToken](const std::string &part) {
                                   return lowercase(part).find(loweredToken) !=
                                       std::string::npos;
                               });
        }

        bool hasExtension(const std::string &value) {
            return value.find('.') != std::string::npos;
        }

        bool hasSuffix(const std::string &value, const char *suffix) {
            const auto suffixLen = std::strlen(suffix);
            return value.size() >= suffixLen &&
                value.compare(value.size() - suffixLen, suffixLen, suffix) == 0;
        }

        std::string basename(const std::string &value) {
            const auto slash = value.find_last_of("/\\");
            return slash == std::string::npos ? value : value.substr(slash + 1);
        }

        bool isTargetLogoMotionPath(const std::string &motionPath) {
            const auto lowered = lowercase(motionPath);
            return lowered.find("yuzulogo.mtn") != std::string::npos ||
                lowered.find("m2logo.mtn") != std::string::npos;
        }

        bool logoTraceQueryEnabled() {
#ifdef EMSCRIPTEN
            return EM_ASM_INT({
                       try {
                           if(typeof window != = 'undefined' &&
                                  window.__KRKR_TRACE_LOGO_CHAIN__) {
                               return 1;
                           }
                           const params =
                               new URLSearchParams(window.location.search);
                           const traceParam = params.get('trace') || "";
                           if(params.has('traceLogoChain')) {
                               return 1;
                           }
                           return traceParam == = 'logo' || traceParam ==
                               = 'logo-chain' || traceParam == = '1';
                       } catch(e) {
                           return 0;
                       }
                   }) != 0;
#else
            // libkrkr2.so (Android original) has no logo chain trace feature.
            // Verified via IDA Pro MCP:
            //   - No "tracelogochain" / "snaplogo" / "logoChain*" strings in
            //     either UTF-8 or UTF-16LE encoding (ida-search-string skill
            //     scan across all segments).
            //   - EmoteObject_init at 0x67DBAC (the PSB load entry) contains
            //     zero spdlog/LOGGER calls and zero conditional-trace branches
            //     in its full 1632-byte body.
            //   - libkrkr2.so's only command-line query helper is sub_90DA50
            //     (the equivalent of the named-arg TVPGetCommandLine). The
            //     string pool contains -forcelog / -lowpri / -laxtimer as
            //     query targets, but not -tracelogochain, so no function in
            //     libkrkr2.so ever issues a sub_90DA50(L"-tracelogochain", _)
            //     call. Introducing one here would add a call-site that does
            //     not exist in the original binary.
            //
            // The whole logoChainTrace* subsystem (added in commit 0830b84)
            // is a pure-logging local debug path, preserved on the EMSCRIPTEN
            // side only. For non-EMSCRIPTEN builds the aligned behavior is to
            // never enable it.
            return false;
#endif
        }

        bool logoSnapshotQueryEnabled() {
#ifdef EMSCRIPTEN
            return EM_ASM_INT({
                       try {
                           const params =
                               new URLSearchParams(window.location.search);
                           const snapParam = params.get('snap') || "";
                           const traceParam = params.get('trace') || "";
                           return snapParam == = '1' || snapParam ==
                               = 'logo' || traceParam ==
                               = 'snap' || traceParam == = 'logo-snap';
                       } catch(e) {
                           return 0;
                       }
                   }) != 0;
#else
            // Same rationale as logoTraceQueryEnabled above: verified absent
            // from libkrkr2.so, non-EMSCRIPTEN builds stay aligned by never
            // enabling the snapshot feature.
            return false;
#endif
        }

        LogoChainTraceSession &
        ensureLogoTraceSessionLocked(const std::string &motionPath) {
            auto &session = logoTraceSessions()[lowercase(motionPath)];
            if(session.motionPath != motionPath) {
                session = {};
                session.motionPath = motionPath;
                session.motionName = basename(motionPath);
            }
            if(session.motionName.empty()) {
                session.motionName = basename(motionPath);
            }
            return session;
        }

        std::string frameLabel(double frameTime) {
            return std::isfinite(frameTime) ? fmt::format("{:.3f}", frameTime)
                                            : "n/a";
        }

        bool looksLikeStoragePath(const std::string &value) {
            const auto lowered = lowercase(value);
            static const char *exts[] = { ".psb",  ".pimg", ".png", ".jpg",
                                          ".jpeg", ".bmp",  ".tlg", ".webp" };
            return std::any_of(std::begin(exts), std::end(exts),
                               [&lowered](const char *ext) {
                                   return hasSuffix(lowered, ext);
                               });
        }

        std::optional<std::string>
        psbString(const std::shared_ptr<PSB::IPSBValue> &value) {
            if(auto str = std::dynamic_pointer_cast<PSB::PSBString>(value)) {
                return str->value;
            }
            return std::nullopt;
        }

        std::optional<double>
        psbNumber(const std::shared_ptr<PSB::IPSBValue> &value) {
            if(auto number = std::dynamic_pointer_cast<PSB::PSBNumber>(value)) {
                switch(number->numberType) {
                    case PSB::PSBNumberType::Float:
                        return number->getValue<float>();
                    case PSB::PSBNumberType::Double:
                        return number->getValue<double>();
                    case PSB::PSBNumberType::Int:
                        return static_cast<double>(number->getValue<int>());
                    case PSB::PSBNumberType::Long:
                    default:
                        return static_cast<double>(
                            number->getValue<tjs_int64>());
                }
            }
            if(auto boolean = std::dynamic_pointer_cast<PSB::PSBBool>(value)) {
                return boolean->value ? 1.0 : 0.0;
            }
            return std::nullopt;
        }

        std::optional<bool>
        psbBool(const std::shared_ptr<PSB::IPSBValue> &value) {
            if(auto boolean = std::dynamic_pointer_cast<PSB::PSBBool>(value)) {
                return boolean->value;
            }
            if(auto number = psbNumber(value)) {
                return *number != 0.0;
            }
            return std::nullopt;
        }

        std::optional<std::string>
        dictionaryString(const std::shared_ptr<const PSB::PSBDictionary> &dic,
                         const std::vector<std::string> &keys) {
            for(const auto &key : keys) {
                if(const auto value = (*dic)[key]) {
                    if(const auto result = psbString(value)) {
                        return result;
                    }
                }
            }
            return std::nullopt;
        }

        std::optional<double>
        dictionaryNumber(const std::shared_ptr<const PSB::PSBDictionary> &dic,
                         const std::vector<std::string> &keys) {
            for(const auto &key : keys) {
                if(const auto value = (*dic)[key]) {
                    if(const auto result = psbNumber(value)) {
                        return result;
                    }
                }
            }
            return std::nullopt;
        }

        std::optional<bool>
        dictionaryBool(const std::shared_ptr<const PSB::PSBDictionary> &dic,
                       const std::vector<std::string> &keys) {
            for(const auto &key : keys) {
                if(const auto value = (*dic)[key]) {
                    if(const auto result = psbBool(value)) {
                        return result;
                    }
                }
            }
            return std::nullopt;
        }

        std::shared_ptr<PSB::PSBList>
        dictionaryList(const std::shared_ptr<const PSB::PSBDictionary> &dic,
                       const std::vector<std::string> &keys) {
            for(const auto &key : keys) {
                if(auto value =
                       std::dynamic_pointer_cast<PSB::PSBList>((*dic)[key])) {
                    return value;
                }
            }
            return nullptr;
        }

        bool
        dictionaryHasKey(const std::shared_ptr<const PSB::PSBDictionary> &dic,
                         const std::string &key) {
            return (*dic)[key] != nullptr;
        }

        bool dictionaryKeyContains(
            const std::shared_ptr<const PSB::PSBDictionary> &dic,
            const std::string &token) {
            const auto loweredToken = lowercase(token);
            return std::any_of(
                dic->begin(), dic->end(), [&loweredToken](const auto &entry) {
                    return lowercase(entry.first).find(loweredToken) !=
                        std::string::npos;
                });
        }

        std::shared_ptr<const PSB::PSBDictionary> navigateDictionaryPath(
            const std::shared_ptr<const PSB::PSBDictionary> &root,
            const std::string &path) {
            if(!root || path.empty()) {
                return nullptr;
            }

            auto node = root;
            std::istringstream stream(path);
            std::string segment;
            while(std::getline(stream, segment, '/')) {
                if(segment.empty() || !node) {
                    continue;
                }
                node = std::dynamic_pointer_cast<const PSB::PSBDictionary>(
                    (*node)[segment]);
                if(!node) {
                    return nullptr;
                }
            }
            return node;
        }

        std::string joinStrings(const std::vector<std::string> &values,
                                const char *separator = ",") {
            std::ostringstream buffer;
            for(size_t i = 0; i < values.size(); ++i) {
                if(i > 0) {
                    buffer << separator;
                }
                buffer << values[i];
            }
            return buffer.str();
        }

        void appendUnique(std::vector<std::string> &values,
                          const std::string &value) {
            if(value.empty()) {
                return;
            }
            if(std::find(values.begin(), values.end(), value) == values.end()) {
                values.push_back(value);
            }
        }

        double timelineControlEaseWeightLike_0x66FC5C(double easing) {
            if(easing > 0.0) {
                return easing + 1.0;
            }
            if(easing < 0.0) {
                return 1.0 / (1.0 - easing);
            }
            return 1.0;
        }

        std::string basenameWithoutExtension(const std::string &value) {
            const auto slash = value.find_last_of("/\\");
            const auto fileName =
                slash == std::string::npos ? value : value.substr(slash + 1);
            const auto dot = fileName.find_last_of('.');
            return dot == std::string::npos ? fileName
                                            : fileName.substr(0, dot);
        }

        void collectVariableListMetadata(
            const std::shared_ptr<const PSB::PSBDictionary> &base,
            MotionSnapshot &snapshot) {
            const auto list = dictionaryList(base, { "variableList" });
            if(!list) {
                return;
            }

            snapshot.variableLabels.clear();
            snapshot.variableRanges.clear();
            snapshot.variableFrames.clear();

            for(const auto &item : *list) {
                const auto dic =
                    std::dynamic_pointer_cast<PSB::PSBDictionary>(item);
                if(!dic) {
                    continue;
                }

                const auto label =
                    dictionaryString(dic, { "label", "name", "id" });
                if(!label || label->empty()) {
                    continue;
                }

                appendUnique(snapshot.variableLabels, *label);

                std::vector<VariableFrameInfo> frames;
                double minValue = std::numeric_limits<double>::infinity();
                double maxValue = -std::numeric_limits<double>::infinity();
                if(const auto frameList =
                       dictionaryList(dic, { "frameList" })) {
                    int frameIndex = 0;
                    for(const auto &frameItem : *frameList) {
                        if(const auto frameDic =
                               std::dynamic_pointer_cast<PSB::PSBDictionary>(
                                   frameItem)) {
                            const auto frameLabel =
                                dictionaryString(frameDic,
                                                 { "label", "name", "id" })
                                    .value_or(std::to_string(frameIndex));
                            const double frameValue =
                                dictionaryNumber(frameDic, { "f" })
                                    .value_or(0.0);
                            frames.push_back({ frameLabel, frameValue });
                            minValue = std::min(minValue, frameValue);
                            maxValue = std::max(maxValue, frameValue);
                        } else if(const auto value = psbNumber(frameItem)) {
                            frames.push_back(
                                { std::to_string(frameIndex), *value });
                            minValue = std::min(minValue, *value);
                            maxValue = std::max(maxValue, *value);
                        }
                        ++frameIndex;
                    }
                }

                if(!frames.empty()) {
                    snapshot.variableFrames[*label] = std::move(frames);
                    snapshot.variableRanges[*label] = { minValue, maxValue };
                }
            }
        }

        void recordControllerBinding(MotionSnapshot &snapshot,
                                     const std::string &label, int type,
                                     int index, const char *source,
                                     const char *role) {
            if(label.empty()) {
                return;
            }
            appendUnique(snapshot.variableLabels, label);
            snapshot.controllerBindings[label] = VariableControllerBinding{
                type,
                index,
                source ? source : "",
                role ? role : "",
            };
        }

        void collectInstantVariableList(
            const std::shared_ptr<const PSB::PSBDictionary> &base,
            MotionSnapshot &snapshot) {
            const auto list = dictionaryList(base, { "instantVariableList" });
            if(!list) {
                return;
            }

            snapshot.instantVariableLabels.clear();
            for(const auto &item : *list) {
                std::optional<std::string> label;
                if(const auto text = psbString(item)) {
                    label = *text;
                } else if(const auto dic =
                              std::dynamic_pointer_cast<PSB::PSBDictionary>(
                                  item)) {
                    label = dictionaryString(dic, { "label", "name", "id" });
                }

                if(!label || label->empty()) {
                    continue;
                }

                appendUnique(snapshot.variableLabels, *label);
                snapshot.instantVariableLabels.insert(*label);
            }
        }

        void collectControlBindings(
            const std::shared_ptr<const PSB::PSBDictionary> &base,
            const char *listKey, int type,
            const std::vector<std::pair<std::string, std::string>> &labelKeys,
            MotionSnapshot &snapshot) {
            const auto list = dictionaryList(base, { listKey });
            if(!list) {
                return;
            }

            int index = 0;
            for(const auto &item : *list) {
                const auto dic =
                    std::dynamic_pointer_cast<PSB::PSBDictionary>(item);
                if(!dic) {
                    ++index;
                    continue;
                }

                // Aligned to sub_6636D4: missing "enabled" returns false.
                if(!dictionaryBool(dic, { "enabled" }).value_or(false)) {
                    ++index;
                    continue;
                }

                for(const auto &[labelKey, role] : labelKeys) {
                    if(const auto label = dictionaryString(dic, { labelKey });
                       label && !label->empty()) {
                        recordControllerBinding(snapshot, *label, type, index,
                                                listKey, role.c_str());
                    }
                }
                ++index;
            }
        }

        void collectTimelineControlMetadata(
            const std::shared_ptr<const PSB::PSBDictionary> &base,
            MotionSnapshot &snapshot) {
            const auto list = dictionaryList(base, { "timelineControl" });
            if(!list) {
                return;
            }

            snapshot.mainTimelineLabels.clear();
            snapshot.diffTimelineLabels.clear();
            snapshot.timelineControlByLabel.clear();

            for(const auto &item : *list) {
                const auto dic =
                    std::dynamic_pointer_cast<PSB::PSBDictionary>(item);
                if(!dic) {
                    continue;
                }

                const auto label =
                    dictionaryString(dic, { "label", "name", "id" });
                if(!label || label->empty()) {
                    continue;
                }

                const bool isDiff =
                    dictionaryBool(dic, { "diff" }).value_or(false);
                appendUnique(isDiff ? snapshot.diffTimelineLabels
                                    : snapshot.mainTimelineLabels,
                             *label);
                TimelineControlBinding binding;
                binding.label = *label;
                binding.loopBegin =
                    dictionaryNumber(dic, { "loopBegin" }).value_or(-1.0);
                binding.loopEnd =
                    dictionaryNumber(dic, { "loopEnd" }).value_or(-1.0);
                binding.lastTime =
                    dictionaryNumber(dic, { "lastTime" }).value_or(-1.0);

                if(const auto variableList =
                       dictionaryList(dic, { "variableList" })) {
                    for(const auto &variableItem : *variableList) {
                        const auto variableDic =
                            std::dynamic_pointer_cast<PSB::PSBDictionary>(
                                variableItem);
                        if(!variableDic) {
                            continue;
                        }

                        TimelineControlTrack track;
                        track.label =
                            dictionaryString(variableDic,
                                             { "label", "name", "id" })
                                .value_or(std::string{});
                        if(track.label.empty()) {
                            continue;
                        }
                        track.instantVariable =
                            snapshot.instantVariableLabels.find(track.label) !=
                            snapshot.instantVariableLabels.end();

                        if(const auto frameList =
                               dictionaryList(variableDic, { "frameList" })) {
                            for(const auto &frameItem : *frameList) {
                                const auto frameDic = std::dynamic_pointer_cast<
                                    PSB::PSBDictionary>(frameItem);
                                if(!frameDic) {
                                    continue;
                                }

                                TimelineControlFrame frame;
                                frame.time =
                                    dictionaryNumber(frameDic, { "time" })
                                        .value_or(0.0);
                                const int type = static_cast<int>(
                                    dictionaryNumber(frameDic, { "type" })
                                        .value_or(0.0));
                                frame.isTypeZero = type == 0;
                                if(!frame.isTypeZero) {
                                    const auto contentDic =
                                        std::dynamic_pointer_cast<
                                            const PSB::PSBDictionary>(
                                            (*frameDic)["content"]);
                                    if(contentDic) {
                                        frame.value = static_cast<float>(
                                            dictionaryNumber(contentDic,
                                                             { "value" })
                                                .value_or(0.0));
                                        frame.easingWeight =
                                            timelineControlEaseWeightLike_0x66FC5C(
                                                dictionaryNumber(contentDic,
                                                                 { "easing" })
                                                    .value_or(0.0));
                                    }
                                }
                                track.frames.push_back(std::move(frame));
                            }
                        }

                        if(!track.frames.empty()) {
                            binding.lastTime = std::max(
                                binding.lastTime, track.frames.back().time);
                            binding.tracks.push_back(std::move(track));
                        }
                    }
                }

                snapshot.timelineControlByLabel[*label] = std::move(binding);
            }
        }

        void collectSelectorControlMetadata(
            const std::shared_ptr<const PSB::PSBDictionary> &base,
            MotionSnapshot &snapshot) {
            const auto list = dictionaryList(base, { "selectorControl" });
            if(!list) {
                return;
            }

            snapshot.selectorControls.clear();
            int index = 0;
            for(const auto &item : *list) {
                const auto dic =
                    std::dynamic_pointer_cast<PSB::PSBDictionary>(item);
                if(!dic) {
                    ++index;
                    continue;
                }

                const auto label =
                    dictionaryString(dic, { "label", "name", "id" });
                if(!label || label->empty()) {
                    ++index;
                    continue;
                }

                // Aligned to sub_66D8FC + sub_66E248:
                // disabled selector entries are removed from the selector label
                // container instead of participating in controller binding.
                if(!dictionaryBool(dic, { "enabled" }).value_or(false)) {
                    snapshot.controllerBindings.erase(*label);
                    ++index;
                    continue;
                }

                SelectorControlBinding binding;
                binding.label = *label;
                if(const auto optionList =
                       dictionaryList(dic, { "optionList" })) {
                    for(const auto &optionItem : *optionList) {
                        const auto optionDic =
                            std::dynamic_pointer_cast<PSB::PSBDictionary>(
                                optionItem);
                        if(!optionDic) {
                            continue;
                        }
                        const auto optionLabel = dictionaryString(
                            optionDic, { "label", "name", "id" });
                        if(!optionLabel || optionLabel->empty()) {
                            continue;
                        }
                        binding.options.push_back(SelectorControlOption{
                            *optionLabel,
                            dictionaryNumber(optionDic, { "offValue" })
                                .value_or(0.0),
                            dictionaryNumber(optionDic, { "onValue" })
                                .value_or(0.0),
                        });
                    }
                }

                snapshot.selectorControls[*label] = binding;
                recordControllerBinding(snapshot, *label, 8, index,
                                        "selectorControl", "label");
                ++index;
            }
        }

        void collectClampControlMetadata(
            const std::shared_ptr<const PSB::PSBDictionary> &base,
            MotionSnapshot &snapshot) {
            const auto list = dictionaryList(base, { "clampControl" });
            if(!list) {
                return;
            }

            snapshot.clampControls.clear();
            for(const auto &item : *list) {
                const auto dic =
                    std::dynamic_pointer_cast<PSB::PSBDictionary>(item);
                if(!dic ||
                   !dictionaryBool(dic, { "enabled" }).value_or(false)) {
                    continue;
                }

                ClampControlBinding binding;
                binding.type = static_cast<int>(
                    dictionaryNumber(dic, { "type" }).value_or(0.0));
                binding.varLr =
                    dictionaryString(dic, { "var_lr" }).value_or(std::string{});
                binding.varUd =
                    dictionaryString(dic, { "var_ud" }).value_or(std::string{});
                binding.minValue =
                    dictionaryNumber(dic, { "min" }).value_or(0.0);
                binding.maxValue =
                    dictionaryNumber(dic, { "max" }).value_or(0.0);
                snapshot.clampControls.push_back(std::move(binding));
            }
        }

        void collectMirrorControlMetadata(
            const std::shared_ptr<const PSB::PSBDictionary> &base,
            MotionSnapshot &snapshot) {
            snapshot.mirrorVariableMatchList.clear();

            const auto mirrorDic =
                std::dynamic_pointer_cast<const PSB::PSBDictionary>(
                    (*base)["mirrorControl"]);
            if(!mirrorDic) {
                return;
            }

            if(const auto list =
                   dictionaryList(mirrorDic, { "variableMatchList" })) {
                for(const auto &item : *list) {
                    if(const auto label = psbString(item);
                       label && !label->empty()) {
                        appendUnique(snapshot.mirrorVariableMatchList, *label);
                    }
                }
            }
        }

        void buildFixedControllerOutputOrder(MotionSnapshot &snapshot) {
            snapshot.fixedControllerOutputs.clear();

            for(const auto &[label, binding] : snapshot.controllerBindings) {
                switch(binding.type) {
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                        snapshot.fixedControllerOutputs.push_back(
                            FixedControllerOutputBinding{
                                label,
                                binding.type,
                                binding.index,
                                binding.role,
                            });
                        break;
                    default:
                        break;
                }
            }

            const auto typeOrder = [](int type) {
                switch(type) {
                    case 4:
                        return 0; // player+256 / sub_676478
                    case 5:
                        return 1; // player+336 / sub_67653C
                    case 6:
                        return 2; // player+416 / sub_676600
                    case 8:
                        return 3; // player+656 / sub_668470
                    case 7:
                        return 4; // player+576 / sub_666BF8
                    default:
                        return 99;
                }
            };
            const auto roleOrder = [](const std::string &role) {
                if(role == "label") {
                    return 0;
                }
                if(role == "talkLabel") {
                    return 1;
                }
                return 2;
            };

            std::sort(snapshot.fixedControllerOutputs.begin(),
                      snapshot.fixedControllerOutputs.end(),
                      [&](const FixedControllerOutputBinding &lhs,
                          const FixedControllerOutputBinding &rhs) {
                          const int lhsType = typeOrder(lhs.type);
                          const int rhsType = typeOrder(rhs.type);
                          if(lhsType != rhsType) {
                              return lhsType < rhsType;
                          }
                          if(lhs.index != rhs.index) {
                              return lhs.index < rhs.index;
                          }
                          const int lhsRole = roleOrder(lhs.role);
                          const int rhsRole = roleOrder(rhs.role);
                          if(lhsRole != rhsRole) {
                              return lhsRole < rhsRole;
                          }
                          return lhs.label < rhs.label;
                      });
        }

        void collectControlMetadata(MotionSnapshot &snapshot) {
            const auto base =
                navigateDictionaryPath(snapshot.root, "metadata/base");
            if(!base) {
                return;
            }

            collectVariableListMetadata(base, snapshot);
            collectControlBindings(
                base, "bustControl", 0,
                { { "var_lr", "var_lr" }, { "var_ud", "var_ud" } }, snapshot);
            collectControlBindings(base, "hairControl", 1,
                                   { { "var_lr", "var_lr" },
                                     { "var_lrm", "var_lrm" },
                                     { "var_ud", "var_ud" } },
                                   snapshot);
            collectControlBindings(base, "partsControl", 2,
                                   { { "var_lr", "var_lr" },
                                     { "var_lrm", "var_lrm" },
                                     { "var_ud", "var_ud" } },
                                   snapshot);
            collectControlBindings(base, "loopControl", 3,
                                   { { "var_loop", "var_loop" } }, snapshot);
            collectControlBindings(base, "eyeControl", 4,
                                   { { "label", "label" } }, snapshot);
            collectControlBindings(base, "eyebrowControl", 5,
                                   { { "label", "label" } }, snapshot);
            collectControlBindings(
                base, "mouthControl", 6,
                { { "label", "label" }, { "talkLabel", "talkLabel" } },
                snapshot);
            collectControlBindings(base, "transitionControl", 7,
                                   { { "label", "label" } }, snapshot);
            collectSelectorControlMetadata(base, snapshot);
            collectClampControlMetadata(base, snapshot);
            collectMirrorControlMetadata(base, snapshot);
            collectInstantVariableList(base, snapshot);
            collectTimelineControlMetadata(base, snapshot);
            buildFixedControllerOutputOrder(snapshot);
        }

        void maybeRecordLayer(const std::vector<std::string> &path,
                              const std::shared_ptr<PSB::PSBDictionary> &dic,
                              MotionSnapshot &snapshot) {
            const auto label = dictionaryString(dic, { "name", "label", "id" });
            if(!label || label->empty()) {
                return;
            }

            const bool layerLike = pathContainsToken(path, "layer") ||
                dictionaryHasKey(dic, "layer_id") ||
                dictionaryHasKey(dic, "layer_type") ||
                (dictionaryHasKey(dic, "label") &&
                 dictionaryHasKey(dic, "frameList")) ||
                (dictionaryHasKey(dic, "width") &&
                 dictionaryHasKey(dic, "height") &&
                 (dictionaryHasKey(dic, "left") ||
                  dictionaryHasKey(dic, "top")));
            if(!layerLike) {
                return;
            }

            // Aligned to libkrkr2.so Player_buildNodeTree_recursive (0x6B4A6C):
            // layer[] is iterated by index, duplicates preserved. Store every
            // occurrence into layerList in PSB order.
            snapshot.layerList.push_back(dic);
        }

        void maybeRecordTimeline(const std::vector<std::string> &path,
                                 const std::shared_ptr<PSB::PSBDictionary> &dic,
                                 MotionSnapshot &snapshot) {
            const bool timelineLike = pathContainsToken(path, "timeline") ||
                dictionaryKeyContains(dic, "timeline") ||
                dictionaryHasKey(dic, "loop") ||
                dictionaryHasKey(dic, "frame_count") ||
                dictionaryHasKey(dic, "frameCount");
            if(!timelineLike) {
                return;
            }

            const auto label = dictionaryString(dic, { "label", "name", "id" });
            if(!label || label->empty()) {
                return;
            }

            const bool isDiff = pathContainsToken(path, "diff");
            appendUnique(isDiff ? snapshot.diffTimelineLabels
                                : snapshot.mainTimelineLabels,
                         *label);

            snapshot.loopTimelines[*label] =
                dictionaryBool(dic, { "loop", "repeat", "is_loop" })
                    .value_or(false);
            snapshot.timelineTotalFrames[*label] =
                dictionaryNumber(dic,
                                 { "frameCount", "frame_count",
                                   "totalFrameCount", "total_frame_count",
                                   "frames", "length", "end" })
                    .value_or(0.0);
        }

        void collectValueSources(const std::shared_ptr<PSB::IPSBValue> &value,
                                 std::vector<std::string> &sources);

        void
        collectDictionarySources(const std::shared_ptr<PSB::PSBDictionary> &dic,
                                 std::vector<std::string> &sources) {
            for(const auto &[key, child] : *dic) {
                const auto loweredKey = lowercase(key);
                if(const auto text = psbString(child)) {
                    if(looksLikeStoragePath(*text) ||
                       loweredKey.find("source") != std::string::npos ||
                       loweredKey == "path" || loweredKey == "file" ||
                       loweredKey == "src") {
                        appendUnique(sources, *text);
                    }
                }
                collectValueSources(child, sources);
            }
        }

        void collectListSources(const std::shared_ptr<PSB::PSBList> &list,
                                std::vector<std::string> &sources) {
            for(const auto &item : *list) {
                collectValueSources(item, sources);
            }
        }

        void collectValueSources(const std::shared_ptr<PSB::IPSBValue> &value,
                                 std::vector<std::string> &sources) {
            if(auto dic =
                   std::dynamic_pointer_cast<PSB::PSBDictionary>(value)) {
                collectDictionarySources(dic, sources);
            } else if(auto list =
                          std::dynamic_pointer_cast<PSB::PSBList>(value)) {
                collectListSources(list, sources);
            } else if(const auto text = psbString(value)) {
                if(looksLikeStoragePath(*text)) {
                    appendUnique(sources, *text);
                }
            }
        }

        void
        maybeRecordMotionClip(const std::vector<std::string> &path,
                              const std::shared_ptr<PSB::PSBDictionary> &dic,
                              MotionSnapshot &snapshot) {
            if(path.size() < 4 ||
               lowercase(path[path.size() - 2]) != "motion" ||
               lowercase(path[path.size() - 4]) != "object") {
                return;
            }

            const auto label = path.back();
            if(label.empty()) {
                return;
            }

            // Aligned to libkrkr2.so Player+548 priority[] ordering:
            // store each motion/priority entry by PSB array index (duplicate
            // labels preserved) with auxiliary label→index map resolving via
            // last-wins to mirror the binary's labelMap semantics.
            auto existingIt = snapshot.clipIndexByLabel.find(label);
            int clipIndex;
            if(existingIt != snapshot.clipIndexByLabel.end()) {
                clipIndex = existingIt->second;
            } else {
                clipIndex = static_cast<int>(snapshot.clipList.size());
                snapshot.clipList.emplace_back();
            }
            snapshot.clipIndexByLabel[label] = clipIndex;
            auto &clip = snapshot.clipList[clipIndex];
            clip.label = label;
            clip.owner = path[path.size() - 3];
            clip.motionObject = dic;
            clip.contentObject = dic;
            clip.totalFrames =
                dictionaryNumber(dic,
                                 { "lastTime", "frameCount", "frame_count",
                                   "totalFrameCount", "total_frame_count",
                                   "frames", "length", "end" })
                    .value_or(0.0);
            if(const auto loopTime = dictionaryNumber(dic, { "loopTime" })) {
                clip.loopTime = *loopTime;
                clip.loop = *loopTime >= 0.0;
            } else if(const auto loop = dictionaryBool(
                          dic, { "loop", "repeat", "is_loop" })) {
                clip.loop = *loop;
                clip.loopTime = *loop ? 0.0 : -1.0;
            }

            if(const auto layers = dictionaryList(dic, { "layer" })) {
                for(const auto &item : *layers) {
                    const auto layer =
                        std::dynamic_pointer_cast<PSB::PSBDictionary>(item);
                    if(!layer) {
                        continue;
                    }

                    const auto layerLabel =
                        dictionaryString(layer, { "label", "name", "id" });
                    if(!layerLabel || layerLabel->empty()) {
                        continue;
                    }

                    // Aligned to libkrkr2.so: motion.priority[*].layer[] is
                    // iterated by index, duplicates preserved.
                    clip.layerList.push_back(layer);
                    (void)layerLabel;
                    collectValueSources(layer, clip.sourceCandidates);
                }
            }

            collectValueSources(dic, clip.sourceCandidates);

            appendUnique(snapshot.mainTimelineLabels, clip.label);
            snapshot.loopTimelines[clip.label] = clip.loop;
            snapshot.timelineLoopTimes[clip.label] = clip.loopTime;
            snapshot.timelineTotalFrames[clip.label] = clip.totalFrames;
        }

        void appendResolvedLayerReference(
            const std::shared_ptr<PSB::IPSBValue> &item,
            const MotionSnapshot &snapshot,
            std::vector<std::shared_ptr<const PSB::PSBDictionary>> &out) {
            if(auto layer =
                   std::dynamic_pointer_cast<const PSB::PSBDictionary>(item)) {
                out.push_back(layer);
                return;
            }
            if(const auto num = psbNumber(item)) {
                const int idx = static_cast<int>(*num);
                if(idx >= 0 &&
                   idx < static_cast<int>(snapshot.layerList.size())) {
                    out.push_back(snapshot.layerList[static_cast<size_t>(idx)]);
                }
            }
        }

        // Emote motion clips store layer[] as node indices (参考 sdl3/emotefile
        // emotemotion constructor); resolve to layer dicts after root layerList
        // is populated.
        void resolveClipLayerReferences(MotionSnapshot &snapshot) {
            for(auto &clip : snapshot.clipList) {
                if(!clip.layerList.empty() || !clip.motionObject) {
                    continue;
                }

                std::vector<std::shared_ptr<const PSB::PSBDictionary>> resolved;
                if(const auto layers =
                       dictionaryList(clip.motionObject, { "layer" })) {
                    for(const auto &item : *layers) {
                        appendResolvedLayerReference(item, snapshot, resolved);
                    }
                }

                if(resolved.empty()) {
                    if(const auto priorities =
                           dictionaryList(clip.motionObject, { "priority" })) {
                        for(const auto &priItem : *priorities) {
                            const auto priDic =
                                std::dynamic_pointer_cast<PSB::PSBDictionary>(
                                    priItem);
                            if(!priDic) {
                                continue;
                            }
                            if(const auto content =
                                   dictionaryList(priDic, { "content" })) {
                                for(const auto &item : *content) {
                                    appendResolvedLayerReference(item, snapshot,
                                                                 resolved);
                                }
                            }
                            if(!resolved.empty()) {
                                break;
                            }
                        }
                    }
                }

                clip.layerList = std::move(resolved);
            }
        }

        void scanValue(const std::shared_ptr<PSB::IPSBValue> &value,
                       std::vector<std::string> &path,
                       MotionSnapshot &snapshot);

        void scanDictionary(const std::shared_ptr<PSB::PSBDictionary> &dic,
                            std::vector<std::string> &path,
                            MotionSnapshot &snapshot) {
            maybeRecordMotionClip(path, dic, snapshot);
            maybeRecordLayer(path, dic, snapshot);
            maybeRecordTimeline(path, dic, snapshot);

            if(const auto width = dictionaryNumber(dic, { "width" });
               width && snapshot.width == 0.0) {
                snapshot.width = *width;
            }
            if(const auto height = dictionaryNumber(dic, { "height" });
               height && snapshot.height == 0.0) {
                snapshot.height = *height;
            }

            for(const auto &[key, child] : *dic) {
                const auto loweredKey = lowercase(key);
                if(const auto text = psbString(child)) {
                    if(looksLikeStoragePath(*text) ||
                       loweredKey.find("source") != std::string::npos ||
                       loweredKey == "path" || loweredKey == "file" ||
                       loweredKey == "src") {
                        appendUnique(snapshot.sourceCandidates, *text);
                    }
                }

                path.push_back(key);
                scanValue(child, path, snapshot);
                path.pop_back();
            }
        }

        void scanList(const std::shared_ptr<PSB::PSBList> &list,
                      std::vector<std::string> &path,
                      MotionSnapshot &snapshot) {
            for(size_t index = 0; index < list->size(); ++index) {
                path.push_back(std::to_string(index));
                scanValue((*list)[static_cast<int>(index)], path, snapshot);
                path.pop_back();
            }
        }

        void scanValue(const std::shared_ptr<PSB::IPSBValue> &value,
                       std::vector<std::string> &path,
                       MotionSnapshot &snapshot) {
            if(auto dic =
                   std::dynamic_pointer_cast<PSB::PSBDictionary>(value)) {
                scanDictionary(dic, path, snapshot);
            } else if(auto list =
                          std::dynamic_pointer_cast<PSB::PSBList>(value)) {
                scanList(list, path, snapshot);
            } else if(const auto text = psbString(value)) {
                if(looksLikeStoragePath(*text)) {
                    appendUnique(snapshot.sourceCandidates, *text);
                }
            }
        }

        bool looksLikeEmbeddedSourceKey(const std::string &value) {
            return looksLikeStoragePath(value) ||
                value.find('/') != std::string::npos ||
                value.find('\\') != std::string::npos;
        }

        void collectResourceMap(const std::shared_ptr<PSB::IPSBValue> &value,
                                std::vector<std::string> &path,
                                MotionSnapshot &snapshot);

        void collectDictionaryResourceMap(
            const std::shared_ptr<PSB::PSBDictionary> &dic,
            std::vector<std::string> &path, MotionSnapshot &snapshot) {
            for(const auto &[key, child] : *dic) {
                path.push_back(key);
                collectResourceMap(child, path, snapshot);
                path.pop_back();
            }
        }

        void collectListResourceMap(const std::shared_ptr<PSB::PSBList> &list,
                                    std::vector<std::string> &path,
                                    MotionSnapshot &snapshot) {
            for(size_t index = 0; index < list->size(); ++index) {
                path.push_back(std::to_string(index));
                collectResourceMap((*list)[static_cast<int>(index)], path,
                                   snapshot);
                path.pop_back();
            }
        }

        void collectResourceMap(const std::shared_ptr<PSB::IPSBValue> &value,
                                std::vector<std::string> &path,
                                MotionSnapshot &snapshot) {
            if(auto resource =
                   std::dynamic_pointer_cast<PSB::PSBResource>(value)) {
                std::string joined;
                for(size_t index = 0; index < path.size(); ++index) {
                    if(index != 0) {
                        joined += '/';
                    }
                    joined += path[index];
                }
                if(!joined.empty()) {
                    snapshot.resourcesByPath.emplace(joined, resource);
                    if(looksLikeEmbeddedSourceKey(joined)) {
                        appendUnique(snapshot.sourceCandidates, joined);
                    }
                }
                return;
            }

            if(auto dic =
                   std::dynamic_pointer_cast<PSB::PSBDictionary>(value)) {
                collectDictionaryResourceMap(dic, path, snapshot);
            } else if(auto list =
                          std::dynamic_pointer_cast<PSB::PSBList>(value)) {
                collectListResourceMap(list, path, snapshot);
            }
        }

        void collectRootResources(
            const std::shared_ptr<const PSB::PSBDictionary> &root,
            MotionSnapshot &snapshot) {
            if(!root) {
                return;
            }

            std::vector<std::string> path;
            collectResourceMap(
                std::const_pointer_cast<PSB::PSBDictionary>(root), path,
                snapshot);

            for(const auto &[key, value] : *root) {
                const auto resource =
                    std::dynamic_pointer_cast<PSB::PSBResource>(value);
                if(!resource) {
                    continue;
                }
                if(looksLikeEmbeddedSourceKey(key)) {
                    appendUnique(snapshot.sourceCandidates, key);
                }
            }
        }

        void appendResourceAlias(MotionSnapshot &snapshot, const ttstr &alias) {
            const auto raw = narrow(alias);
            if(raw.empty()) {
                return;
            }
            appendUnique(snapshot.resourceAliases, raw);
        }

        std::shared_ptr<PSB::PSBFile> loadPSBFile(const ttstr &path,
                                                  const tjs_int decryptSeed) {
            auto file = std::make_shared<PSB::PSBFile>();
            file->setSeed(decryptSeed);
            if(!file->loadPSBFile(path)) {
                G_PluginLog.error("motion load file: {} failed",
                                  path.AsStdString());
                return nullptr;
            }
            return file;
        }

    } // namespace

    void ensureRootNodeLike_0x6CED30(PlayerRuntime &runtime) {
        if(!runtime.nodes.empty()) {
            runtime.nodes.front().index = 0;
            runtime.nodes.front().parentIndex = -1;
            return;
        }
        MotionNode root;
        root.index = 0;
        root.parentIndex = -1;
        runtime.nodes.emplace_back(std::move(root));
    }

    void resetNodeTreeKeepRootLike_0x6B56F8(PlayerRuntime &runtime) {
        ensureRootNodeLike_0x6CED30(runtime);
        auto &root = runtime.nodes.front();
        root.index = 0;
        root.parentIndex = -1;
        if(runtime.nodes.size() > 1) {
            runtime.nodes.erase(std::next(runtime.nodes.begin()),
                                runtime.nodes.end());
        }
        runtime.nodeLabelMap.clear();
        runtime.renderItemNativeFieldLifetimeByNode.clear();
    }

    std::shared_ptr<PlayerRuntime> makePlayerRuntime() {
        auto runtime = std::make_shared<PlayerRuntime>();
        runtime->defaultParameterEntry.rangeScale = 1.0;
        runtime->defaultParameterEntry.mode = 0;
        ensureRootNodeLike_0x6CED30(*runtime);
        return runtime;
    }

    std::string narrow(const ttstr &value) { return value.AsStdString(); }

    ttstr widen(const std::string &value) { return ttstr{ value }; }

    std::vector<ttstr> buildMotionLookupCandidates(const ttstr &name) {
        std::vector<ttstr> candidates;
        if(name.IsEmpty()) {
            return candidates;
        }

        const auto raw = narrow(name);
        const bool hasPathSeparator = raw.find('/') != std::string::npos ||
            raw.find('\\') != std::string::npos;
        const bool hasKnownExtension = hasExtension(raw);
        if(hasPathSeparator || hasKnownExtension) {
            candidates.push_back(name);
        } else {
            candidates.emplace_back(ttstr{ raw + ".mtn" });
            candidates.emplace_back(ttstr{ raw + ".psb" });
            candidates.emplace_back(ttstr{ "motion/" + raw + ".mtn" });
            candidates.emplace_back(ttstr{ "motion/" + raw + ".psb" });
        }

        return candidates;
    }

    bool resolveExistingPath(const std::vector<ttstr> &candidates,
                             ttstr &resolved) {
        for(const auto &candidate : candidates) {
            if(const auto placed = TVPGetPlacedPath(candidate);
               !placed.IsEmpty()) {
                resolved = placed;
                return true;
            }
        }
        return false;
    }

    void appendEmbeddedSourceCandidates(const MotionSnapshot &snapshot,
                                        const std::string &source,
                                        std::vector<ttstr> &candidates) {
        if(source.empty()) {
            return;
        }

        for(const auto &alias : snapshot.resourceAliases) {
            candidates.emplace_back(ttstr{ TJS_W("psb://") } + widen(alias) +
                                    TJS_W("/") + widen(source));
        }
    }

    std::shared_ptr<const PSB::PSBDictionary> resolveLayerDictionaryReference(
        const std::shared_ptr<PSB::IPSBValue> &item,
        const std::vector<std::shared_ptr<const PSB::PSBDictionary>>
            &layerList) {
        if(auto layer =
               std::dynamic_pointer_cast<const PSB::PSBDictionary>(item)) {
            return layer;
        }
        if(const auto num = psbNumber(item)) {
            const int idx = static_cast<int>(*num);
            if(idx >= 0 && idx < static_cast<int>(layerList.size())) {
                return layerList[static_cast<size_t>(idx)];
            }
        }
        return nullptr;
    }

    void mergeAttachedSnapshotResources(MotionSnapshot &primary,
                                        const MotionSnapshot &attached) {
        for(const auto &alias : attached.resourceAliases) {
            appendUnique(primary.resourceAliases, alias);
        }
        for(const auto &source : attached.sourceCandidates) {
            appendUnique(primary.sourceCandidates, source);
        }
        for(const auto &[path, resource] : attached.resourcesByPath) {
            primary.resourcesByPath.emplace(path, resource);
        }
        if(attached.file) {
            PSB::registerRootResources(
                { widen(attached.path),
                  TVPExtractStorageName(widen(attached.path)) },
                *attached.file);
        }
    }

    std::shared_ptr<MotionSnapshot>
    loadMotionSnapshot(const ttstr &path, const tjs_int decryptSeed) {
        const auto file = loadPSBFile(path, decryptSeed);
        if(!file) {
            return nullptr;
        }
        if(file->getType() != PSB::PSBType::Motion) {
            G_PluginLog.error("this psb file is not motion file: {}",
                              path.AsStdString());
            return nullptr;
        }

        const auto root = file->getObjects();
        if(!root) {
            return nullptr;
        }

        auto snapshot = std::make_shared<MotionSnapshot>();
        snapshot->path = narrow(path);
        snapshot->file = file;
        snapshot->root = root;
        snapshot->moduleValue = root->toTJSVal();
        if(logoChainTraceEnabled(snapshot)) {
            resetLogoChainTraceSession(snapshot->path);
            logoChainTraceLogf(snapshot->path, "snapshot.load", "PSB parse",
                               -1.0, "path={} phase=begin", snapshot->path);
        }
        appendResourceAlias(*snapshot, path);
        appendResourceAlias(*snapshot, TVPExtractStorageName(path));
        PSB::registerRootResources({ path, TVPExtractStorageName(path) },
                                   *file);

        std::vector<std::string> pathParts;
        scanValue(std::const_pointer_cast<PSB::PSBDictionary>(root), pathParts,
                  *snapshot);
        resolveClipLayerReferences(*snapshot);
        collectControlMetadata(*snapshot);
        collectRootResources(root, *snapshot);
        if(logoChainTraceEnabled(snapshot)) {
            const auto rootParameterList =
                dictionaryList(snapshot->root, { "parameter" });
            const auto rootParameterizeValue =
                (*snapshot->root)["parameterize"];
            const auto contentNode =
                navigateDictionaryPath(snapshot->root, "content");
            const auto contentParameterList = contentNode
                ? dictionaryList(contentNode, { "parameter" })
                : nullptr;
            const auto contentParameterizeValue = contentNode
                ? (*contentNode)["parameterize"]
                : std::shared_ptr<PSB::IPSBValue>{};
            const auto describeValue =
                [](const std::shared_ptr<PSB::IPSBValue> &value)
                -> std::string {
                if(!value)
                    return "<none>";
                if(std::dynamic_pointer_cast<PSB::PSBList>(value))
                    return "list";
                if(std::dynamic_pointer_cast<PSB::PSBDictionary>(value))
                    return "dict";
                if(std::dynamic_pointer_cast<PSB::PSBString>(value))
                    return "string";
                if(std::dynamic_pointer_cast<PSB::PSBNumber>(value))
                    return "number";
                if(std::dynamic_pointer_cast<PSB::PSBBool>(value))
                    return "bool";
                return "other";
            };
            logoChainTraceLogf(
                snapshot->path, "snapshot.parsed", "PSB parse", -1.0,
                "path={} clipCount={} mainLabels={} sourceCount={} "
                "resourceAliases={} variableCount={} controllerBindings={} "
                "fixedControllerOutputs={} selectorControls={} "
                "timelineControls={} instantVariables={} clampControls={} "
                "mirrorMatches={} rootParameterCount={} rootParameterize={} "
                "contentParameterCount={} contentParameterize={}",
                snapshot->path, snapshot->clipList.size(),
                joinStrings(snapshot->mainTimelineLabels),
                snapshot->sourceCandidates.size(),
                joinStrings(snapshot->resourceAliases),
                snapshot->variableLabels.size(),
                snapshot->controllerBindings.size(),
                snapshot->fixedControllerOutputs.size(),
                snapshot->selectorControls.size(),
                snapshot->timelineControlByLabel.size(),
                snapshot->instantVariableLabels.size(),
                snapshot->clampControls.size(),
                snapshot->mirrorVariableMatchList.size(),
                rootParameterList ? rootParameterList->size() : 0,
                describeValue(rootParameterizeValue),
                contentParameterList ? contentParameterList->size() : 0,
                describeValue(contentParameterizeValue));
            for(const auto &[resourcePath, resource] :
                snapshot->resourcesByPath) {
                if(!hasSuffix(resourcePath, "/pixel") &&
                   !hasSuffix(resourcePath, "/pal")) {
                    continue;
                }
                const auto iconPath = hasSuffix(resourcePath, "/pixel")
                    ? resourcePath.substr(0, resourcePath.size() - 6)
                    : resourcePath.substr(0, resourcePath.size() - 4);
                const auto iconNode =
                    navigateDictionaryPath(snapshot->root, iconPath);
                const auto width = iconNode
                    ? dictionaryNumber(iconNode, { "width", "truncated_width" })
                          .value_or(0.0)
                    : 0.0;
                const auto height = iconNode
                    ? dictionaryNumber(iconNode,
                                       { "height", "truncated_height" })
                          .value_or(0.0)
                    : 0.0;
                const auto originX = iconNode
                    ? dictionaryNumber(iconNode, { "originX" }).value_or(0.0)
                    : 0.0;
                const auto originY = iconNode
                    ? dictionaryNumber(iconNode, { "originY" }).value_or(0.0)
                    : 0.0;
                const auto compress = iconNode
                    ? dictionaryString(iconNode, { "compress" }).value_or("raw")
                    : std::string("raw");
                const bool hasPal =
                    snapshot->resourcesByPath.find(iconPath + "/pal") !=
                    snapshot->resourcesByPath.end();
                logoChainTraceLogf(
                    snapshot->path, "snapshot.resource", "PSB parse", -1.0,
                    "resource={} width={:.0f} height={:.0f} "
                    "origin=({:.3f},{:.3f}) hasPal={} isRL={} bytes={}",
                    resourcePath, width, height, originX, originY,
                    hasPal ? 1 : 0, lowercase(compress) == "rl" ? 1 : 0,
                    resource ? resource->data.size() : 0);
            }
        }
        registerModuleSnapshot(snapshot->moduleValue, snapshot);
        return snapshot;
    }

    tTJSVariant loadPSBVariant(const ttstr &path, const tjs_int decryptSeed) {
        if(const auto snapshot = loadMotionSnapshot(path, decryptSeed)) {
            return snapshot->moduleValue;
        }

        const auto file = loadPSBFile(path, decryptSeed);
        if(!file || !file->getObjects()) {
            return {};
        }

        return file->getObjects()->toTJSVal();
    }

    void
    registerModuleSnapshot(const tTJSVariant &module,
                           const std::shared_ptr<MotionSnapshot> &snapshot) {
        if(module.Type() != tvtObject || module.AsObjectNoAddRef() == nullptr ||
           !snapshot) {
            return;
        }

        std::lock_guard lock(snapshotRegistryMutex());
        snapshotRegistry()[module.AsObjectNoAddRef()] = snapshot;
    }

    std::shared_ptr<MotionSnapshot>
    lookupModuleSnapshot(const tTJSVariant &module) {
        if(module.Type() != tvtObject || module.AsObjectNoAddRef() == nullptr) {
            return nullptr;
        }

        std::lock_guard lock(snapshotRegistryMutex());
        const auto it = snapshotRegistry().find(module.AsObjectNoAddRef());
        return it != snapshotRegistry().end() ? it->second : nullptr;
    }

    void unregisterModuleSnapshot(const tTJSVariant &module) {
        if(module.Type() != tvtObject || module.AsObjectNoAddRef() == nullptr) {
            return;
        }

        std::lock_guard lock(snapshotRegistryMutex());
        snapshotRegistry().erase(module.AsObjectNoAddRef());
    }

    void clearModuleSnapshots() {
        std::lock_guard lock(snapshotRegistryMutex());
        snapshotRegistry().clear();
    }

    tTJSVariant makeArray(const std::vector<tTJSVariant> &items) {
        iTJSDispatch2 *array = TJSCreateArrayObject();
        static tjs_uint addHint = 0;
        for(const auto &item : items) {
            tTJSVariant value = item;
            tTJSVariant *args[] = { &value };
            array->FuncCall(0, TJS_W("add"), &addHint, nullptr, 1, args, array);
        }
        tTJSVariant result(array, array);
        array->Release();
        return result;
    }

    tTJSVariant makeDictionary(
        const std::vector<std::pair<std::string, tTJSVariant>> &entries) {
        iTJSDispatch2 *dic = TJSCreateDictionaryObject();
        for(const auto &[key, value] : entries) {
            tTJSVariant tmp = value;
            dic->PropSet(TJS_MEMBERENSURE, widen(key).c_str(), nullptr, &tmp,
                         dic);
        }
        tTJSVariant result(dic, dic);
        dic->Release();
        return result;
    }

    std::vector<tTJSVariant>
    stringsToVariants(const std::vector<std::string> &values) {
        std::vector<tTJSVariant> result;
        result.reserve(values.size());
        for(const auto &value : values) {
            result.emplace_back(widen(value));
        }
        return result;
    }

    void
    primeTimelineStates(std::unordered_map<std::string, TimelineState> &states,
                        const MotionSnapshot &snapshot) {
        const auto primeOne = [&](const std::string &label) {
            auto &state = states[label];
            state.label = label;
            state.controlInitialized = false;
            state.controlLastAppliedTime = 0.0;
            state.controlFrameCursor.clear();
            state.controlTrackValues.clear();
            state.controlTrackAnimators.clear();
            state.loop = snapshot.loopTimelines.find(label) !=
                    snapshot.loopTimelines.end()
                ? snapshot.loopTimelines.at(label)
                : false;
            state.loopTime = snapshot.timelineLoopTimes.find(label) !=
                    snapshot.timelineLoopTimes.end()
                ? snapshot.timelineLoopTimes.at(label)
                : -1.0;
            state.totalFrames = snapshot.timelineTotalFrames.find(label) !=
                    snapshot.timelineTotalFrames.end()
                ? snapshot.timelineTotalFrames.at(label)
                : 0.0;
        };

        for(const auto &label : snapshot.mainTimelineLabels) {
            primeOne(label);
        }
        for(const auto &label : snapshot.diffTimelineLabels) {
            primeOne(label);
        }
    }

    void stepTimelines(std::unordered_map<std::string, TimelineState> &states,
                       const double dt, std::vector<MotionEvent> *events) {
        if(dt <= 0.0) {
            return;
        }

        for(auto &[name, state] : states) {
            if(!state.playing) {
                state.wasPlaying = false;
                continue;
            }

            state.wasPlaying = true;
            state.currentTime += dt;
            if(state.totalFrames <= 0.0) {
                continue;
            }

            if(state.currentTime < state.totalFrames) {
                continue;
            }

            if(!wrapTimelineCurrentTime(state.currentTime, state.totalFrames,
                                        state.loopTime)) {
                state.playing = false;
                // Aligned to libkrkr2.so Player_dispatchEvents (0x6C4490):
                // Queue onSync event when timeline stops (playing→false)
                if(events && state.wasPlaying) {
                    events->push_back({ 1, name, {} });
                    state.wasPlaying = false;
                }
            }
        }
    }

    bool logoChainTraceEnabled() {
        static const bool enabled = logoTraceQueryEnabled();
        return enabled;
    }

    bool logoSnapshotMarkEnabled() {
        static const bool enabled = logoSnapshotQueryEnabled();
        return enabled;
    }

    bool logoChainTraceEnabledForPath(const std::string &motionPath) {
        return logoChainTraceEnabled() && isTargetLogoMotionPath(motionPath);
    }

    bool logoSnapshotMarkEnabledForPath(const std::string &motionPath) {
        return logoSnapshotMarkEnabled() && isTargetLogoMotionPath(motionPath);
    }

    bool
    logoChainTraceEnabled(const std::shared_ptr<MotionSnapshot> &snapshot) {
        return snapshot && logoChainTraceEnabledForPath(snapshot->path);
    }

    void resetLogoChainTraceSession(const std::string &motionPath) {
        if(!logoChainTraceEnabledForPath(motionPath)) {
            return;
        }
        std::lock_guard lock(logoTraceMutex());
        auto &session = ensureLogoTraceSessionLocked(motionPath);
        session = {};
        session.motionPath = motionPath;
        session.motionName = basename(motionPath);
    }

    void logoChainTraceLog(const std::string &motionPath, const char *stage,
                           const char *func, const double frameTime,
                           const std::string &message) {
        if(!logoChainTraceEnabledForPath(motionPath)) {
            return;
        }
        std::lock_guard lock(logoTraceMutex());
        auto &session = ensureLogoTraceSessionLocked(motionPath);
        ++session.sequence;
        G_PluginLog.warn("CHAIN SEQ={} stage={} func={} motion={} frame={} {}",
                         session.sequence, stage, func, session.motionName,
                         frameLabel(frameTime), message);
    }

    void logoChainTraceCheck(const std::string &motionPath, const char *stage,
                             const char *func, const double frameTime,
                             const std::string &expected,
                             const std::string &actual, const bool ok,
                             const std::string &likelyRootCause) {
        if(!logoChainTraceEnabledForPath(motionPath)) {
            return;
        }

        std::lock_guard lock(logoTraceMutex());
        auto &session = ensureLogoTraceSessionLocked(motionPath);
        ++session.sequence;
        G_PluginLog.warn(
            "CHAIN SEQ={} stage={} func={} motion={} frame={} exp={} "
            "act={} ok={}",
            session.sequence, stage, func, session.motionName,
            frameLabel(frameTime), expected, actual, ok ? 1 : 0);

        if(ok) {
            if(session.firstBadStage.empty()) {
                session.upstreamLastGoodStage = stage;
            }
            return;
        }

        if(session.firstBadStage.empty()) {
            session.firstBadStage = stage;
            session.firstBadExpected = expected;
            session.firstBadActual = actual;
            session.likelyRootCause = likelyRootCause;
        }
    }

    void logoChainTraceSummary(const std::string &motionPath, const char *func,
                               const double frameTime,
                               const std::string &note) {
        if(!logoChainTraceEnabledForPath(motionPath)) {
            return;
        }

        std::lock_guard lock(logoTraceMutex());
        auto &session = ensureLogoTraceSessionLocked(motionPath);
        if(session.summaryEmitted) {
            return;
        }
        session.summaryEmitted = true;

        const auto firstBadStage = session.firstBadStage.empty()
            ? std::string("none")
            : session.firstBadStage;
        const auto expected = session.firstBadExpected.empty()
            ? std::string("all_logged_stages_ok")
            : session.firstBadExpected;
        const auto actual = session.firstBadActual.empty()
            ? std::string("all_logged_stages_ok")
            : session.firstBadActual;
        const auto upstream = session.upstreamLastGoodStage.empty()
            ? std::string("none")
            : session.upstreamLastGoodStage;
        const auto rootCause = session.likelyRootCause.empty()
            ? std::string("not_detected_in_logged_fields")
            : session.likelyRootCause;

        G_PluginLog.warn("CHAIN SUMMARY func={} motion={} frame={} "
                         "first_bad_stage={} expected={} actual={} "
                         "upstream_last_good_stage={} likely_root_cause={}{}{}",
                         func, session.motionName, frameLabel(frameTime),
                         firstBadStage, expected, actual, upstream, rootCause,
                         note.empty() ? "" : " note=", note);
    }

    // Scan PSB layer tree for action/sync events between prevTime and newTime.
    // Aligned to libkrkr2.so: updateLayers queues events when frame evaluation
    // crosses a frame boundary that has content.action or content.sync.
    void scanLayerActions(const MotionSnapshot &snapshot, double prevTime,
                          double newTime, std::vector<MotionEvent> &events) {
        // Helper: read the layer label from the PSB dict (via
        // "label"/"name"/"id").
        const auto readLabel =
            [](const std::shared_ptr<const PSB::PSBDictionary> &dic)
            -> std::string {
            if(!dic)
                return {};
            if(const auto s = dictionaryString(dic, { "label", "name", "id" }))
                return *s;
            return {};
        };

        // Emit events for a single layer dict's frameList crossing [prevTime,
        // newTime].
        const auto scanLayer =
            [&](const std::shared_ptr<const PSB::PSBDictionary> &layerDict) {
                if(!layerDict)
                    return;
                const std::string layerLabel = readLabel(layerDict);
                auto frameList = std::dynamic_pointer_cast<PSB::PSBList>(
                    (*layerDict)["frameList"]);
                if(!frameList)
                    return;

                for(size_t i = 0; i < frameList->size(); ++i) {
                    auto frame = std::dynamic_pointer_cast<PSB::PSBDictionary>(
                        (*frameList)[static_cast<int>(i)]);
                    if(!frame)
                        continue;

                    auto timeVal = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*frame)["time"]);
                    if(!timeVal)
                        continue;
                    double frameTime = 0.0;
                    switch(timeVal->numberType) {
                        case PSB::PSBNumberType::Float:
                            frameTime = timeVal->getValue<float>();
                            break;
                        case PSB::PSBNumberType::Double:
                            frameTime = timeVal->getValue<double>();
                            break;
                        case PSB::PSBNumberType::Int:
                            frameTime =
                                static_cast<double>(timeVal->getValue<int>());
                            break;
                        default:
                            frameTime = static_cast<double>(
                                timeVal->getValue<tjs_int64>());
                            break;
                    }

                    if(frameTime <= prevTime || frameTime > newTime)
                        continue;

                    auto content =
                        std::dynamic_pointer_cast<PSB::PSBDictionary>(
                            (*frame)["content"]);
                    if(!content)
                        continue;

                    if(auto actionStr =
                           std::dynamic_pointer_cast<PSB::PSBString>(
                               (*content)["action"])) {
                        if(!actionStr->value.empty()) {
                            events.push_back(
                                { 0, actionStr->value, layerLabel });
                        }
                    }

                    if(auto syncVal = std::dynamic_pointer_cast<PSB::PSBNumber>(
                           (*content)["sync"])) {
                        double sv = 0.0;
                        switch(syncVal->numberType) {
                            case PSB::PSBNumberType::Float:
                                sv = syncVal->getValue<float>();
                                break;
                            case PSB::PSBNumberType::Int:
                                sv = static_cast<double>(
                                    syncVal->getValue<int>());
                                break;
                            default:
                                break;
                        }
                        if(sv != 0.0) {
                            events.push_back({ 1, layerLabel, {} });
                        }
                    }
                }
            };

        // Iterate the PSB "layer" array in index order (libkrkr2.so alignment).
        for(const auto &layerDict : snapshot.layerList) {
            scanLayer(layerDict);
        }

        // Also scan every clip's layer[] in PSB priority[] order.
        for(const auto &clip : snapshot.clipList) {
            for(const auto &layerDict : clip.layerList) {
                scanLayer(layerDict);
            }
        }
    }

} // namespace motion::detail
