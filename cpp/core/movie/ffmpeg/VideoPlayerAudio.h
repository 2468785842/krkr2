#pragma once

#include "Thread.h"
#include "IVideoPlayer.h"
#include "AudioCodec.h"
#include "BitstreamStats.h"
#include "MessageQueue.h"
#include "Timer.h"
#include "Clock.h"
#include "AudioDevice.h"

NS_KRMOVIE_BEGIN
class CDVDClock;

class CVideoPlayerAudio : public CThread, public IDVDStreamPlayerAudio {
public:
    CVideoPlayerAudio(CDVDClock *pClock, CDVDMessageQueue &parent,
                      CProcessInfo &processInfo);

    ~CVideoPlayerAudio() override;

    bool OpenStream(CDVDStreamInfo &hints) override;

    void CloseStream(bool bWaitForBuffers) override;

    void SetSpeed(int speed) override;

    void Flush(bool sync) override;

    // waits until all available data has been rendered
    bool AcceptsData() override;

    bool HasData() const override { return m_messageQueue.GetDataSize() > 0; }

    int GetLevel() override { return m_messageQueue.GetLevel(); }

    bool IsInited() const override { return m_messageQueue.IsInited(); }

    void SendMessage(CDVDMsg *pMsg, int priority = 0) override {
        m_messageQueue.Put(pMsg, priority);
    }

    void FlushMessages() override { m_messageQueue.Flush(); }

    //	void SetDynamicRangeCompression(long drc)             {
    // m_dvdAudio.SetDynamicRangeCompression(drc); }
    float GetDynamicRangeAmplification() const override { return 0.0f; }

    std::string GetPlayerInfo() override;

    int GetAudioBitrate() override;

    int GetAudioChannels() override;

    // holds stream information for current playing stream
    CDVDStreamInfo m_streaminfo;

    double GetCurrentPts() override {
        CSingleLock lock(m_info_section);
        return m_info.pts;
    }

    bool IsStalled() const override { return m_stalled; }

    bool IsPassthrough() override;

    CDVDAudio *GetOutputDevice() override { return &m_dvdAudio; }

protected:
    void OnStartup() override;

    void OnExit() override;

    void Process() override;

    void UpdatePlayerInfo();

    void OpenStream(CDVDStreamInfo &hints, CDVDAudioCodec *codec);

    //! Switch codec if needed. Called when the sample rate gotten
    //! from the codec changes, in which case we may want to switch
    //! passthrough on/off.
    bool SwitchCodecIfNeeded();
    //	float GetCurrentAttenuation()                         { return
    // m_dvdAudio.GetCurrentAttenuation(); }

    CDVDMessageQueue m_messageQueue;
    CDVDMessageQueue &m_messageParent;

    double m_audioClock;

    CDVDAudio m_dvdAudio; // audio output device
    CDVDClock *m_pClock; // dvd master clock
    CDVDAudioCodec *m_pAudioCodec; // audio codec
    BitstreamStats m_audioStats;

    int m_speed;
    bool m_stalled;
    bool m_silence;
    bool m_paused;
    IDVDStreamPlayer::ESyncState m_syncState;
    Timer m_syncTimer;

    bool OutputPacket(DVDAudioFrame &audioframe);

    // SYNC_DISCON, SYNC_SKIPDUP, SYNC_RESAMPLE
    int m_synctype;
    int m_setsynctype;
    int m_prevsynctype; // so we can print to the log

    void SetSyncType(bool passthrough);

    bool m_prevskipped;
    double m_maxspeedadjust;

    struct SInfo {
        SInfo() : pts(DVD_NOPTS_VALUE), passthrough(false) {}

        std::string info;
        double pts;
        bool passthrough;
    };

    CCriticalSection m_info_section;
    SInfo m_info;
};

NS_KRMOVIE_END