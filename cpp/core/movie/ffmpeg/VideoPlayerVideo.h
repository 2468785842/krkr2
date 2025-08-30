#pragma once

#include "Thread.h"
#include "IVideoPlayer.h"
#include "VideoCodec.h"
#include "BitstreamStats.h"
#include <deque>
#include <list>
#include "MessageQueue.h"

NS_KRMOVIE_BEGIN

class CDemuxStreamVideo;

class CRenderManager;

#define VIDEO_PICTURE_QUEUE_SIZE 1

#define EOS_ABORT 1
#define EOS_DROPPED 2
#define EOS_VERYLATE 4
#define EOS_BUFFER_LEVEL 8

class CDroppingStats {
public:
    void Reset();

    void AddOutputDropGain(double pts, int frames);

    struct CGain {
        int frames;
        double pts;
    };
    std::deque<CGain> m_gain;
    double m_totalGain;
    double m_lastPts;
};

class CDVDClock;

class CVideoPlayerVideo : public CThread, public IDVDStreamPlayerVideo {
public:
    CVideoPlayerVideo(CDVDClock *pClock
                      //	, CDVDOverlayContainer* pOverlayContainer
                      ,
                      CDVDMessageQueue &parent, CRenderManager &renderManager,
                      CProcessInfo &processInfo);

    ~CVideoPlayerVideo() override;

    bool OpenStream(CDVDStreamInfo &hint) override;

    void CloseStream(bool bWaitForBuffers) override;

    void Flush(bool sync) override;

    bool AcceptsData() override;

    bool HasData() const override { return m_messageQueue.GetDataSize() > 0; }

    int GetLevel() override { return m_messageQueue.GetLevel(); }

    bool IsInited() const override { return m_messageQueue.IsInited(); }

    void SendMessage(CDVDMsg *pMsg, int priority = 0) override {
        m_messageQueue.Put(pMsg, priority);
    }

    void FlushMessages() override { m_messageQueue.Flush(); }

    void EnableSubtitle(bool bEnable) override { m_bRenderSubs = bEnable; }

    bool IsSubtitleEnabled() override { return m_bRenderSubs; }

    //	void EnableFullscreen(bool bEnable) { m_bAllowFullscreen =
    // bEnable; }
    double GetSubtitleDelay() override { return m_iSubtitleDelay; }

    void SetSubtitleDelay(double delay) override { m_iSubtitleDelay = delay; }

    bool IsStalled() const override { return m_stalled; }

    bool IsRewindStalled() const override { return m_rewindStalled; }

    double GetCurrentPts() override;

    double GetOutputDelay() override; /* returns the expected delay, from that
                                a packet is put in queue */
    int GetDecoderFreeSpace() override { return 0; }

    std::string GetPlayerInfo() override;

    int GetVideoBitrate() override;

    std::string GetStereoMode() override;

    void SetSpeed(int iSpeed) override;

    // classes
    //	CDVDOverlayContainer* m_pOverlayContainer;
    CDVDClock *m_pClock;

protected:
    void OnExit() override;

    void Process() override;

    bool ProcessDecoderOutput(int &decoderState, double &frametime,
                              double &pts);

    int OutputPicture(const DVDVideoPicture *src, double pts);

    //	void ProcessOverlays(DVDVideoPicture* pSource, double pts);
    void OpenStream(CDVDStreamInfo &hint, CDVDVideoCodec *codec);

    void ResetFrameRateCalc();

    void CalcFrameRate();

    int CalcDropRequirement(double pts);

    double m_iSubtitleDelay;

    int m_iLateFrames;
    int m_iDroppedFrames;
    int m_iDroppedRequest;

    double m_fFrameRate = 25; // framerate of the video currently playing
    //	bool m_bCalcFrameRate;     //if we should calculate the
    // framerate from
    // the timestamps 	double m_fStableFrameRate; //place to store
    // calculated framerates 	int m_iFrameRateCount;     //how many
    // calculated framerates we stored in m_fStableFrameRate
    bool m_bAllowDrop; // we can't drop frames until we've calculated
                       // the framerate
    //	int m_iFrameRateErr;       //how many frames we couldn't
    // calculate the
    // framerate, we give up after a while 	int m_iFrameRateLength;
    // //how many seconds we should measure the framerate this is
    // increased exponentially from CVideoPlayerVideo::CalcFrameRate()

    bool m_bFpsInvalid; // needed to ignore fps (e.g. dvd stills)
                        //	bool m_bAllowFullscreen;
    bool m_bRenderSubs;
    float m_fForcedAspectRatio;
    int m_speed;
    std::atomic_bool m_stalled;
    std::atomic_bool m_rewindStalled;
    bool m_paused;
    IDVDStreamPlayer::ESyncState m_syncState;
    std::atomic_bool m_bAbortOutput;

    BitstreamStats m_videoStats;

    CDVDMessageQueue m_messageQueue;
    CDVDMessageQueue &m_messageParent;
    CDVDStreamInfo m_hints;
    CDVDVideoCodec *m_pVideoCodec;
    DVDVideoPicture *m_pTempOverlayPicture;
    //	CPullupCorrection m_pullupCorrection;
    std::list<DVDMessageListItem> m_packets;
    CDroppingStats m_droppingStats;
    CRenderManager &m_renderManager;
    DVDVideoPicture m_picture;
};

NS_KRMOVIE_END