#pragma once
#define NOMINMAX

#include "VideoPlayer.h"
#include "krmovie.h"
#include "ComplexRect.h"

struct SwsContext;

class iTVPSoundBuffer;

class TVPYUVSprite;

namespace cocos2d {
    class Sprite;

    class Node;
} // namespace cocos2d

NS_KRMOVIE_BEGIN
#define MAX_BUFFER_COUNT 4

class TVPMoviePlayer : public iTVPVideoOverlay, public CBaseRenderer {
public:
    ~TVPMoviePlayer() override;

    void AddRef() override { RefCount++; }

    void Release() override;

    void SetVisible(bool b) override { Visible = b; }

    void Play() override { m_pPlayer->Play(); }

    void Stop() override { m_pPlayer->Stop(); }

    void Pause() override { m_pPlayer->Pause(); }

    void SetPosition(uint64_t tick) override;

    void GetPosition(uint64_t *tick) override;

    void GetStatus(tTVPVideoStatus *status) override;

    void Rewind() override;

    void SetFrame(int f) override;

    void GetFrame(int *f) override;

    void GetFPS(double *f) override;

    void GetNumberOfFrame(int *f) override;

    void GetTotalTime(int64_t *t) override;

    void GetVideoSize(long *width, long *height) override;

    void SetPlayRate(double rate) override;

    void GetPlayRate(double *rate) override;

    void SetAudioBalance(long balance) override;

    void GetAudioBalance(long *balance) override;

    void SetAudioVolume(long volume) override;

    void GetAudioVolume(long *volume) override;

    void GetNumberOfAudioStream(unsigned long *streamCount) override;

    void SelectAudioStream(unsigned long num) override;

    void GetEnableAudioStreamNum(long *num) override;

    void DisableAudioStream() override;

    void GetNumberOfVideoStream(unsigned long *streamCount) override;

    void SelectVideoStream(unsigned long num) override;

    void GetEnableVideoStreamNum(long *num) override;

    // TODO
    void SetStopFrame(int frame) override {}

    void GetStopFrame(int *frame) override {}

    void SetDefaultStopFrame() override {}

    // function for overlay mode
    void SetWindow(class tTJSNI_Window *window) override {}

    void SetMessageDrainWindow(void *window) override {}

    void SetRect(int l, int t, int r, int b) override {}

    // function for layer mode
    tTVPBaseTexture *GetFrontBuffer() override { return nullptr; }

    void SetVideoBuffer(tTVPBaseTexture *buff1, tTVPBaseTexture *buff2,
                        long size) override {}

    // function for mixer mode
    void SetMixingBitmap(class tTVPBaseTexture *dest, float alpha) override {}

    void ResetMixingBitmap() override {}

    void SetMixingMovieAlpha(float a) override {}

    void GetMixingMovieAlpha(float *a) override { *a = 1.0f; }

    void SetMixingMovieBGColor(unsigned long col) override {}

    void GetMixingMovieBGColor(unsigned long *col) override {
        *col = 0xFF000000;
    }

    void PresentVideoImage() override {}

    void GetContrastRangeMin(float *v) override {}

    void GetContrastRangeMax(float *v) override {}

    void GetContrastDefaultValue(float *v) override {}

    void GetContrastStepSize(float *v) override {}

    void GetContrast(float *v) override {}

    void SetContrast(float v) override {}

    void GetBrightnessRangeMin(float *v) override {}

    void GetBrightnessRangeMax(float *v) override {}

    void GetBrightnessDefaultValue(float *v) override {}

    void GetBrightnessStepSize(float *v) override {}

    void GetBrightness(float *v) override {}

    void SetBrightness(float v) override {}

    void GetHueRangeMin(float *v) override {}

    void GetHueRangeMax(float *v) override {}

    void GetHueDefaultValue(float *v) override {}

    void GetHueStepSize(float *v) override {}

    void GetHue(float *v) override {}

    void SetHue(float v) override {}

    void GetSaturationRangeMin(float *v) override {}

    void GetSaturationRangeMax(float *v) override {}

    void GetSaturationDefaultValue(float *v) override {}

    void GetSaturationStepSize(float *v) override {}

    void GetSaturation(float *v) override {}

    void SetSaturation(float v) override {}

    void SetLoopSegement(int beginFrame, int endFrame) override;

    int AddVideoPicture(DVDVideoPicture &pic, int index) override;

    int WaitForBuffer(volatile std::atomic_bool &bStop,
                      int timeout = 0) override;

    void Flush() override;

    bool IsPlaying() const { return m_pPlayer->IsPlaying(); }

    void FrameMove();

protected:
    TVPMoviePlayer();

    iTVPSoundBuffer *GetSoundDevice();

    uint32_t RefCount = 1;
    bool Visible = false;

    BasePlayer *m_pPlayer = nullptr;

    struct BitmapPicture {
        ERenderFormat fmt;
        union {
            uint8_t *data[4];
            uint8_t *rgba;
            uint8_t *yuv[3];
        };
        int width = 0; // pitch = width * 4
        int height = 0;
        double pts;

        BitmapPicture() {
            fmt = RENDER_FMT_NONE;
            for(int i = 0; i < sizeof(data) / sizeof(data[0]); ++i)
                data[i] = nullptr;
        }

        ~BitmapPicture() { Clear(); }

        void swap(BitmapPicture &r);

        void Clear();
    };

    BitmapPicture m_picture[MAX_BUFFER_COUNT];
    int m_curPicture = 0, m_usedPicture = 0;
    std::mutex m_mtxPicture;
    std::condition_variable m_condPicture;
    struct SwsContext *img_convert_ctx = nullptr;
    double m_curpts = 0;
};

class VideoPresentOverlay : public TVPMoviePlayer // cocos2d compatible video
                                                  // display overlay
{
protected:
    cocos2d::Node *m_pRootNode = nullptr;
    TVPYUVSprite *m_pSprite = nullptr;

    ~VideoPresentOverlay() override;

    void ClearNode();

public:
    void PresentPicture(float dt);

    void Stop() override;

    void Play() override;

protected:
    virtual const tTVPRect &GetBounds() = 0;
};

class MoviePlayerOverlay : public VideoPresentOverlay {
    tTJSNI_VideoOverlay *m_pCallbackWin = nullptr;
    tTJSNI_Window *m_pOwnerWindow = nullptr;

    void OnPlayEvent(KRMovieEvent msg, void *p);

public:
    ~MoviePlayerOverlay() override;

    void SetWindow(class tTJSNI_Window *window) override;

    void BuildGraph(tTJSNI_VideoOverlay *callbackwin, IStream *stream,
                    const tjs_char *streamname, const tjs_char *type,
                    uint64_t size);

    const tTVPRect &GetBounds() override;

    void SetVisible(bool b) override;
};

class VideoPresentOverlay2 : public VideoPresentOverlay {
    std::function<const tTVPRect &()> m_funcGetBounds;

public:
    const tTVPRect &GetBounds() override { return m_funcGetBounds(); }

    void SetFuncGetBounds(const std::function<const tTVPRect &()> &func) {
        m_funcGetBounds = func;
    }

    BasePlayer *GetPlayer() { return m_pPlayer; }

    void SetRootNode(cocos2d::Node *node);

    cocos2d::Node *GetRootNode() { return m_pRootNode; }

    static VideoPresentOverlay2 *create();
};

NS_KRMOVIE_END
