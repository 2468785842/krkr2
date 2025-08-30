#pragma once

#include <stdint.h>
#include <vector>
#include <string>

#include "AEChannelData.h"

NS_KRMOVIE_BEGIN
class CHelper_libKODI_audioengine;

class CAEChannelInfo {
    friend class CHelper_libKODI_audioengine;

public:
    CAEChannelInfo();

    CAEChannelInfo(const enum AEChannel *rhs);

    CAEChannelInfo(enum AEStdChLayout rhs);

    ~CAEChannelInfo();

    CAEChannelInfo &operator=(const CAEChannelInfo &rhs);

    CAEChannelInfo &operator=(const enum AEChannel *rhs);

    CAEChannelInfo &operator=(enum AEStdChLayout rhs);

    bool operator==(const CAEChannelInfo &rhs) const;

    bool operator!=(const CAEChannelInfo &rhs) const;

    CAEChannelInfo &operator+=(const enum AEChannel &rhs);

    CAEChannelInfo &operator-=(const enum AEChannel &rhs);

    enum AEChannel operator[](unsigned int i) const;

    operator std::string() const;

    /* remove any channels that dont exist in the provided info */
    void ResolveChannels(const CAEChannelInfo &rhs);

    void Reset();

    [[nodiscard]] inline unsigned int Count() const { return m_channelCount; }

    static const char *GetChName(enum AEChannel ch);

    [[nodiscard]] bool HasChannel(enum AEChannel ch) const;

    [[nodiscard]] bool ContainsChannels(const CAEChannelInfo &rhs) const;

    void ReplaceChannel(enum AEChannel from, enum AEChannel to);

    int BestMatch(const std::vector<CAEChannelInfo> &dsts,
                  int *score = nullptr) const;

    void AddMissingChannels(const CAEChannelInfo &rhs);

private:
    unsigned int m_channelCount;
    enum AEChannel m_channels[AE_CH_MAX];
};

NS_KRMOVIE_END