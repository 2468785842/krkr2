//
// Created by LiDong on 2025/9/18.
//
#include <catch2/catch_test_macros.hpp>

#include "krffmpeg.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
}

TEST_CASE("init ffmpeg") {
    TVPInitLibAVCodec();
    auto *codec = av_codec_next(nullptr);
    auto *demuxer = av_iformat_next(nullptr);
    auto *filter = avfilter_next(nullptr);
    REQUIRE(codec != nullptr);
    REQUIRE(demuxer != nullptr);
    REQUIRE(filter != nullptr);

    TVPInitLibAVCodec();
    REQUIRE(av_codec_next(nullptr) == codec);
    REQUIRE(av_iformat_next(nullptr) == demuxer);
    REQUIRE(avfilter_next(nullptr) == filter);
}
