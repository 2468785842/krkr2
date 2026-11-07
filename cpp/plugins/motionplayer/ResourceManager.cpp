//
// Created by LiDon on 2025/9/15.
//

#include <vector>

#include "ResourceManager.h"
#include "tjsDictionary.h"

#include <algorithm>
#include <cctype>

#include "RuntimeSupport.h"
#include "StorageIntf.h"

#include "log/TVPLog.h"

namespace {
    std::string lowercase(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char ch) {
                           return static_cast<char>(std::tolower(ch));
                       });
        return value;
    }

    ttstr trimMotionPath(ttstr path) {
        if(path.StartsWith(TJS_W("lzfs://./"))) {
            return path.SubString(9, path.GetLen() - 9);
        }
        return path;
    }

    std::vector<std::string> buildPathCacheKeys(const ttstr &path) {
        std::vector<std::string> keys;
        const auto pushUnique = [&keys](const std::string &key) {
            if(key.empty()) {
                return;
            }
            if(std::find(keys.begin(), keys.end(), key) == keys.end()) {
                keys.push_back(key);
            }
        };

        pushUnique(path.AsStdString());
        const ttstr trimmed = trimMotionPath(path);
        pushUnique(trimmed.AsStdString());
        const ttstr placed = TVPGetPlacedPath(trimmed);
        pushUnique(placed.AsStdString());
        return keys;
    }

    std::vector<std::string> collectModuleCacheKeys(
        const std::unordered_map<std::string, tTJSVariant> &loadedModules,
        iTJSDispatch2 *moduleObject) {
        std::vector<std::string> keys;
        if(!moduleObject) {
            return keys;
        }
        for(const auto &[key, module] : loadedModules) {
            if(module.Type() == tvtObject &&
               module.AsObjectNoAddRef() == moduleObject) {
                keys.push_back(key);
            }
        }
        return keys;
    }
} // namespace

motion::ResourceManager::ResourceManager() :
    _state(std::make_shared<State>()) {}

motion::ResourceManager::ResourceManager(iTJSDispatch2 *kag,
                                         tjs_int cacheSize) :
    _state(std::make_shared<State>()) {
    if(cacheSize > 0) {
        _state->cacheSize = cacheSize;
    }
    G_PluginLog.info("ResourceManager: kag={} cacheSize={}",
                     static_cast<void *>(kag), _state->cacheSize);

    // Pre-define ShortCutInitialPadKeyMap on the KAG window if not already set.
    // The encrypted keybinder.tjs accesses .ShortCutInitialPadKeyMap on the
    // window object. If undefined, it crashes with "Invalid object context".
    if(kag) {
        const tjs_char *padKeys[] = { TJS_W("ShortCutInitialPadKeyMap"),
                                      TJS_W("ShortCutInitialGamePadKeyMap"),
                                      TJS_W("_proceedingKeyList"), nullptr };
        for(int i = 0; padKeys[i]; ++i) {
            tTJSVariant existing;
            if(TJS_FAILED(
                   kag->PropGet(0, padKeys[i], nullptr, &existing, kag)) ||
               existing.Type() == tvtVoid) {
                iTJSDispatch2 *dict = TJSCreateDictionaryObject();
                if(dict) {
                    tTJSVariant v(dict, dict);
                    kag->PropSet(TJS_MEMBERENSURE, padKeys[i], nullptr, &v,
                                 kag);
                    dict->Release();
                }
            }
        }
    }
}

tjs_int motion::ResourceManager::getEmotePSBDecryptSeed() {
    return _decryptSeed;
}

tjs_error motion::ResourceManager::setEmotePSBDecryptSeed(tTJSVariant *,
                                                          tjs_int count,
                                                          tTJSVariant **p,
                                                          iTJSDispatch2 *) {
    if(count != 1) {
        return TJS_E_BADPARAMCOUNT;
    }
    if((*p)->Type() != tvtInteger) {
        return TJS_E_INVALIDPARAM;
    }
    _decryptSeed = static_cast<tjs_int>(*p[0]);
    G_PluginLog.info("setEmotePSBDecryptSeed: {}", _decryptSeed);
    return TJS_S_OK;
}

tjs_error motion::ResourceManager::setEmotePSBDecryptFunc(tTJSVariant *,
                                                          tjs_int count,
                                                          tTJSVariant **p,
                                                          iTJSDispatch2 *) {
    if(count != 1) {
        return TJS_E_BADPARAMCOUNT;
    }
    if(!p[0]) {
        return TJS_E_INVALIDPARAM;
    }
    _decryptFunc = p[0]->AsObjectClosure();
    G_PluginLog.info("setEmotePSBDecryptFunc: {}",
                     _decryptFunc.Object ? "closure stored" : "cleared");
    return TJS_S_OK;
}

tTJSVariant motion::ResourceManager::load(ttstr path) const {
    const auto rawPath = path.AsStdString();
    const auto loweredPath = lowercase(rawPath);
    if(loweredPath.find(".mtn") != std::string::npos) {
        G_PluginLog.debug("ResourceManager::load motion: {}", rawPath);
    } else if(loweredPath.find(".psb") != std::string::npos) {
        G_PluginLog.debug("ResourceManager::load emote/psb: {}", rawPath);
    }

    const auto loaded = detail::loadPSBVariant(path, _decryptSeed);
    if(loaded.Type() != tvtObject || !_state) {
        if(loaded.Type() == tvtVoid) {
            G_PluginLog.warn("ResourceManager::load({}) failed", rawPath);
        }
        return loaded;
    }

    for(const auto &key : buildPathCacheKeys(path)) {
        _state->loadedModules[key] = loaded;
    }
    _state->lastLoadedPath = rawPath;
    _state->lastLoadedModule = loaded;
    return loaded;
}

tTJSVariant motion::ResourceManager::loadSource(ttstr path) const {
    return load(path);
}

void motion::ResourceManager::unload(ttstr path) const {
    G_PluginLog.debug("ResourceManager::unload({})", path.AsStdString());
    if(!_state) {
        return;
    }

    tTJSVariant module;
    for(const auto &key : buildPathCacheKeys(path)) {
        const auto it = _state->loadedModules.find(key);
        if(it != _state->loadedModules.end()) {
            if(module.Type() == tvtVoid) {
                module = it->second;
            }
        }
    }

    if(module.Type() == tvtObject) {
        const auto keys = collectModuleCacheKeys(_state->loadedModules,
                                                 module.AsObjectNoAddRef());
        for(const auto &key : keys) {
            _state->loadedModules.erase(key);
        }
        detail::unregisterModuleSnapshot(module);
    } else {
        for(const auto &key : buildPathCacheKeys(path)) {
            _state->loadedModules.erase(key);
        }
    }

    if(_state->lastLoadedPath == path.AsStdString()) {
        _state->lastLoadedPath.clear();
        _state->lastLoadedModule.Clear();
    }
}

void motion::ResourceManager::clearCache() const {
    G_PluginLog.debug("ResourceManager::clearCache()");
    if(!_state) {
        return;
    }

    _state->loadedModules.clear();
    _state->lastLoadedPath.clear();
    _state->lastLoadedModule.Clear();
    _state->layerIdsByName.clear();
    _state->layerNamesById.clear();
    _state->usedLayerIds.clear();
    _state->nextLayerId = 1;
    detail::clearModuleSnapshots();
}

tTJSVariant motion::ResourceManager::getLastLoadedModule() const {
    return _state ? _state->lastLoadedModule : tTJSVariant{};
}

tTJSVariant motion::ResourceManager::findLoaded(ttstr path) const {
    return findLoadedModule(path);
}

tTJSVariant motion::ResourceManager::findLoadedModule(ttstr path) const {
    if(!_state || path.IsEmpty()) {
        return {};
    }

    const auto tryKey = [this](const std::string &key) -> tTJSVariant {
        if(key.empty()) {
            return {};
        }
        const auto it = _state->loadedModules.find(key);
        return it != _state->loadedModules.end() ? it->second : tTJSVariant{};
    };

    for(const auto &key : buildPathCacheKeys(path)) {
        if(auto loaded = tryKey(key); loaded.Type() == tvtObject) {
            return loaded;
        }
    }

    for(const auto &candidate : detail::buildMotionLookupCandidates(path)) {
        for(const auto &key : buildPathCacheKeys(candidate)) {
            if(auto loaded = tryKey(key); loaded.Type() == tvtObject) {
                return loaded;
            }
        }
    }

    G_PluginLog.debug("ResourceManager::findLoadedModule({}): cache miss",
                      path.AsStdString());
    return {};
}

tTJSVariant motion::ResourceManager::findSource(ttstr path) const {
    return findLoadedModule(path);
}

std::size_t motion::ResourceManager::uniqueCachedModuleCount() const {
    return uniqueCachedModules().size();
}

std::vector<motion::ResourceManager::CachedModuleEntry>
motion::ResourceManager::uniqueCachedModules() const {
    std::vector<CachedModuleEntry> result;
    if(!_state) {
        return result;
    }

    std::unordered_set<iTJSDispatch2 *> seen;
    for(const auto &[key, module] : _state->loadedModules) {
        if(module.Type() != tvtObject) {
            continue;
        }
        iTJSDispatch2 *obj = module.AsObjectNoAddRef();
        if(!obj || !seen.insert(obj).second) {
            continue;
        }
        result.push_back({ key, module });
    }
    return result;
}

tjs_int motion::ResourceManager::requireLayerId() {
    if(!_state) {
        return 0;
    }

    while(_state->usedLayerIds.find(_state->nextLayerId) !=
          _state->usedLayerIds.end()) {
        ++_state->nextLayerId;
    }
    const auto id = _state->nextLayerId;
    _state->usedLayerIds.insert(id);
    ++_state->nextLayerId;
    return id;
}

tjs_int motion::ResourceManager::requireLayerIdForName(ttstr name) {
    if(!_state) {
        return 0;
    }
    const auto key = name.AsStdString();
    if(const auto it = _state->layerIdsByName.find(key);
       it != _state->layerIdsByName.end()) {
        return it->second;
    }

    const auto id = requireLayerId();
    _state->layerIdsByName[key] = id;
    _state->layerNamesById[id] = key;
    return id;
}

void motion::ResourceManager::releaseLayerId(tjs_int id) {
    if(!_state || id == 0) {
        return;
    }
    _state->usedLayerIds.erase(id);
    if(const auto it = _state->layerNamesById.find(id);
       it != _state->layerNamesById.end()) {
        _state->layerIdsByName.erase(it->second);
        _state->layerNamesById.erase(it);
    }
}
