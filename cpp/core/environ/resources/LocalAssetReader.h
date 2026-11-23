#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace krkr::assets {
    using Reader = std::function<void(const unsigned char *, std::size_t)>;

    class AssetReader {
    public:
        virtual ~AssetReader() = default;
        virtual bool Exists(const std::string &name) const = 0;
        virtual bool Read(const std::string &name,
                          const Reader &reader) const = 0;
    };

    // Roots are captured at construction. Absolute file names bypass them.
    class LocalAssetReader final : public AssetReader {
        std::vector<std::filesystem::path> roots;
        std::filesystem::path Resolve(const std::string &name) const;

    public:
        explicit LocalAssetReader(std::vector<std::filesystem::path> roots);
        bool Exists(const std::string &name) const override;
        bool Read(const std::string &name, const Reader &reader) const override;
    };

    // Install/restore on the host thread. Readers may be used from worker
    // threads.
    class ScopedReader {
        std::shared_ptr<const AssetReader> previous;

    public:
        explicit ScopedReader(std::shared_ptr<const AssetReader> reader);
        ~ScopedReader();
        ScopedReader(const ScopedReader &) = delete;
        ScopedReader &operator=(const ScopedReader &) = delete;
    };

    // Hosts install their provider before initializing engine resources.
    void SetReader(std::shared_ptr<const AssetReader> reader);
    std::shared_ptr<const AssetReader> CurrentReader();
} // namespace krkr::assets
