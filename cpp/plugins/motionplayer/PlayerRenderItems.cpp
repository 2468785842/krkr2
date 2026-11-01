// PlayerRenderItems.cpp — calcBounds and prepared render-item build
// Split from PlayerUpdateLayers.cpp for maintainability.
//
#include "PlayerInternal.h"
#include "MotionTraceWeb.h"

using namespace motion::internal;

namespace {

    // Detect parent/child Player cycles while merging nested motion render
    // lists.
    thread_local std::unordered_set<motion::Player *>
        gActivePrepareRenderPlayers;

    class PrepareRenderPlayerScope {
    public:
        explicit PrepareRenderPlayerScope(motion::Player *player) :
            _player(player) {
            if(_player) {
                _active = gActivePrepareRenderPlayers.insert(_player).second;
            }
        }
        ~PrepareRenderPlayerScope() {
            if(_player && _active) {
                gActivePrepareRenderPlayers.erase(_player);
            }
        }
        PrepareRenderPlayerScope(const PrepareRenderPlayerScope &) = delete;
        PrepareRenderPlayerScope &
        operator=(const PrepareRenderPlayerScope &) = delete;
        bool active() const { return _active; }

    private:
        motion::Player *_player = nullptr;
        bool _active = false;
    };

    inline std::array<std::uint32_t, 4>
    copyPackedColorsFromBytes(const uint8_t (&colorBytes)[16]) {
        std::array<std::uint32_t, 4> packedColors{};
        std::memcpy(packedColors.data(), colorBytes,
                    sizeof(std::uint32_t) * packedColors.size());
        return packedColors;
    }

    inline std::array<int, 4> unpackPackedRgba(std::uint32_t packedColor) {
        return {
            static_cast<int>(packedColor & 0xFFu),
            static_cast<int>((packedColor >> 8) & 0xFFu),
            static_cast<int>((packedColor >> 16) & 0xFFu),
            static_cast<int>((packedColor >> 24) & 0xFFu),
        };
    }

} // anonymous namespace

namespace motion {

    void Player::calcBounds() {
        // Equivalent to sub_6D5164 @ 0x6D5178's `player+544` null gate —
        // without a loaded motion there is no render list to measure.
        if(!_runtime || !_runtime->activeMotion) {
            _boundsMinX = 0.0;
            _boundsMinY = 0.0;
            _boundsMaxX = 0.0;
            _boundsMaxY = 0.0;
            return;
        }
        PrepareRenderPlayerScope selfScope(this);
        if(!selfScope.active()) {
            return;
        }
        const auto motionPath = _runtime && _runtime->activeMotion
            ? _runtime->activeMotion->path
            : std::string{};

        _boundsMinX = 1e308;
        _boundsMinY = 1e308;
        _boundsMaxX = -1e308;
        _boundsMaxY = -1e308;

        bool haveBounds = false;
        auto mergeBounds = [&](double minX, double minY, double maxX,
                               double maxY) {
            if(minX > maxX || minY > maxY) {
                return;
            }
            if(!haveBounds) {
                _boundsMinX = minX;
                _boundsMinY = minY;
                _boundsMaxX = maxX;
                _boundsMaxY = maxY;
                haveBounds = true;
                return;
            }
            if(minX < _boundsMinX)
                _boundsMinX = minX;
            if(minY < _boundsMinY)
                _boundsMinY = minY;
            if(maxX > _boundsMaxX)
                _boundsMaxX = maxX;
            if(maxY > _boundsMaxY)
                _boundsMaxY = maxY;
        };

        for(auto &node : _runtime->nodes) {
            node.bounds[0] = 1.0f;
            node.bounds[1] = 1.0f;
            node.bounds[2] = -1.0f;
            node.bounds[3] = -1.0f;

            // Aligned to Player_calcBounds @ 0x6C40B0 (libkrkr2.so):
            //   v30 = 1 << nodeType
            //   v31 = completionType ? 0x1449 : 0x1441
            //   if ((v31 & v30) == 0 || !*(BYTE*)(node+200)) skip
            // The actual native gate is the Path A nodeType mask PLUS
            // renderTreeFlag200 (node+0xC8) — NOT drawFlag (Path B) and
            // NOT drawnThisFrame (node+1944, set by sub_6C2334 but not
            // read here). Port's previous drawFlag/drawnThisFrame reads
            // were both proxies; this is the authoritative gate.
            //
            // NOTE: libkrkr2's calcBounds also has (a) a slot-done byte
            // gate at node+536*slot+344 and (b) recursive handling for
            // nodeType=3 sub-players (0x6C4048) and nodeType=4 particles
            // (0x6C3F08). Port's simplified structure doesn't model those
            // yet; hasSource is the current placeholder for "this node
            // has its own geometry to contribute to the bbox".
            const int visBitmaskCalc = _completionType ? 0x1449 : 0x1441;
            if(((1 << node.nodeType) & visBitmaskCalc) == 0 ||
               !node.renderTreeFlag200) {
                continue;
            }
            if(!node.hasSource) {
                continue;
            }

            bool haveNodeBounds = false;
            double minX = 0.0;
            double minY = 0.0;
            double maxX = 0.0;
            double maxY = 0.0;
            auto extendPoint = [&](double x, double y) {
                if(!haveNodeBounds) {
                    minX = maxX = x;
                    minY = maxY = y;
                    haveNodeBounds = true;
                    return;
                }
                if(x < minX)
                    minX = x;
                if(y < minY)
                    minY = y;
                if(x > maxX)
                    maxX = x;
                if(y > maxY)
                    maxY = y;
            };

            if(!node.meshControlPoints.empty()) {
                for(size_t pi = 0; pi + 1 < node.meshControlPoints.size();
                    pi += 2) {
                    extendPoint(node.meshControlPoints[pi],
                                node.meshControlPoints[pi + 1]);
                }
            } else if(node.clipW > 0.0 || node.clipH > 0.0) {
                for(int ci = 0; ci < 4; ++ci) {
                    extendPoint(node.vertices[ci * 2],
                                node.vertices[ci * 2 + 1]);
                }
            } else {
                extendPoint(node.vertexPosX, node.vertexPosY);
            }

            if(!haveNodeBounds) {
                continue;
            }

            const std::array<float, 4> expectedBounds = {
                static_cast<float>(std::floor(minX)),
                static_cast<float>(std::floor(minY)),
                static_cast<float>(std::ceil(maxX)),
                static_cast<float>(std::ceil(maxY))
            };
            node.bounds[0] = expectedBounds[0];
            node.bounds[1] = expectedBounds[1];
            node.bounds[2] = expectedBounds[2];
            node.bounds[3] = expectedBounds[3];
            mergeBounds(node.bounds[0], node.bounds[1], node.bounds[2],
                        node.bounds[3]);
            if(detail::logoChainTraceEnabled(_runtime->activeMotion)) {
                const std::array<float, 4> actualBounds = { node.bounds[0],
                                                            node.bounds[1],
                                                            node.bounds[2],
                                                            node.bounds[3] };
                bool ok = true;
                for(size_t bi = 0; bi < expectedBounds.size(); ++bi) {
                    if(std::fabs(expectedBounds[bi] - actualBounds[bi]) >
                       0.01f) {
                        ok = false;
                        break;
                    }
                }
                detail::logoChainTraceCheck(
                    motionPath, "calcBounds.node", "0x6C3D04", _clampedEvalTime,
                    fmt::format("from=minmax({:.3f},{:.3f},{:.3f},{:.3f}) "
                                "exp=[{:.3f},{:.3f},{:.3f},{:.3f}]",
                                minX, minY, maxX, maxY, expectedBounds[0],
                                expectedBounds[1], expectedBounds[2],
                                expectedBounds[3]),
                    fmt::format("nodeIndex={} label={} "
                                "act=[{:.3f},{:.3f},{:.3f},{:.3f}]",
                                node.index,
                                node.layerName.empty() ? std::string("<root>")
                                                       : node.layerName,
                                actualBounds[0], actualBounds[1],
                                actualBounds[2], actualBounds[3]),
                    ok, "Player_calcBounds produced an unexpected node AABB");
            }
        }

        for(size_t ni = 1; ni < _runtime->nodes.size(); ++ni) {
            auto &node = _runtime->nodes[ni];
            if(detail::isEmoteLikeMotion(*_runtime)) {
                break;
            }
            if(node.nodeType == 3) {
                if(auto *child = node.getChildPlayer()) {
                    if(gActivePrepareRenderPlayers.count(child) != 0) {
                        continue;
                    }
                    PrepareRenderPlayerScope childScope(child);
                    if(!childScope.active()) {
                        continue;
                    }
                    child->calcBounds();
                    mergeBounds(child->_boundsMinX, child->_boundsMinY,
                                child->_boundsMaxX, child->_boundsMaxY);
                }
            } else if(node.nodeType == 4) {
                const int particleCount = node.getParticleCount();
                for(int pi = 0; pi < particleCount; ++pi) {
                    if(auto *child = node.getParticleChild(pi)) {
                        child->calcBounds();
                        mergeBounds(child->_boundsMinX, child->_boundsMinY,
                                    child->_boundsMaxX, child->_boundsMaxY);
                    }
                }
            }
        }

        if(!haveBounds) {
            _boundsMinX = 0.0;
            _boundsMinY = 0.0;
            _boundsMaxX = 0.0;
            _boundsMaxY = 0.0;
        }
        detail::logoChainTraceLogf(
            motionPath, "calcBounds.player", "0x6C3D04", _clampedEvalTime,
            "playerBounds=({:.3f},{:.3f},{:.3f},{:.3f}) haveBounds={}",
            _boundsMinX, _boundsMinY, _boundsMaxX, _boundsMaxY,
            haveBounds ? 1 : 0);
    }

    void Player::appendPreparedRenderItems() {
        // sub_6D5164 @ 0x6D5178: the first instruction of the libkrkr2.so
        // build+sort wrapper is `if (!*(DWORD*)(player+544)) return 0;`.
        // Port has no explicit `player+544` mirror; the equivalent gate
        // is a null activeMotion, since without a loaded motion there is
        // no render list to build.
        if(!_runtime || !_runtime->activeMotion) {
            return;
        }
        PrepareRenderPlayerScope selfScope(this);
        if(!selfScope.active()) {
            return;
        }

        // Aligned to sub_6C2334 top: clear every node's drawnThisFrame
        // (node+1944) before rebuilding mainList, so downstream consumers
        // like calcBounds see only nodes that entered this frame's list.
        for(auto &node : _runtime->nodes) {
            node.drawnThisFrame = false;
        }

        const bool inheritedFlag18 = _renderItemInheritedFlag18;
        auto &entries = _runtime->preparedRenderItems;
        const auto &nodes = _runtime->nodes;
        const auto motionPath = _runtime->activeMotion->path;
        const bool emoteLikePrepare = detail::isEmoteLikeMotion(*_runtime);
        // 与 updateLayersPhase3_Visibility 一致：emote 须含 nodeType
        // 11（icon/口型）。
        const int bitmask = emoteLikePrepare ? 6153 : 6145;
        const auto &dam = _runtime->drawAffineMatrix;
        std::unordered_set<int> requiredGroupNodeIndices;
        // 参考 sdl3（不编译）：子 motion 节点在子 Player 的 nodeList
        // merged child preparedRenderItems keep foreign nodeIndex values and
        // must not be indexed through this player's nodes[] vector.
        const auto isLocalPreparedItem =
            [runtime = _runtime.get()](
                const detail::PlayerRuntime::PreparedRenderItem &entry)
            -> bool {
            return entry.nativeLifetimeOwner == nullptr ||
                entry.nativeLifetimeOwner == runtime;
        };
        const auto tryGetLocalNode =
            [&nodes](int nodeIndex) -> const detail::MotionNode * {
            if(nodeIndex < 0 ||
               static_cast<size_t>(nodeIndex) >= nodes.size()) {
                return nullptr;
            }
            return &nodes[static_cast<size_t>(nodeIndex)];
        };

        auto climbVisibleAncestorChain = [&](int startNodeIndex, auto &&visit) {
            int ancestorIndex = startNodeIndex;
            for(int guard = 0; ancestorIndex >= 0 && guard < 256; ++guard) {
                const detail::MotionNode *ancestorNode =
                    tryGetLocalNode(ancestorIndex);
                if(!ancestorNode) {
                    break;
                }
                visit(ancestorIndex, *ancestorNode);
                const int nextIndex = ancestorNode->visibleAncestorIndex;
                if(nextIndex == ancestorIndex) {
                    break;
                }
                ancestorIndex = nextIndex;
            }
        };

        auto appendChildEntriesAtCurrentNode = [&](Player *child,
                                                   bool nodePriorDraw) {
            if(!child || !child->_runtime) {
                return;
            }
            if(gActivePrepareRenderPlayers.count(child) != 0) {
                G_PluginLog.warn(
                    "appendPreparedRenderItems: skip cyclic child Player "
                    "motion={}",
                    child->_runtime->activeMotion
                        ? child->_runtime->activeMotion->path
                        : std::string("<none>"));
                return;
            }
            child->_renderItemInheritedFlag18 =
                _renderItemInheritedFlag18 || nodePriorDraw;
            const auto savedChildDrawAffine = child->_runtime->drawAffineMatrix;
            child->_runtime->drawAffineMatrix = dam;
            child->_runtime->preparedRenderItems.clear();
            child->appendPreparedRenderItems();
            child->_runtime->drawAffineMatrix = savedChildDrawAffine;
            auto &childEntries = child->_runtime->preparedRenderItems;
            if(childEntries.empty()) {
                return;
            }
            entries.insert(entries.end(),
                           std::make_move_iterator(childEntries.begin()),
                           std::make_move_iterator(childEntries.end()));
            G_PluginLog.debug(
                "appendPreparedRenderItems: merged {} child render items "
                "(foreign nodeIndex space; 参考 sdl3 子 motion 独立渲染上下文)",
                childEntries.size());
            detail::logoChainTraceLogf(motionPath, "prepare.childMerge",
                                       "0x6C2334/0x6D4F00", _clampedEvalTime,
                                       "childMotionPath={} appendedAtNode={} "
                                       "parentTotalAfterInsert={}",
                                       child->_runtime->activeMotion
                                           ? child->_runtime->activeMotion->path
                                           : std::string("<none>"),
                                       childEntries.size(), entries.size());
            childEntries.clear();
        };

        auto transformPoint = [&](float x, float y) -> tTVPPointD {
            return { dam[0] * static_cast<double>(x) +
                         dam[2] * static_cast<double>(y) + dam[4],
                     dam[1] * static_cast<double>(x) +
                         dam[3] * static_cast<double>(y) + dam[5] };
        };

        constexpr std::array<float, 4> kInvalidPreparedPaintBox = { 1.0f, 1.0f,
                                                                    -1.0f,
                                                                    -1.0f };

        auto restoreNativeFieldLifetime =
            [&](detail::PlayerRuntime::PreparedRenderItem &entry) {
                entry.nativeLifetimeOwner = _runtime.get();
                entry.nativeLifetimeKey = entry.nodeIndex;
                const auto it =
                    _runtime->renderItemNativeFieldLifetimeByNode.find(
                        entry.nativeLifetimeKey);
                if(it == _runtime->renderItemNativeFieldLifetimeByNode.end()) {
                    return;
                }
                // libkrkr2.so 0x6C2334 does not blanket-initialize item+20,
                // item+21, or item+216..228 on every population path. The
                // local item object is reconstructed each frame, so restore
                // the native field lifetime explicitly before 0x6C4E28 writes
                // the subset reached by this frame's branches.
                entry.rawFlag20 = it->second.rawFlag20;
                entry.rawFlag21 = it->second.rawFlag21;
                entry.clipRect = it->second.clipRect;
                entry.dirtyRect = it->second.dirtyRect;
                entry.localCorners = it->second.localCorners;
                entry.localMeshPoints = it->second.localMeshPoints;
            };

        auto updatePaintBox =
            [](detail::PlayerRuntime::PreparedRenderItem &entry, double x,
               double y, bool firstPoint) {
                const float fx = static_cast<float>(x);
                const float fy = static_cast<float>(y);
                if(firstPoint) {
                    entry.paintBox = { fx, fy, fx, fy };
                    return;
                }
                if(fx < entry.paintBox[0])
                    entry.paintBox[0] = fx;
                if(fy < entry.paintBox[1])
                    entry.paintBox[1] = fy;
                if(fx > entry.paintBox[2])
                    entry.paintBox[2] = fx;
                if(fy > entry.paintBox[3])
                    entry.paintBox[3] = fy;
            };

        for(size_t i = 0; i < nodes.size(); ++i) {
            auto &node = _runtime->nodes[i];
            if(!node.accumulated.active)
                continue;
            if(!node.forceVisible && (((1 << node.nodeType) & bitmask) == 0)) {
                if(detail::logoSnapshotMarkEnabledForPath(motionPath) &&
                   motionPath.find("m2logo.mtn") != std::string::npos &&
                   _clampedEvalTime >= 43.0 && _clampedEvalTime <= 50.0 &&
                   (node.index == 18 || node.index == 19)) {
                    std::fprintf(
                        stderr,
                        "SNAPPREPCAND frame=%.3f nodeIndex=%d label=%s "
                        "phase=maskReject forceVisible=%d nodeType=%d "
                        "bitmask=0x%x hasSource=%d currentSrc=%s drawFlag=%d "
                        "visibleAncestorIndex=%d\n",
                        _clampedEvalTime, node.index,
                        node.layerName.empty() ? "<none>"
                                               : node.layerName.c_str(),
                        node.forceVisible, node.nodeType, bitmask,
                        node.hasSource ? 1 : 0,
                        node.interpolatedCache.src.empty()
                            ? "<none>"
                            : node.interpolatedCache.src.c_str(),
                        node.drawFlag ? 1 : 0, node.visibleAncestorIndex);
                }
                continue;
            }
            if(!node.hasSource || node.interpolatedCache.src.empty())
                continue;

            if(detail::logoSnapshotMarkEnabledForPath(motionPath) &&
               motionPath.find("m2logo.mtn") != std::string::npos &&
               _clampedEvalTime >= 43.0 && _clampedEvalTime <= 50.0 &&
               (node.index == 18 || node.index == 19)) {
                std::fprintf(stderr,
                             "SNAPPREPCAND frame=%.3f nodeIndex=%d label=%s "
                             "phase=accept forceVisible=%d nodeType=%d "
                             "bitmask=0x%x hasSource=%d currentSrc=%s "
                             "drawFlag=%d visibleAncestorIndex=%d\n",
                             _clampedEvalTime, node.index,
                             node.layerName.empty() ? "<none>"
                                                    : node.layerName.c_str(),
                             node.forceVisible, node.nodeType, bitmask,
                             node.hasSource ? 1 : 0,
                             node.interpolatedCache.src.empty()
                                 ? "<none>"
                                 : node.interpolatedCache.src.c_str(),
                             node.drawFlag ? 1 : 0, node.visibleAncestorIndex);
            }

            // libkrkr2.so sub_6C2334 (0x6C2334):
            // leaf items always take node+1952->1904 as their parent item
            // pointer, allocating that synthetic ancestor item on demand even
            // when the ancestor itself has no source-backed render item.
            if(node.visibleAncestorIndex >= 0 &&
               node.visibleAncestorIndex != node.index &&
               tryGetLocalNode(node.visibleAncestorIndex) != nullptr) {
                requiredGroupNodeIndices.insert(node.visibleAncestorIndex);
            }

            if(const detail::MotionNode *ancestorNode =
                   tryGetLocalNode(node.visibleAncestorIndex)) {
                climbVisibleAncestorChain(
                    ancestorNode->visibleAncestorIndex,
                    [&](int ancestorIndex, const detail::MotionNode &ancestor) {
                        const bool isSpecialCompositeParent =
                            ancestor.nodeType == 12 &&
                            (ancestor.stencilType & 4) != 0;
                        if(isSpecialCompositeParent) {
                            requiredGroupNodeIndices.insert(ancestorIndex);
                        }
                    });
            }
        }

        for(size_t i = 0; i < nodes.size(); ++i) {
            const auto &node = nodes[i];
            if(!node.accumulated.active)
                continue;
            if(!_preview) {
                // sdl3: e-mote 单棵树 walk；不 libkrkr2 式 nodeType=3 子 Player
                // prepare 合并（NEKOPARA 頭部変形基礎 会触发数百次子
                // prepare）。
                if(!emoteLikePrepare) {
                    if(node.nodeType == 3) {
                        appendChildEntriesAtCurrentNode(node.getChildPlayer(),
                                                        node.priorDraw != 0);
                    } else if(node.nodeType == 4) {
                        const int particleCount = node.getParticleCount();
                        for(int pi = 0; pi < particleCount; ++pi) {
                            appendChildEntriesAtCurrentNode(
                                node.getParticleChild(pi), node.priorDraw != 0);
                        }
                    }
                }
            }
            const bool hasOwnSource =
                node.hasSource && !node.interpolatedCache.src.empty();
            const bool needsGroupEntry =
                requiredGroupNodeIndices.find(static_cast<int>(i)) !=
                requiredGroupNodeIndices.end();
            const bool syntheticParentOnly = needsGroupEntry && !hasOwnSource &&
                !node.forceVisible && (((1 << node.nodeType) & bitmask) == 0);
            if(!needsGroupEntry && !node.forceVisible &&
               (((1 << node.nodeType) & bitmask) == 0)) {
                continue;
            }
            if(!hasOwnSource && !needsGroupEntry)
                continue;

            detail::PlayerRuntime::PreparedRenderItem entry;
            entry.nodeIndex = static_cast<int>(i);
            restoreNativeFieldLifetime(entry);
            entry.hasOwnSource = hasOwnSource;
            entry.groupOnly = !hasOwnSource && needsGroupEntry;
            entry.topLevelList = true;
            entry.groupList = false;
            entry.selfSeedChildList = false;
            if(hasOwnSource) {
                entry.sourceKey = node.interpolatedCache.src;
                entry.srcRef = findSource(detail::widen(entry.sourceKey));
            }
            // Aligned to sub_6D5164 -> sub_6C2334:
            // top-level build uses arg4=0, so render-item draw flag becomes
            // node+1960 ? 1 : node+1961. node+1961 is the post-build
            // stencilComposite mask-layer reference flag.
            entry.drawFlag = node.drawFlag ||
                node.stencilCompositeMaskReferenced || needsGroupEntry;
            entry.rawFlag16 = node.renderTreeFlag201;
            const int skipTypeBitmask =
                emoteLikePrepare ? 6153 : (_preview ? 1097 : 1089);
            entry.skipFlag0 = ((skipTypeBitmask & (1 << node.nodeType)) == 0);
            entry.skipFlag1 = !(inheritedFlag18 || (node.priorDraw != 0));
            // libkrkr2.so sub_6C2334 copies player+1012 straight into item+248.
            // Keep that variant explicit in the local render item instead of
            // folding it away completely.
            entry.contextVariant = _findMotionContextVariant;
            entry.layerId = node.layerId1;
            entry.layerId2 = node.layerId2;

            if(syntheticParentOnly) {
                // libkrkr2.so sub_6C2334 (0x6C2334):
                // when a leaf item references node+1952->1904, the ancestor
                // item is allocated as a zero-initialized 0x1B0 container.
                // It is not populated from the ancestor node's viewport/parent
                // chain at allocation time. Keep the local synthetic parent as
                // a neutral container and let child union/build steps fill it.
                entry.blendMode = 0;
                entry.paintBox = { 1.0f, 1.0f, -1.0f, -1.0f };
                entry.viewport = { 1.0f, 1.0f, -1.0f, -1.0f };
                entry.hasViewport = false;
                entry.visibleAncestorIndex = -1;
                const bool specialCompositeParent =
                    node.nodeType == 12 && (node.stencilType & 4) != 0;
                if(specialCompositeParent) {
                    // libkrkr2.so 0x6C3740..0x6C37E4:
                    // special type12 parents are inserted into the auxiliary
                    // render list (a3) and seeded into their own item+24 child
                    // vector. They are not inserted into the main top-level
                    // list at this point.
                    entry.topLevelList = false;
                    entry.groupList = true;
                    entry.selfSeedChildList = true;
                }
            } else {
                // Aligned to libkrkr2.so sub_6C2334 (0x6C2334):
                // render item +0x40 stores node accumulated posZ (node+0x5F8),
                // while coordinateMode and objTriPriority are copied into
                // separate integer fields (+0xEC/+0xF0).
                entry.sortKey = node.accumulated.posZ;
                entry.blendMode = node.accumulated.blendMode;
                entry.packedColors = copyPackedColorsFromBytes(node.colorBytes);
                entry.opacity = node.accumulated.opacity;
                entry.stencilComposite = node.stencilType;
                entry.coordinateMode = node.coordinateMode;
                entry.objTriPriority = node.objTriPriority;
                entry.visibleAncestorIndex = node.visibleAncestorIndex;
                entry.meshType = node.meshType;
                entry.meshDivX = node.meshDivX;
                entry.meshDivY = node.meshDivY;
            }

            bool havePaintBox = false;
            if(hasOwnSource && node.clipW > 0.0 && node.clipH > 0.0) {
                for(int ci = 0; ci < 4; ++ci) {
                    const auto pt = transformPoint(node.vertices[ci * 2],
                                                   node.vertices[ci * 2 + 1]);
                    entry.corners[ci * 2] = static_cast<float>(pt.x);
                    entry.corners[ci * 2 + 1] = static_cast<float>(pt.y);
                    updatePaintBox(entry, pt.x, pt.y, !havePaintBox);
                    havePaintBox = true;
                }
            }

            if(hasOwnSource && !node.meshControlPoints.empty()) {
                entry.meshPoints.resize(node.meshControlPoints.size());
                for(size_t pi = 0; pi + 1 < node.meshControlPoints.size();
                    pi += 2) {
                    const auto pt =
                        transformPoint(node.meshControlPoints[pi],
                                       node.meshControlPoints[pi + 1]);
                    entry.meshPoints[pi] = static_cast<float>(pt.x);
                    entry.meshPoints[pi + 1] = static_cast<float>(pt.y);
                    updatePaintBox(entry, pt.x, pt.y, !havePaintBox);
                    havePaintBox = true;
                }
            }

            if(!havePaintBox && hasOwnSource &&
               node.bounds[2] >= node.bounds[0] &&
               node.bounds[3] >= node.bounds[1]) {
                const auto p0 = transformPoint(node.bounds[0], node.bounds[1]);
                const auto p1 = transformPoint(node.bounds[2], node.bounds[1]);
                const auto p2 = transformPoint(node.bounds[2], node.bounds[3]);
                const auto p3 = transformPoint(node.bounds[0], node.bounds[3]);
                entry.paintBox = {
                    static_cast<float>(std::floor(
                        std::min(std::min(p0.x, p1.x), std::min(p2.x, p3.x)))),
                    static_cast<float>(std::floor(
                        std::min(std::min(p0.y, p1.y), std::min(p2.y, p3.y)))),
                    static_cast<float>(std::ceil(
                        std::max(std::max(p0.x, p1.x), std::max(p2.x, p3.x)))),
                    static_cast<float>(std::ceil(
                        std::max(std::max(p0.y, p1.y), std::max(p2.y, p3.y))))
                };
                havePaintBox = true;
            }

            if(!havePaintBox) {
                // libkrkr2.so sub_6C2334 initializes item+200..212 from
                // node+1936 when present, otherwise from xmmword_14D7C60.
                // That default is {1,1,-1,-1}, i.e. an invalid rect sentinel,
                // not a point box at vertexPos. Group-only items rely on this
                // invalid sentinel so the later child-union pass can replace
                // the parent paintBox with the first real child bounds instead
                // of being permanently anchored at (0,0).
                entry.paintBox = kInvalidPreparedPaintBox;
            } else {
                entry.paintBox = {
                    static_cast<float>(std::floor(entry.paintBox[0])),
                    static_cast<float>(std::floor(entry.paintBox[1])),
                    static_cast<float>(std::ceil(entry.paintBox[2])),
                    static_cast<float>(std::ceil(entry.paintBox[3]))
                };
            }

            if(!syntheticParentOnly && node.parentClipIndex >= 0 &&
               node.parentClipIndex < static_cast<int>(nodes.size())) {
                const auto &clipNode = nodes[node.parentClipIndex];
                if(clipNode.shapeAABB[2] >= clipNode.shapeAABB[0] &&
                   clipNode.shapeAABB[3] >= clipNode.shapeAABB[1]) {
                    const auto p0 = transformPoint(clipNode.shapeAABB[0],
                                                   clipNode.shapeAABB[1]);
                    const auto p1 = transformPoint(clipNode.shapeAABB[2],
                                                   clipNode.shapeAABB[1]);
                    const auto p2 = transformPoint(clipNode.shapeAABB[2],
                                                   clipNode.shapeAABB[3]);
                    const auto p3 = transformPoint(clipNode.shapeAABB[0],
                                                   clipNode.shapeAABB[3]);
                    entry.viewport = {
                        static_cast<float>(std::min(std::min(p0.x, p1.x),
                                                    std::min(p2.x, p3.x))),
                        static_cast<float>(std::min(std::min(p0.y, p1.y),
                                                    std::min(p2.y, p3.y))),
                        static_cast<float>(std::max(std::max(p0.x, p1.x),
                                                    std::max(p2.x, p3.x))),
                        static_cast<float>(std::max(std::max(p0.y, p1.y),
                                                    std::max(p2.y, p3.y)))
                    };
                    entry.hasViewport = true;
                }
            }

            if(detail::logoChainTraceEnabled(_runtime->activeMotion)) {
                const auto motionPath = _runtime->activeMotion->path;
                const std::array<float, 8> expectedCorners = {
                    static_cast<float>(
                        dam[0] * static_cast<double>(node.vertices[0]) +
                        dam[2] * static_cast<double>(node.vertices[1]) +
                        dam[4]),
                    static_cast<float>(
                        dam[1] * static_cast<double>(node.vertices[0]) +
                        dam[3] * static_cast<double>(node.vertices[1]) +
                        dam[5]),
                    static_cast<float>(
                        dam[0] * static_cast<double>(node.vertices[2]) +
                        dam[2] * static_cast<double>(node.vertices[3]) +
                        dam[4]),
                    static_cast<float>(
                        dam[1] * static_cast<double>(node.vertices[2]) +
                        dam[3] * static_cast<double>(node.vertices[3]) +
                        dam[5]),
                    static_cast<float>(
                        dam[0] * static_cast<double>(node.vertices[4]) +
                        dam[2] * static_cast<double>(node.vertices[5]) +
                        dam[4]),
                    static_cast<float>(
                        dam[1] * static_cast<double>(node.vertices[4]) +
                        dam[3] * static_cast<double>(node.vertices[5]) +
                        dam[5]),
                    static_cast<float>(
                        dam[0] * static_cast<double>(node.vertices[6]) +
                        dam[2] * static_cast<double>(node.vertices[7]) +
                        dam[4]),
                    static_cast<float>(
                        dam[1] * static_cast<double>(node.vertices[6]) +
                        dam[3] * static_cast<double>(node.vertices[7]) + dam[5])
                };
                const auto effectiveColor =
                    unpackPackedRgba(entry.packedColors[0]);
                detail::logoChainTraceLogf(
                    motionPath, "prepare.item", "0x6C2334", _clampedEvalTime,
                    "nodeIndex={} src={} blend={} opacity={} "
                    "packedColor=[0x{:08x},0x{:08x},0x{:08x},0x{:08x}] "
                    "effectiveColor=[{},{},{},{}] meshType={} meshDiv=({},{}) "
                    "sortKey={:.3f} coordinateMode={} objTriPriority={} "
                    "layerId=({}, {}) nodeDrawFlag={} maskRef={} "
                    "itemDrawFlag={} visibleAncestorIndex={} slotDone={} "
                    "frameType={} stencilBase={} stencilType={}",
                    entry.nodeIndex,
                    entry.sourceKey.empty() ? std::string("<none>")
                                            : entry.sourceKey,
                    entry.blendMode, entry.opacity, entry.packedColors[0],
                    entry.packedColors[1], entry.packedColors[2],
                    entry.packedColors[3], effectiveColor[0], effectiveColor[1],
                    effectiveColor[2], effectiveColor[3], entry.meshType,
                    entry.meshDivX, entry.meshDivY, entry.sortKey,
                    entry.coordinateMode, entry.objTriPriority, entry.layerId,
                    entry.layerId2, node.drawFlag ? 1 : 0,
                    node.stencilCompositeMaskReferenced ? 1 : 0,
                    entry.drawFlag ? 1 : 0, entry.visibleAncestorIndex,
                    node.activeSlot().done ? 1 : 0, node.currentFrameType,
                    node.stencilTypeBase, node.stencilType);
                bool cornersOk = node.clipW <= 0.0 && node.clipH <= 0.0;
                if(!cornersOk) {
                    cornersOk = true;
                    for(size_t ci = 0; ci < expectedCorners.size(); ++ci) {
                        if(std::fabs(entry.corners[ci] - expectedCorners[ci]) >
                           0.01f) {
                            cornersOk = false;
                            break;
                        }
                    }
                }
                detail::logoChainTraceCheck(
                    motionPath, "prepare.corners", "0x6C2334", _clampedEvalTime,
                    fmt::format("drawAffine*vertices "
                                "exp=[{:.3f},{:.3f},{:.3f},{:.3f},{:.3f},{:.3f}"
                                ",{:.3f},{:.3f}]",
                                expectedCorners[0], expectedCorners[1],
                                expectedCorners[2], expectedCorners[3],
                                expectedCorners[4], expectedCorners[5],
                                expectedCorners[6], expectedCorners[7]),
                    fmt::format(
                        "nodeIndex={} "
                        "act=[{:.3f},{:.3f},{:.3f},{:.3f},{:.3f},{:.3f},{:.3f},"
                        "{:.3f}]",
                        entry.nodeIndex, entry.corners[0], entry.corners[1],
                        entry.corners[2], entry.corners[3], entry.corners[4],
                        entry.corners[5], entry.corners[6], entry.corners[7]),
                    cornersOk,
                    "PreparedRenderItem corners diverged from drawAffineMatrix "
                    "* node.vertices");
                detail::logoChainTraceCheck(
                    motionPath, "prepare.paintBox", "0x6C2334",
                    _clampedEvalTime,
                    fmt::format("paintBox from transformed geometry "
                                "exp=[{:.3f},{:.3f},{:.3f},{:.3f}]",
                                entry.paintBox[0], entry.paintBox[1],
                                entry.paintBox[2], entry.paintBox[3]),
                    fmt::format(
                        "nodeIndex={} act=[{:.3f},{:.3f},{:.3f},{:.3f}]",
                        entry.nodeIndex, entry.paintBox[0], entry.paintBox[1],
                        entry.paintBox[2], entry.paintBox[3]),
                    true,
                    "PreparedRenderItem paintBox diverged from transformed "
                    "geometry");
                detail::logoChainTraceCheck(
                    motionPath, "prepare.viewport", "0x6C2334",
                    _clampedEvalTime,
                    entry.hasViewport
                        ? fmt::format("parent shapeAABB chain "
                                      "exp=[{:.3f},{:.3f},{:.3f},{:.3f}]",
                                      entry.viewport[0], entry.viewport[1],
                                      entry.viewport[2], entry.viewport[3])
                        : std::string(
                              "parent shapeAABB chain exp=<invalid default>"),
                    entry.hasViewport
                        ? fmt::format(
                              "nodeIndex={} act=[{:.3f},{:.3f},{:.3f},{:.3f}]",
                              entry.nodeIndex, entry.viewport[0],
                              entry.viewport[1], entry.viewport[2],
                              entry.viewport[3])
                        : fmt::format("nodeIndex={} act=<invalid default>",
                                      entry.nodeIndex),
                    true,
                    "PreparedRenderItem viewport propagation diverged from "
                    "parent clip chain");
            }

            if(detail::logoSnapshotMarkEnabledForPath(motionPath) &&
               motionPath.find("m2logo.mtn") != std::string::npos &&
               _clampedEvalTime >= 43.0 && _clampedEvalTime <= 50.0 &&
               (entry.nodeIndex == 14 || entry.nodeIndex == 15 ||
                entry.nodeIndex == 19 ||
                (entry.nodeIndex >= 20 && entry.nodeIndex <= 29))) {
                std::fprintf(
                    stderr,
                    "SNAPPREP frame=%.3f nodeIndex=%d source=%s "
                    "hasOwnSource=%d groupOnly=%d rawFlags=[%d,%d,%d,%d] "
                    "priorDraw=%d inherited18=%d sortKey=%.3f "
                    "coordinateMode=%d objTriPriority=%d "
                    "visibleAncestorIndex=%d drawFlag=%d opacity=%d "
                    "paintBox=[%.1f,%.1f,%.1f,%.1f] viewport=%s\n",
                    _clampedEvalTime, entry.nodeIndex,
                    entry.sourceKey.empty() ? "<none>"
                                            : entry.sourceKey.c_str(),
                    entry.hasOwnSource ? 1 : 0, entry.groupOnly ? 1 : 0,
                    entry.rawFlag16 ? 1 : 0, entry.skipFlag0 ? 1 : 0,
                    entry.skipFlag1 ? 0 : 1, entry.drawFlag ? 1 : 0,
                    node.priorDraw, inheritedFlag18 ? 1 : 0, entry.sortKey,
                    entry.coordinateMode, entry.objTriPriority,
                    entry.visibleAncestorIndex, entry.drawFlag ? 1 : 0,
                    entry.opacity, entry.paintBox[0], entry.paintBox[1],
                    entry.paintBox[2], entry.paintBox[3],
                    entry.hasViewport
                        ? fmt::format("[{:.1f},{:.1f},{:.1f},{:.1f}]",
                                      entry.viewport[0], entry.viewport[1],
                                      entry.viewport[2], entry.viewport[3])
                              .c_str()
                        : "<invalid>");
            }

            // Aligned to sub_6C2334 mainList enqueue: this node is now in
            // the Path A render list; mark it so downstream consumers
            // (e.g. calcBounds) can distinguish Path A presence from the
            // Path B drawFlag.
            _runtime->nodes[i].drawnThisFrame = true;

            entries.push_back(std::move(entry));
        }

        if(entries.empty()) {
            return;
        }

        std::unordered_map<int, size_t> entryIndexByNode;
        entryIndexByNode.reserve(entries.size());
        for(size_t i = 0; i < entries.size(); ++i) {
            if(isLocalPreparedItem(entries[i])) {
                entryIndexByNode.emplace(entries[i].nodeIndex, i);
            }
        }

        auto unionPaintBox =
            [](detail::PlayerRuntime::PreparedRenderItem &parent,
               const detail::PlayerRuntime::PreparedRenderItem &child) {
                if(child.paintBox[2] < child.paintBox[0] ||
                   child.paintBox[3] < child.paintBox[1]) {
                    return;
                }
                if(parent.paintBox[2] < parent.paintBox[0] ||
                   parent.paintBox[3] < parent.paintBox[1]) {
                    parent.paintBox = child.paintBox;
                    return;
                }
                parent.paintBox[0] =
                    std::min(parent.paintBox[0], child.paintBox[0]);
                parent.paintBox[1] =
                    std::min(parent.paintBox[1], child.paintBox[1]);
                parent.paintBox[2] =
                    std::max(parent.paintBox[2], child.paintBox[2]);
                parent.paintBox[3] =
                    std::max(parent.paintBox[3], child.paintBox[3]);
            };

        for(const auto &childEntry : entries) {
            if(!isLocalPreparedItem(childEntry)) {
                continue;
            }
            for(int ancestorIndex = childEntry.visibleAncestorIndex;
                ancestorIndex >= 0;) {
                if(!tryGetLocalNode(ancestorIndex)) {
                    break;
                }
                const auto parentIt = entryIndexByNode.find(ancestorIndex);
                if(parentIt == entryIndexByNode.end()) {
                    break;
                }
                auto &parentEntry = entries[parentIt->second];
                if(!isLocalPreparedItem(parentEntry)) {
                    break;
                }
                const detail::MotionNode *ancestorNode =
                    tryGetLocalNode(parentEntry.nodeIndex);
                if(!ancestorNode) {
                    break;
                }
                unionPaintBox(parentEntry, childEntry);
                const int nextAncestorIndex =
                    ancestorNode->visibleAncestorIndex;
                if(nextAncestorIndex == ancestorIndex) {
                    break;
                }
                ancestorIndex = nextAncestorIndex;
            }
        }
    }

    bool Player::prepareRenderItems(bool inheritedFlag18) {
#if defined(KRKR2_WASMTIME_HEADLESS)
        detail::motionTraceRenderPrepareEnter(this);
#endif
        if(!_runtime) {
#if defined(KRKR2_WASMTIME_HEADLESS)
            detail::motionTraceRenderPrepareLeave(this, false);
#endif
            return false;
        }

        const bool savedInheritedFlag18 = _renderItemInheritedFlag18;
        _renderItemInheritedFlag18 = inheritedFlag18;
        _runtime->preparedRenderItems.clear();
        const auto motionPath = _runtime->activeMotion
            ? _runtime->activeMotion->path
            : std::string{};
        // Merged child-player items carry nodeIndex into the child's nodes[]
        // vector. Never dereference those indices against this player's
        // nodes[].
        const auto isLocalPreparedItem =
            [runtime = _runtime.get()](
                const detail::PlayerRuntime::PreparedRenderItem &entry)
            -> bool {
            return entry.nativeLifetimeOwner == nullptr ||
                entry.nativeLifetimeOwner == runtime;
        };
        const auto tryGetLocalNode =
            [this](int nodeIndex) -> const detail::MotionNode * {
            if(!_runtime || nodeIndex < 0 ||
               static_cast<size_t>(nodeIndex) >= _runtime->nodes.size()) {
                return nullptr;
            }
            return &_runtime->nodes[static_cast<size_t>(nodeIndex)];
        };

#if defined(KRKR2_WASMTIME_HEADLESS)
        detail::motionTraceRenderBuildItemsEnter(this);
#endif
        appendPreparedRenderItems();
        std::vector<double> beforeSortKeys;
        beforeSortKeys.reserve(_runtime->preparedRenderItems.size());
        for(const auto &item : _runtime->preparedRenderItems) {
            beforeSortKeys.push_back(item.sortKey);
        }
        // Aligned to sub_6D4F00 (0x6D4F00): compare render-item sort key.
        std::stable_sort(
            _runtime->preparedRenderItems.begin(),
            _runtime->preparedRenderItems.end(),
            [](const detail::PlayerRuntime::PreparedRenderItem &lhs,
               const detail::PlayerRuntime::PreparedRenderItem &rhs) {
                return lhs.sortKey < rhs.sortKey;
            });
        if(detail::logoChainTraceEnabled(_runtime->activeMotion)) {
            std::ostringstream beforeSort;
            std::ostringstream afterSort;
            for(size_t i = 0; i < beforeSortKeys.size(); ++i) {
                if(i)
                    beforeSort << ",";
                beforeSort << beforeSortKeys[i];
            }
            for(size_t i = 0; i < _runtime->preparedRenderItems.size(); ++i) {
                if(i)
                    afterSort << ",";
                afterSort << _runtime->preparedRenderItems[i].sortKey;
            }
            detail::logoChainTraceLogf(
                motionPath, "prepare.sort", "0x6D5164/0x6D4F00",
                _clampedEvalTime,
                "itemCount={} sortKeysBefore=[{}] sortKeysAfter=[{}]",
                _runtime->preparedRenderItems.size(), beforeSort.str(),
                afterSort.str());
        }

        std::unordered_map<int, detail::PlayerRuntime::PreparedRenderItem *>
            entryPtrByNode;
        entryPtrByNode.reserve(_runtime->preparedRenderItems.size());
        for(auto &item : _runtime->preparedRenderItems) {
            item.parentItem = nullptr;
            item.childItems.clear();
            if(isLocalPreparedItem(item)) {
                entryPtrByNode.emplace(item.nodeIndex, &item);
            }
        }
        for(auto &item : _runtime->preparedRenderItems) {
            if(item.selfSeedChildList) {
                item.childItems.push_back(&item);
            }
        }
        // Aligned to libkrkr2.so sub_6C2334: item+264 (parentItem) is written
        // in exactly two places — Branch A for nodeType=3 sub-player wrappers
        // (0x6c2b28) and the type12 composite aggregation branch (0x6c2654).
        // Normal type0 / type1 / type2 layers — including transform-only
        // parents like yuzulogo's `slide` nodes — **never** see their item+264
        // populated by the binary, so their visibleAncestor chain stays at the
        // render command level only (item+24 children vector is untouched).
        //
        // The previous generic "assign parentItem for any visibleAncestor" loop
        // caused moji_y/u/z/... to inherit `hasRenderParent=true` from their
        // slide parents, which the top-level execute filter at
        // PlayerRender.cpp:1851 then skipped — leaving the letters unrendered
        // despite correct updateLayers data. The type12 composite case is
        // handled further below at the selfSeedChildList loop, which matches
        // Branch 0x6C3740..0x6C3924 / sub_6F3424.

        // Align to 0x6C3800..0x6C3924 and sub_6F3424:
        // under a special type12 composite parent, nodeType 0 children are
        // appended directly; nodeType 3 children append directly only in
        // preview mode, otherwise their child vector is spliced into the
        // parent's child vector.
        for(auto &parentItem : _runtime->preparedRenderItems) {
            if(!parentItem.selfSeedChildList ||
               !isLocalPreparedItem(parentItem)) {
                continue;
            }
            const auto parentNodeIndex = parentItem.nodeIndex;
            for(auto &candidate : _runtime->preparedRenderItems) {
                if(candidate.nodeIndex == parentNodeIndex) {
                    continue;
                }
                if(candidate.visibleAncestorIndex != parentNodeIndex) {
                    continue;
                }
                if(!isLocalPreparedItem(candidate)) {
                    continue;
                }
                const detail::MotionNode *candidateNode =
                    tryGetLocalNode(candidate.nodeIndex);
                if(!candidateNode) {
                    G_PluginLog.warn(
                        "prepareRenderItems: skip composite candidate "
                        "nodeIndex={} (nodes.size={})",
                        candidate.nodeIndex, _runtime->nodes.size());
                    continue;
                }
                if(candidateNode->nodeType == 0) {
                    candidate.parentItem = &parentItem;
                    parentItem.childItems.push_back(&candidate);
                    continue;
                }
                if(candidateNode->nodeType != 3) {
                    continue;
                }
                if(_preview) {
                    candidate.parentItem = &parentItem;
                    parentItem.childItems.push_back(&candidate);
                } else {
                    for(auto *grandChild : candidate.childItems) {
                        if(!grandChild) {
                            continue;
                        }
                        grandChild->parentItem = &parentItem;
                        parentItem.childItems.push_back(grandChild);
                    }
                }
            }
        }

        // Branch A — aligned to sub_6C2334 @ 0x6C2B28 (libkrkr2.so).
        // nodeType=3 sub-player wrappers get their item+264 (parentItem)
        // populated with the visibleAncestor's PreparedRenderItem pointer,
        // forming the sub-player ancestor chain that sub_6C7440 reads to
        // decide direct vs composed render path. This is distinct from the
        // type12 composite aggregation above (item+24 children collection).
        // Only fill when parentItem is still nullptr so the type12 composite
        // writes above remain authoritative for their covered cases. See
        // analysis/RenderPipeline_Path_A_ImplRef.md §7.3.
        for(auto &entry : _runtime->preparedRenderItems) {
            if(entry.parentItem != nullptr || !isLocalPreparedItem(entry)) {
                continue;
            }
            const detail::MotionNode *entryNode =
                tryGetLocalNode(entry.nodeIndex);
            if(!entryNode) {
                continue;
            }
            if(entryNode->nodeType != 3) {
                continue;
            }
            const int va = entry.visibleAncestorIndex;
            if(va < 0) {
                continue;
            }
            auto it = entryPtrByNode.find(va);
            if(it == entryPtrByNode.end()) {
                continue;
            }
            entry.parentItem = it->second;
        }
#if defined(KRKR2_WASMTIME_HEADLESS)
        detail::motionTraceRenderBuildItemsLeave(this);
#endif

        if(detail::logoSnapshotMarkEnabledForPath(motionPath) &&
           motionPath.find("m2logo.mtn") != std::string::npos &&
           _clampedEvalTime >= 43.0 && _clampedEvalTime <= 50.0) {
            for(size_t i = 0; i < _runtime->preparedRenderItems.size(); ++i) {
                const auto &item = _runtime->preparedRenderItems[i];
                if(!(item.nodeIndex == 14 || item.nodeIndex == 15 ||
                     item.nodeIndex == 19 ||
                     (item.nodeIndex >= 20 && item.nodeIndex <= 29))) {
                    continue;
                }
                std::fprintf(
                    stderr,
                    "SNAPPREPORDER frame=%.3f order=%zu nodeIndex=%d source=%s "
                    "groupOnly=%d topLevel=%d groupList=%d selfSeed=%d "
                    "sortKey=%.3f visibleAncestorIndex=%d parentNodeIndex=%d "
                    "childCount=%zu coordinateMode=%d objTriPriority=%d\n",
                    _clampedEvalTime, i, item.nodeIndex,
                    item.sourceKey.empty() ? "<none>" : item.sourceKey.c_str(),
                    item.groupOnly ? 1 : 0, item.topLevelList ? 1 : 0,
                    item.groupList ? 1 : 0, item.selfSeedChildList ? 1 : 0,
                    item.sortKey, item.visibleAncestorIndex,
                    item.parentItem ? item.parentItem->nodeIndex : -1,
                    item.childItems.size(), item.coordinateMode,
                    item.objTriPriority);
            }
        }
        _renderItemInheritedFlag18 = savedInheritedFlag18;
        const bool ok = !_runtime->preparedRenderItems.empty();
#if defined(KRKR2_WASMTIME_HEADLESS)
        detail::motionTraceRenderPrepareLeave(this, ok);
#endif
        return ok;
    }

    void Player::applyPreparedRenderItemTranslateOffsets() {
#if defined(KRKR2_WASMTIME_HEADLESS)
        detail::motionTraceRenderApplyTranslateEnter(this);
#endif
        if(!_runtime) {
#if defined(KRKR2_WASMTIME_HEADLESS)
            detail::motionTraceRenderApplyTranslateLeave(this);
#endif
            return;
        }

        // Aligned to libkrkr2.so Player_applyTranslateOffset (0x6D5264):
        // normal path adds cameraOffset to prepared render items here.
        // Root position is already baked into node state during updateLayers.
        const double ofsX = static_cast<double>(_cameraOffsetX);
        const double ofsY = static_cast<double>(_cameraOffsetY);
        const auto motionPath = _runtime->activeMotion
            ? _runtime->activeMotion->path
            : std::string{};
        for(auto &entry : _runtime->preparedRenderItems) {
            const auto beforeCorners = entry.corners;
            const auto beforePaintBox = entry.paintBox;
            const auto beforeViewport = entry.viewport;
            const auto beforeMeshPoints = entry.meshPoints;
            for(size_t ci = 0; ci < entry.corners.size(); ci += 2) {
                entry.corners[ci] = static_cast<float>(
                    static_cast<double>(entry.corners[ci]) + ofsX);
                entry.corners[ci + 1] = static_cast<float>(
                    static_cast<double>(entry.corners[ci + 1]) + ofsY);
            }
            entry.paintBox[0] = static_cast<float>(
                static_cast<double>(entry.paintBox[0]) + ofsX);
            entry.paintBox[1] = static_cast<float>(
                static_cast<double>(entry.paintBox[1]) + ofsY);
            entry.paintBox[2] = static_cast<float>(
                static_cast<double>(entry.paintBox[2]) + ofsX);
            entry.paintBox[3] = static_cast<float>(
                static_cast<double>(entry.paintBox[3]) + ofsY);
            if(entry.hasViewport) {
                entry.viewport[0] = static_cast<float>(
                    static_cast<double>(entry.viewport[0]) + ofsX);
                entry.viewport[1] = static_cast<float>(
                    static_cast<double>(entry.viewport[1]) + ofsY);
                entry.viewport[2] = static_cast<float>(
                    static_cast<double>(entry.viewport[2]) + ofsX);
                entry.viewport[3] = static_cast<float>(
                    static_cast<double>(entry.viewport[3]) + ofsY);
            }
            for(size_t pi = 0; pi + 1 < entry.meshPoints.size(); pi += 2) {
                entry.meshPoints[pi] = static_cast<float>(
                    static_cast<double>(entry.meshPoints[pi]) + ofsX);
                entry.meshPoints[pi + 1] = static_cast<float>(
                    static_cast<double>(entry.meshPoints[pi + 1]) + ofsY);
            }
            if(detail::logoChainTraceEnabled(_runtime->activeMotion)) {
                bool ok = true;
                for(size_t ci = 0; ci < entry.corners.size(); ci += 2) {
                    if(std::fabs((entry.corners[ci] - beforeCorners[ci]) -
                                 static_cast<float>(ofsX)) > 0.01f ||
                       std::fabs(
                           (entry.corners[ci + 1] - beforeCorners[ci + 1]) -
                           static_cast<float>(ofsY)) > 0.01f) {
                        ok = false;
                        break;
                    }
                }
                if(ok && entry.hasViewport) {
                    for(size_t vi = 0; vi < entry.viewport.size(); vi += 2) {
                        if(std::fabs((entry.viewport[vi] - beforeViewport[vi]) -
                                     static_cast<float>(ofsX)) > 0.01f ||
                           std::fabs((entry.viewport[vi + 1] -
                                      beforeViewport[vi + 1]) -
                                     static_cast<float>(ofsY)) > 0.01f) {
                            ok = false;
                            break;
                        }
                    }
                }
                if(ok) {
                    for(size_t pi = 0; pi + 1 < entry.meshPoints.size();
                        pi += 2) {
                        if(std::fabs(
                               (entry.meshPoints[pi] - beforeMeshPoints[pi]) -
                               static_cast<float>(ofsX)) > 0.01f ||
                           std::fabs((entry.meshPoints[pi + 1] -
                                      beforeMeshPoints[pi + 1]) -
                                     static_cast<float>(ofsY)) > 0.01f) {
                            ok = false;
                            break;
                        }
                    }
                }
                detail::logoChainTraceCheck(
                    motionPath, "prepare.translate", "0x6D5264",
                    _clampedEvalTime,
                    fmt::format("cameraOffset=({:.3f},{:.3f}) applied to "
                                "corners/paintBox/viewport/mesh",
                                ofsX, ofsY),
                    fmt::format(
                        "nodeIndex={} beforeCorner0=({:.3f},{:.3f}) "
                        "afterCorner0=({:.3f},{:.3f}) "
                        "beforePaintBox=[{:.3f},{:.3f},{:.3f},{:.3f}] "
                        "afterPaintBox=[{:.3f},{:.3f},{:.3f},{:.3f}]",
                        entry.nodeIndex, beforeCorners[0], beforeCorners[1],
                        entry.corners[0], entry.corners[1], beforePaintBox[0],
                        beforePaintBox[1], beforePaintBox[2], beforePaintBox[3],
                        entry.paintBox[0], entry.paintBox[1], entry.paintBox[2],
                        entry.paintBox[3]),
                    ok,
                    "Player_applyTranslateOffset added more than cameraOffset");
            }
        }
#if defined(KRKR2_WASMTIME_HEADLESS)
        detail::motionTraceRenderApplyTranslateLeave(this);
#endif
    }

} // namespace motion
