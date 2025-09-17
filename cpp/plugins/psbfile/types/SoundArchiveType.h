#pragma once

#include <string>

#include "IPSBType.h"

namespace PSB {

    class SoundArchiveType : public IPSBType {
    public:
        static inline const std::string &G_VoiceResourceKey = "voice";

        PSBType getPSBType() override { return PSBType::SoundArchive; }

        bool isThisType(const PSBFile &psb) override;

        std::vector<std::unique_ptr<IResourceMetadata>>
        collectResources(const PSBFile &psb, bool deDuplication) override;
    };
}; // namespace PSB