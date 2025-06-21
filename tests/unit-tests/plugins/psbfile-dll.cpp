//
// Created by lidong on 25-6-21.
//

#include <sstream>
#include <psbfile/PSBFile.h>

#include <catch2/catch_test_macros.hpp>

#include "test_config.h"

PSB::PSBFile load(std::string_view s) {
    std::stringstream path{"file://."};
    path << TEST_FILES_PATH << "/" << s;
    PSB::PSBFile f;
    REQUIRE(f.loadPSBFile(path.str()));
    return f;
}

TEST_CASE("read psbfile title.psb") {
    const PSB::PSBFile &f = load("title.psb");
    const PSB::PSBHeader &header = f.getPSBHeader();
    REQUIRE(f.getType() == PSB::PSBType::PSB);
    CAPTURE(header.version, f.getType());
}

TEST_CASE("read psbfile ev107a.pimg") {
    const PSB::PSBFile &f = load("ev107a.pimg");
    const PSB::PSBHeader &header = f.getPSBHeader();
    CAPTURE(header.version, f.getType());
    const std::shared_ptr<const PSB::PSBDictionary> &objs = f.getObjects();
    REQUIRE(objs->find("layers") != objs->end());
}
