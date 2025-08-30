#pragma once
#include "AudioCodec.h"
#include "AEAudioFormat.h"
#include "AEStreamInfo.h"
#include <memory>
NS_KRMOVIE_BEGIN
class CProcessInfo;

class CDVDAudioCodecPassthrough : public CDVDAudioCodec {
public:
    CDVDAudioCodecPassthrough(CProcessInfo &processInfo);
    ~CDVDAudioCodecPassthrough() override;

    bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
    void Dispose() override;
    int Decode(uint8_t *pData, int iSize, double dts, double pts) override;
    void GetData(DVDAudioFrame &frame) override;
    int GetData(uint8_t **dst) override;
    void Reset() override;
    AEAudioFormat GetFormat() override { return m_format; }
    bool NeedPassthrough() override { return true; }
    const char *GetName() override { return "passthrough"; }
    int GetBufferSize() override;

private:
    CAEStreamParser m_parser;
    uint8_t *m_buffer;
    unsigned int m_bufferSize;
    unsigned int m_dataSize{};
    AEAudioFormat m_format;
    uint8_t m_backlogBuffer[61440]{};
    unsigned int m_backlogSize{};
    double m_currentPts{};
    double m_nextPts{};

    // TrueHD specifics
    std::unique_ptr<uint8_t[]> m_trueHDBuffer;
    unsigned int m_trueHDoffset;
};

NS_KRMOVIE_END