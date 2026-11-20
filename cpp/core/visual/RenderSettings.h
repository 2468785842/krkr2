#pragma once

#include <string>
#include <string_view>

namespace krkr::render {
    enum class TextureStorage {
        Normal,
        HalfRows,
        Lz4,
        Lz4Tlg5,
        Half,
        Etc2,
        Pvrtc
    };

    inline const char *StorageName(TextureStorage storage) {
        switch(storage) {
            case TextureStorage::HalfRows:
                return "halfline";
            case TextureStorage::Lz4:
                return "lz4";
            case TextureStorage::Lz4Tlg5:
                return "lz4+tlg5";
            case TextureStorage::Half:
                return "half";
            case TextureStorage::Etc2:
                return "etc2";
            case TextureStorage::Pvrtc:
                return "pvrtc";
            default:
                return "none";
        }
    }

    class PreferenceSource {
    public:
        virtual ~PreferenceSource() = default;
        virtual std::string String(const char *key, const char *fallback) = 0;
        virtual int Integer(const char *key, int fallback) = 0;
    };

    // Each getter requests only its original key. The legacy configuration
    // source writes missing defaults, so eager snapshots change observable
    // state.
    inline std::string RendererName(PreferenceSource &source) {
        return source.String("renderer", "software");
    }
    inline TextureStorage SoftwareStorage(PreferenceSource &source) {
        auto value = source.String("software_compress_tex", "none");
        if(value == "halfline")
            return TextureStorage::HalfRows;
        if(value == "lz4")
            return TextureStorage::Lz4;
        if(value == "lz4+tlg5")
            return TextureStorage::Lz4Tlg5;
        return TextureStorage::Normal;
    }
    inline TextureStorage GLStorage(PreferenceSource &source) {
        auto value = source.String("ogl_compress_tex", "none");
        if(value == "half")
            return TextureStorage::Half;
        if(value == "etc2")
            return TextureStorage::Etc2;
        if(value == "pvrtc")
            return TextureStorage::Pvrtc;
        return TextureStorage::Normal;
    }
    inline unsigned TextureLimit(unsigned detected, int requested) {
        if(requested > 0 &&
           (static_cast<unsigned>(requested) < detected || detected < 1024))
            return static_cast<unsigned>(requested);
        return detected;
    }
} // namespace krkr::render
