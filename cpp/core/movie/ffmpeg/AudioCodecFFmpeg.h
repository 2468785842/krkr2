#pragma once

#include "KRMovieDef.h"
#include "AudioCodec.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
}

NS_KRMOVIE_BEGIN
class CProcessInfo;

class CDVDAudioCodecFFmpeg : public CDVDAudioCodec {
public:
    CDVDAudioCodecFFmpeg(CProcessInfo &processInfo);

    ~CDVDAudioCodecFFmpeg() override;

    bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;

    void Dispose() override;

    int Decode(uint8_t *pData, int iSize, double dts, double pts) override;

    void GetData(DVDAudioFrame &frame) override;

    int GetData(uint8_t **dst) override;

    void Reset() override;

    AEAudioFormat GetFormat() override { return m_format; }

    const char *GetName() override { return "FFmpeg"; }

    enum AVMatrixEncoding GetMatrixEncoding() override;

    enum AVAudioServiceType GetAudioServiceType() override;

    int GetProfile() override;

protected:
    enum AEDataFormat GetDataFormat();

    int GetSampleRate();

    int GetChannels();

    CAEChannelInfo GetChannelMap();

    int GetBitRate() override;

    AEAudioFormat m_format;
    AVCodecContext *m_pCodecContext;
    enum AVSampleFormat m_iSampleFormat;
    CAEChannelInfo m_channelLayout;
    enum AVMatrixEncoding m_matrixEncoding;

    AVFrame *m_pFrame1;
    int m_gotFrame;

    int m_channels;
    uint64_t m_layout;

    void BuildChannelMap();

    void ConvertToFloat();
};

NS_KRMOVIE_END