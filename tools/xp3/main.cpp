#include <filesystem>
#include <fstream>
#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "XP3Archive.h"

namespace fs = std::filesystem;

static constexpr size_t TVP_LOCAL_TEMP_COPY_BLOCK_SIZE = 65536 * 2;

void extractArchive(const std::string &file, const std::string& destDir) {
    const std::unique_ptr<tTVPArchive> arc{TVPOpenArchive(ttstr{file}, false)};
    const tjs_uint count = arc->GetCount();
    for(tjs_int i = 0; i < count; i++) {
        ttstr name = arc->GetName(i);
#ifndef _WIN32
        name.Replace(TJS_W('\\'), TJS_W('/'), true);
#endif
        const std::unique_ptr<tTJSBinaryStream> src{arc->CreateStreamByIndex(i)};
        const ttstr &destFile = ttstr{destDir} + name;
        fs::create_directories(fs::path(destFile.AsNarrowStdString()).parent_path());
        std::ofstream ofs(destFile.AsNarrowStdString(), std::ios::binary);
        auto buffer = std::make_unique<tjs_uint8[]>(TVP_LOCAL_TEMP_COPY_BLOCK_SIZE);

        while(true) {
            const tjs_uint read = src->Read(buffer.get(), TVP_LOCAL_TEMP_COPY_BLOCK_SIZE);
            if(read == 0) break;
            ofs.write(reinterpret_cast<const std::ostream::char_type *>(buffer.get()), read);
        }
        ofs.close();
    }
}


std::string normalizePath(const std::string& path) {
    if (path.empty()) return path;

    std::string expanded = path;

    // 1. 展开 ~ -> 用户主目录
    if (expanded[0] == '~') {
#if defined(_WIN32)
        const char* home = std::getenv("USERPROFILE");
#else
        const char* home = std::getenv("HOME");
#endif
        if (home) {
            expanded = std::string(home) + expanded.substr(1);
        }
    }

    // 2. 展开环境变量（Windows 常见情况）
#if defined(_WIN32)
    // 处理类似 %APPDATA% 的路径
    size_t start = expanded.find('%');
    while (start != std::string::npos) {
        size_t end = expanded.find('%', start + 1);
        if (end == std::string::npos) break;
        std::string var = expanded.substr(start + 1, end - start - 1);
        const char* val = std::getenv(var.c_str());
        if (val) {
            expanded.replace(start, end - start + 1, val);
        }
        start = expanded.find('%', start + 1);
    }
#endif

    // 3. 转为绝对路径 & 规范化（处理 . .. 多余的分隔符）
    try {
        expanded = fs::weakly_canonical(fs::path(expanded)).string();
    } catch (...) {
        // 如果路径不存在 weakly_canonical 可能抛异常，fallback 用 absolute
        expanded = fs::absolute(fs::path(expanded)).string();
    }

    return expanded;
}

int main(int argc, char *argv[]) {
    argparse::ArgumentParser program(PROGRAM_NAME, VERSION);

    program.add_argument("files")
        .help("input files path")
        .nargs(argparse::nargs_pattern::at_least_one);

    program.add_argument("-o", "--output")
        .help("output dir path");

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    spdlog::set_level(spdlog::level::debug);

    static auto core_logger = spdlog::stdout_color_mt("core");
    static auto tjs2_logger = spdlog::stdout_color_mt("tjs2");
    static auto plugin_logger = spdlog::stdout_color_mt("plugin");
    spdlog::set_pattern("%^%v%$");
    spdlog::set_default_logger(core_logger);

    std::string output_dir = "./";
    if(program.is_used("-o")) {
        output_dir = program.get<std::string>("--output");
    }

    const auto input_files = program.get<std::vector<std::string>>("files");
    for(const auto& input : input_files) {
        fs::path file(normalizePath(input));

        if(!fs::exists(file) || fs::is_directory(file)) {
            std::cerr << "Skipping invalid file: " << input << std::endl;
            continue;
        }
        output_dir = normalizePath(output_dir) / fs::path(file.stem().string()) / "";
        extractArchive(file.string(), output_dir);
    }

    return 0;
}