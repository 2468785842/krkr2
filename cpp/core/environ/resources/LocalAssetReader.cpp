#include "LocalAssetReader.h"

#include <atomic>
#include <cstdint>
#include <fstream>
#include <limits>
#include <utility>

namespace krkr::assets {
    namespace {
        std::shared_ptr<const AssetReader> activeReader;

        bool IsFile(const std::filesystem::path &path) {
            std::error_code error;
            return std::filesystem::is_regular_file(path, error);
        }
    } // namespace

    LocalAssetReader::LocalAssetReader(
        std::vector<std::filesystem::path> paths) {
        for(const auto &path : paths)
            roots.push_back(std::filesystem::absolute(path).lexically_normal());
    }

    std::filesystem::path
    LocalAssetReader::Resolve(const std::string &name) const {
        if(name.empty())
            return {};
        const auto path = std::filesystem::u8path(name);
        if(path.is_absolute())
            return IsFile(path) ? path : std::filesystem::path{};
        for(const auto &root : roots) {
            auto candidate = root / path;
            if(IsFile(candidate))
                return candidate;
        }
        return {};
    }

    bool LocalAssetReader::Exists(const std::string &name) const {
        return !Resolve(name).empty();
    }

    bool LocalAssetReader::Read(const std::string &name,
                                const Reader &reader) const {
        const auto path = Resolve(name);
        if(path.empty())
            return false;
        std::ifstream input(path, std::ios::binary | std::ios::ate);
        if(!input)
            return false;
        const auto length = input.tellg();
        if(length < 0 ||
           static_cast<std::uintmax_t>(length) >
               std::numeric_limits<std::size_t>::max() ||
           static_cast<std::uintmax_t>(length) >
               std::numeric_limits<std::streamsize>::max())
            return false;
        std::vector<unsigned char> bytes(static_cast<std::size_t>(length));
        input.seekg(0);
        if(!input ||
           (!bytes.empty() &&
            !input.read(reinterpret_cast<char *>(bytes.data()),
                        static_cast<std::streamsize>(bytes.size()))))
            return false;
        input.close();
        if(input.fail())
            return false;
        reader(bytes.data(), bytes.size());
        return true;
    }

    ScopedReader::ScopedReader(std::shared_ptr<const AssetReader> reader) :
        previous(std::atomic_exchange(&activeReader, std::move(reader))) {}

    ScopedReader::~ScopedReader() {
        std::atomic_store(&activeReader, std::move(previous));
    }

    std::shared_ptr<const AssetReader> CurrentReader() {
        return std::atomic_load(&activeReader);
    }

    void SetReader(std::shared_ptr<const AssetReader> reader) {
        std::atomic_store(&activeReader, std::move(reader));
    }
} // namespace krkr::assets
