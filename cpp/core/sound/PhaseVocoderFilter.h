//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Phase Vocoder Filter
//---------------------------------------------------------------------------

#ifndef PhaseVocoderFilterH
#define PhaseVocoderFilterH

#include "WaveLoopManager.h"
#include "WaveIntf.h"

class tRisaPhaseVocoderDSP;

//---------------------------------------------------------------------------
// tTJSNI_PhaseVocoder
//---------------------------------------------------------------------------
class tTJSNI_PhaseVocoder : public tTJSNativeInstance,
                            public iTVPBasicWaveFilter,
                            public tTVPSampleAndLabelSource {
    typedef tTJSNativeInstance inherited;

public:
    tTJSNI_PhaseVocoder();

    ~tTJSNI_PhaseVocoder() override;

    tjs_error Construct(tjs_int numparams, tTJSVariant **param,
                        iTJSDispatch2 *tjs_obj) override;

    void Invalidate() override;

private:
    int Window; // window size
    int Overlap; // overlap scale
    float Pitch; // pitch scale
    float Time; // time scale

public:
    [[nodiscard]] int GetWindow() const { return Window; }

    void SetWindow(int window);

    [[nodiscard]] int GetOverlap() const { return Overlap; }

    void SetOverlap(int overlap);

    [[nodiscard]] float GetPitch() const { return Pitch; }

    void SetPitch(float pitch) { Pitch = pitch; }

    [[nodiscard]] float GetTime() const { return Time; }

    void SetTime(float time) { Time = time; }

private:
    tTVPSampleAndLabelSource *
    Recreate(tTVPSampleAndLabelSource *source) override;

    // from iTVPBasicWaveFilter
    void Clear() override; // from iTVPBasicWaveFilter
    void Reset() override; // from iTVPBasicWaveFilter
    void Update() override; // from iTVPBasicWaveFilter

private:
    tTVPSampleAndLabelSource *Source; // source filter

    tRisaPhaseVocoderDSP *PhaseVocoder; // Phase Vocoder DSP instance
    char *FormatConvertBuffer; // buffer for converting PCM formats
                               // internally
    size_t FormatConvertBufferSize;

    tTVPWaveFormat InputFormat;
    tTVPWaveFormat OutputFormat;

    tTVPWaveSegmentQueue InputSegments;
    tTVPWaveSegmentQueue OutputSegments;

    void Fill(float *dest, tjs_uint samples, tjs_uint &written,
              tTVPWaveSegmentQueue &segments);

    void Decode(void *dest, tjs_uint samples, tjs_uint &written,
                tTVPWaveSegmentQueue &segments)
        override; // from tTVPSampleAndLabelSource

    [[nodiscard]] const tTVPWaveFormat &GetFormat() const override {
        return OutputFormat;
    }
    // from tTVPSampleAndLabelSource
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTJSNC_PhaseVocoder
//---------------------------------------------------------------------------
class tTJSNC_PhaseVocoder : public tTJSNativeClass {
public:
    tTJSNC_PhaseVocoder();

    static tjs_uint32 ClassID;

private:
    iTJSNativeInstance *CreateNativeInstance() override;
};
//---------------------------------------------------------------------------

#endif
