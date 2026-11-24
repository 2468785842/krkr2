#include "LocalHostServices.h"

#include <utility>

namespace krkr::host {
    LocalServices::LocalServices(const std::filesystem::path &resourceDirectory,
                                 std::string writableDirectory,
                                 std::function<void()> reportWriteFailure) :
        resources(std::make_shared<assets::LocalAssetReader>(
            std::vector<std::filesystem::path>{ resourceDirectory })),
        preferences(std::make_shared<config::LocalPreferenceStorage>(
            std::move(writableDirectory), std::move(reportWriteFailure))) {}
} // namespace krkr::host
