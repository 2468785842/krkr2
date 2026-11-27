#include "tjsCommHead.h"
#include "SdlHost.h"
#include <filesystem>
#include <stdexcept>
#import <Foundation/Foundation.h>

namespace krkr::sdl {
    std::unique_ptr<krkr::host::LocalServices> CreateHostServices() {
        @autoreleasepool {
            NSBundle *bundle = [NSBundle mainBundle];
            NSString *executable = [bundle executablePath];
            if(!executable)
                throw std::runtime_error("Cannot resolve executable path");
            const auto path = std::filesystem::u8path([executable UTF8String]);
            const bool bundled = [[bundle bundlePath] hasSuffix:@".app"];
            const auto resources = bundled
                ? std::filesystem::u8path([[bundle resourcePath] UTF8String])
                : path.parent_path() / "Resources";
            NSURL *support = [[[NSFileManager defaultManager]
                URLsForDirectory:NSApplicationSupportDirectory
                       inDomains:NSUserDomainMask] firstObject];
            if(!support)
                throw std::runtime_error("Cannot resolve Application Support");
            const auto writable =
                std::filesystem::u8path([[support path] UTF8String]) / "krkr2";
            return std::make_unique<krkr::host::LocalServices>(
                resources, writable.u8string(), ReportPreferenceWriteFailure);
        }
    }
} // namespace krkr::sdl
