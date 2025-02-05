#include <iostream>
#include <filesystem>
#include "psdparse.h"
#include "psdfile.h"
#include "ncbind.hpp"

#define NCB_MODULE_NAME TJS_W("psbfile.dll")

using namespace psd;

// psdファイルをロードする
bool PSDFile::load(const char *filename) {
    clearData();

    namespace fs = std::filesystem;

    isLoaded = false;

    const std::filesystem::path file(filename);
    std::error_code error;
    const bool result = fs::exists(file, error);
    if(!result || error) {
        std::cerr << "file not found!: '" << filename << "'" << std::endl;
        return false;
    }

    in.open(filename);
    if(!in.is_open()) {
        std::cerr << "could not open input file: '" << filename << "'"
                  << std::endl;
        return false;
    }

    typedef boost::iostreams::mapped_file_source::iterator iterator_type;
    iterator_type iter = in.begin();
    iterator_type end = in.end();

    psd::Parser<iterator_type> parser(*this);
    bool r = parse(iter, end, parser);
    if(r && iter == end) {
        dprint("succeeded\n");
        isLoaded = processParsed();
    } else {
        std::cerr << "failed\n";
        in.close();
    }

    return isLoaded;
}