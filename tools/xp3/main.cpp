#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <argparse/argparse.hpp>

#include "log/TVPInitLog.h"
#include "log/TVPDialog.h"
#include "XP3Archive.h"
#include "IFileBackend.h"
#include "StdFileBackend.h"

namespace fs = std::filesystem;

static constexpr size_t TVP_LOCAL_TEMP_COPY_BLOCK_SIZE = 65536 * 2;

void extractArchive(const std::string &file, const std::string &destDir) {
    const ttstr path(file);

    IFileBackend *backend = TVPGetFileBackend();
    if(!backend) {
        std::cerr << "Storage not initialized" << std::endl;
        return;
    }

    auto stream = backend->OpenRead(path);
    if(!stream) {
        std::cerr << "Cannot open: " << file << std::endl;
        return;
    }

    tTVPArchive *raw = tTVPXP3Archive::Create(path, stream.get(), false);
    if(!raw) {
        std::cerr << "Not an XP3 archive: " << file << std::endl;
        return;
    }
    stream.release();

    const std::unique_ptr<tTVPArchive> arc(raw);
    const tjs_uint count = arc->GetCount();
    for(tjs_int i = 0; i < count; i++) {
        ttstr name = arc->GetName(i);
#ifndef _WIN32
        name.Replace(TJS_W('\\'), TJS_W('/'), true);
#endif
        const std::unique_ptr<tTJSBinaryStream> src{ arc->CreateStreamByIndex(
            i) };
        const ttstr &destFile = ttstr{ destDir } + name;

#ifdef _WIN32
        fs::path destFilePath = fs::u8path(destFile.AsNarrowStdString());
#else
        fs::path destFilePath = fs::path(destFile.AsNarrowStdString());
#endif

        fs::create_directories(destFilePath.parent_path());
        std::ofstream ofs(destFilePath, std::ios::binary);
        auto buffer =
            std::make_unique<tjs_uint8[]>(TVP_LOCAL_TEMP_COPY_BLOCK_SIZE);

        while(true) {
            const tjs_uint read =
                src->Read(buffer.get(), TVP_LOCAL_TEMP_COPY_BLOCK_SIZE);
            if(read == 0)
                break;
            ofs.write(
                reinterpret_cast<const std::ostream::char_type *>(buffer.get()),
                read);
        }
        ofs.close();
    }
}


std::string normalizePath(const std::string &path) {
    if(path.empty())
        return path;

    std::string expanded = path;

    // 1. 展开 ~ -> 用户主目录
    if(expanded[0] == '~') {
#if defined(_WIN32)
        const char *home = std::getenv("USERPROFILE");
#else
        const char *home = std::getenv("HOME");
#endif
        if(home) {
            expanded = std::string(home) + expanded.substr(1);
        }
    }

#if defined(_WIN32)
    // 处理类似 %APPDATA% 的路径
    size_t start = expanded.find('%');
    while(start != std::string::npos) {
        size_t end = expanded.find('%', start + 1);
        if(end == std::string::npos)
            break;
        std::string var = expanded.substr(start + 1, end - start - 1);
        const char *val = std::getenv(var.c_str());
        if(val) {
            expanded.replace(start, end - start + 1, val);
        }
        start = expanded.find('%', start + 1);
    }
#endif

    // 3. 转为绝对路径 & 规范化（处理 . .. 多余的分隔符）
    try {
        expanded = fs::weakly_canonical(fs::path(expanded)).string();
    } catch(...) {
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

    program.add_argument("-o", "--output").help("output dir path");
    program.add_argument("-l", "--loglevel")
        .help("log level spec, e.g. debug or tjs2:debug,core:info");

    try {
        program.parse_args(argc, argv);
    } catch(const std::exception &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    TVPLoggingOptions log_options;
    log_options.level = TVPLogInitLevel::Debug;
    log_options.pattern = "%^%v%$";
    std::string loglevel_spec;
    if(program.is_used("--loglevel")) {
        loglevel_spec = program.get<std::string>("--loglevel");
        log_options.log_level = loglevel_spec.c_str();
    }
    TVPInitLogging(log_options);
    TVPSetDialogBackend(nullptr);

    TVPSetFileBackend(std::make_unique<StdFileBackend>());

    std::string output_dir = "./";
    if(program.is_used("-o")) {
        output_dir = program.get<std::string>("-o");
    }

    const auto input_files = program.get<std::vector<std::string>>("files");
    for(const auto &input : input_files) {
        fs::path file(normalizePath(input));

        if(!fs::exists(file) || fs::is_directory(file)) {
            std::cerr << "Skipping invalid file: " << input << std::endl;
            continue;
        }

        extractArchive(file.string(),
                       fs::path(normalizePath(output_dir) /
                                fs::path(file.stem().string()) / "")
                           .string());
    }

    return 0;
}
