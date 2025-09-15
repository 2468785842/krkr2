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
    REQUIRE(f.loadPSBFile(TEST_FILES_PATH "/emote/title.psb"));
    const PSB::PSBHeader &header = f.getPSBHeader();
    REQUIRE(f.getType() == PSB::PSBType::Motion);
    CAPTURE(header.version, f.getType());
}

TEST_CASE("read psbfile ezsave.pimg") {
    PSB::PSBFile f;
    REQUIRE(f.loadPSBFile(TEST_FILES_PATH "/emote/ezsave.pimg"));
    const PSB::PSBHeader &header = f.getPSBHeader();
    REQUIRE(f.getType() == PSB::PSBType::Pimg);
    CAPTURE(header.version, f.getType());

    auto objs = f.getObjects();

    SECTION("check width height") {
        int width = static_cast<int>(
            *std::dynamic_pointer_cast<PSB::PSBNumber>((*objs)["width"]));
        int height = static_cast<int>(
            *std::dynamic_pointer_cast<PSB::PSBNumber>((*objs)["height"]));
        REQUIRE(width == 1280);
        REQUIRE(height == 720);
    }

    SECTION("get layers") {
        auto layers =
            std::dynamic_pointer_cast<PSB::PSBList>((*objs)["layers"]);
        REQUIRE(layers->size() == 32);

        SECTION("check layer properties") {
            std::vector group_layer_ids = { 3093, 3093, 3093, 2174, 2174, 2174,
                                            2174, 2158, 2158, 2158, 2158, 2158,
                                            2158, 2158, 2158, 2158, 2158, 2158,
                                            2158, 2158, 0,    2142, 2142, 2142,
                                            2135, 2135, 0,    0,    0,    0,
                                            0,    0 };

            std::vector heights = { 42,  42,  54,  43, 49,  49, 51, 51,
                                    51,  51,  51,  51, 51,  51, 51, 51,
                                    51,  51,  51,  51, 612, 42, 42, 54,
                                    612, 720, 720, 0,  0,   0,  0,  0 };

            std::vector widths = { 27, 27, 36, 34, 41,   41, 40, 36, 36, 36, 36,
                                   36, 36, 36, 36, 36,   36, 36, 36, 36, 40, 27,
                                   27, 36, 72, 80, 1280, 0,  0,  0,  0 };

            std::vector names = { "@pageup:over",
                                  "@pageup:off",
                                  "@pageup:rect",
                                  "@item:thumb:rect",
                                  "@item:over",
                                  "@item:off",
                                  "@item:rect",
                                  "@item0/cp:item",
                                  "@item1/cp:item",
                                  "@item2/cp:item",
                                  "@item3/cp:item",
                                  "@item4/cp:item",
                                  "@item5/cp:item",
                                  "@item6/cp:item",
                                  "@item7/cp:item",
                                  "@item8/cp:item",
                                  "@item9/cp:item",
                                  "@item10/cp:item",
                                  "@item11/cp:item",
                                  "@item12/cp:item",
                                  "@scroll/lay:rect",
                                  "@pagedown:over",
                                  "@pagedown:off",
                                  "@pagedown:rect",
                                  "@base:open:rect",
                                  "@base:rect",
                                  "レイヤー 1",
                                  "pageup",
                                  "item",
                                  "items",
                                  "pagedown",
                                  "範囲情報" };

            std::vector layer_ids = { 3092, 308,  -1,   -1,   2168, 216,  -1,
                                      2157, 2156, 2155, 2154, 2153, 2152, 2151,
                                      2150, 2149, 2148, 2147, 2146, 214,  -1,
                                      2139, 213,  -1,   -1,   -1,   2036, 3093,
                                      2174, 2158, 2142, 2135 };

            std::vector layer_types = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 2, 2, 2, 2, 2 };

            std::vector lefts = {
                1249, 1249, 1244, 1246, 1239, 1239, 1240, 1244,
                1244, 1244, 1244, 1244, 1244, 1244, 1244, 1244,
                1244, 1244, 1244, 1244, 1240, 1248, 1248, 1244,
                1208, 1200, 0,    0,    0,    0,    0,    0,
            };

            std::vector tops = {
                7,   7,   0,   58,  55,  55,  54,  54,  105, 156, 207,
                258, 309, 360, 411, 462, 513, 564, 615, 666, 54,  671,
                671, 666, 54,  0,   0,   0,   0,   0,   0,   0,
            };

            std::vector visibles = {
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
            };

            std::vector same_images = { 0,    0,    0,    0,    0,    0,
                                        0,    0,    2157, 2157, 2157, 2157,
                                        2157, 2157, 2157, 2157, 2157, 2157,
                                        2157, 2157, 0,    0,    0,    0,
                                        0,    0,    0,    0,    0,    0,
                                        0,    0 };

            for(int i = 0; i < layers->size(); i++) {
                auto layer =
                    std::dynamic_pointer_cast<PSB::PSBDictionary>((*layers)[i]);

                // height width opacity name layer_id layer_type left top type
                // visible  same_image
                SECTION("group_layer_ids") {
                    auto group_layer_id =
                        std::dynamic_pointer_cast<PSB::PSBNumber>(
                            (*layer)["group_layer_id"]);
                    if(group_layer_id == nullptr && group_layer_ids[i] == 0)
                        continue;
                    REQUIRE(static_cast<int>(*group_layer_id) ==
                            group_layer_ids[i]);
                }
                SECTION("heights") {
                    auto height = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["height"]);
                    REQUIRE(static_cast<int>(*height) == heights[i]);
                }
                SECTION("widths") {
                    auto width = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["width"]);
                    REQUIRE(static_cast<int>(*width) == widths[i]);
                }
                SECTION("opacities") {
                    auto opacity = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["opacity"]);
                    REQUIRE(static_cast<int>(*opacity) == 255);
                }
                SECTION("names") {
                    auto name = std::dynamic_pointer_cast<PSB::PSBString>(
                        (*layer)["name"]);
                    REQUIRE(name->value == names[i]);
                }
                SECTION("layer_ids") {
                    auto layer_id = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["layer_id"]);
                    REQUIRE(static_cast<int>(*layer_id) == layer_ids[i]);
                }
                SECTION("layer_types") {
                    auto layer_type = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["layer_type"]);
                    REQUIRE(static_cast<int>(*layer_type) == layer_types[i]);
                }
                SECTION("left") {
                    auto left = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["left"]);
                    REQUIRE(static_cast<int>(*left) == lefts[i]);
                }
                SECTION("top") {
                    auto top = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["top"]);
                    REQUIRE(static_cast<int>(*top) == tops[i]);
                }
                SECTION("type") {
                    auto type = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["type"]);
                    REQUIRE(static_cast<int>(*type) == 13);
                }
                SECTION("visible") {
                    auto visible = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["visible"]);
                    REQUIRE(static_cast<int>(*visible) == visibles[i]);
                }
                SECTION("same_image") {
                    auto same_image = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["same_image"]);
                    if(same_image == nullptr && same_images[i] == 0)
                        continue;
                    REQUIRE(static_cast<int>(*same_image) == same_images[i]);
                }
            }
        }
    }

    SECTION("collect resources") {
        auto resMetadata = f.getTypeHandler()->collectResources(f, true);
        REQUIRE(!resMetadata.empty());
        for(auto &res : resMetadata) {
            auto imgMetadata = dynamic_cast<PSB::ImageMetadata *>(res.get());
            REQUIRE(imgMetadata != nullptr);
        }
    }
}

TEST_CASE("read psbfile e-mote3.0 psb") {
    int key = 742877301;
    PSB::PSBFile f;
    f.setSeed(key);
    REQUIRE(
        f.loadPSBFile(TEST_FILES_PATH "/emote/e-mote3.0バニラパジャマa.psb"));
    REQUIRE(f.getType() == PSB::PSBType::Motion);
}
