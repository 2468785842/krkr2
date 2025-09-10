//
// Created by lidong on 25-6-21.
//

#include <catch2/catch_test_macros.hpp>

#include "LayerBitmapIntf.h"
#include "StorageIntf.h"
#include "psbfile/PSBFile.h"
#include "test_config.h"

TEST_CASE("read psbfile title.psb") {
    PSB::PSBFile f;
    REQUIRE(f.loadPSBFile(TEST_FILES_PATH "/title.psb"));
    const PSB::PSBHeader &header = f.getPSBHeader();
    REQUIRE(f.getType() == PSB::PSBType::PSB);
    CAPTURE(header.version, f.getType());
}

TEST_CASE("read psbfile ev107a.pimg") {
    PSB::PSBFile f;
    REQUIRE(f.loadPSBFile(TEST_FILES_PATH "/ev107a.pimg"));
    const PSB::PSBHeader &header = f.getPSBHeader();
    REQUIRE(f.getType() == PSB::PSBType::Pimg);
    CAPTURE(header.version, f.getType());
    auto resMetadata = f.getTypeHandler()->collectResources(f, true);
    REQUIRE(!resMetadata.empty());
}