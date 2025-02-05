#include "vorbis/vorbisfile.h"
#include "VorbisWaveDecoder.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "WaveIntf.h"
#include "SysInitIntf.h"
#include "StorageIntf.h"
#include "DebugIntf.h"
#include <algorithm>

static const bool FloatExtraction =
    false; // true if output format is IEEE 32-bit float

#define WARN_OLD_VORBIS_DATE 20020717
// warn if the stream is generated by old vorbis encoder earlier than
// this
#define WARN_PREFER_ENCODER_VERSION                                            \
    "Vorbis encoder 1.0 (dated 2002/07/17) or later is recommended."
static bool OldEncoderWarned = false;
// static LONG AllocCount = 0; // memory block allocation count for
// decoder

// static bool Look_Replay_Gain = false; // whether to look replay
// gain information static bool Use_Album_Gain = false; // whether to
// use album gain, otherwise use track gain

class VorbisWaveDecoder : public tTVPWaveDecoder // decoder interface
{
    bool InputFileInit; // whether InputFile is inited
    OggVorbis_File InputFile; // OggVorbis_File instance
    tTJSBinaryStream *InputStream; // input stream
    tTVPWaveFormat Format; // output PCM format
    int CurrentSection; // current section in vorbis stream
    int pcmsize;

public:
    VorbisWaveDecoder() :
        InputFileInit(false), InputStream(nullptr), CurrentSection(-1) {
        pcmsize = FloatExtraction ? 4 : 2;
    }

    ~VorbisWaveDecoder() {
        if(InputFileInit) {
            ov_clear(&InputFile);
            InputFileInit = false;
        }
        if(InputStream) {
            delete InputStream;
        }
    }

public:
    // ITSSWaveDecoder
    virtual void GetFormat(tTVPWaveFormat &format) { format = Format; }

    virtual bool Render(void *buf, tjs_uint bufsamplelen, tjs_uint &rendered);

    virtual bool SetPosition(tjs_uint64 samplepos);

    virtual bool DesiredFormat(const tTVPWaveFormat &format) {
        if(format.IsFloat == Format.IsFloat)
            return false;
        if(format.IsFloat) {
            pcmsize = 4;
            Format.IsFloat = true;
            Format.BitsPerSample = 32;
            Format.BytesPerSample = 4;
        } else {
            pcmsize = 2;
            Format.IsFloat = false;
            Format.BitsPerSample = 16;
            Format.BytesPerSample = 2;
        }
        return true;
    }

    // others
    bool SetStream(const ttstr &storagename);

private:
    size_t static read_func(void *ptr, size_t size, size_t nmemb,
                            void *datasource) {
        VorbisWaveDecoder *decoder = (VorbisWaveDecoder *)datasource;
        if(!decoder->InputStream)
            return 0;
        return decoder->InputStream->Read(ptr, size * nmemb) / size;
    }

    int static seek_func(void *datasource, ogg_int64_t offset, int whence) {
        VorbisWaveDecoder *decoder = (VorbisWaveDecoder *)datasource;
        if(!decoder->InputStream)
            return -1;

        tjs_uint64 result;
        int seek_type = TJS_BS_SEEK_SET;

        switch(whence) {
            case SEEK_SET:
                seek_type = TJS_BS_SEEK_SET;
                break;
            case SEEK_CUR:
                seek_type = TJS_BS_SEEK_CUR;
                break;
            case SEEK_END:
                seek_type = TJS_BS_SEEK_END;
                break;
        }

        result = decoder->InputStream->Seek(offset, seek_type);
        return 0;
    }

    int static close_func(void *datasource) {
        VorbisWaveDecoder *decoder = (VorbisWaveDecoder *)datasource;
        if(!decoder->InputStream)
            return EOF;
        delete decoder->InputStream;
        decoder->InputStream = nullptr;
        return 0;
    }

    long static tell_func(void *datasource) {
        VorbisWaveDecoder *decoder = (VorbisWaveDecoder *)datasource;
        if(!decoder->InputStream)
            return EOF;
        return decoder->InputStream->Seek(0, TJS_BS_SEEK_CUR);
    }
};

tTVPWaveDecoder *VorbisWaveDecoderCreator::Create(const ttstr &storagename,
                                                  const ttstr &extension) {
    static ttstr strext = TJS_W(".ogg");
    VorbisWaveDecoder *decoder = nullptr;
    if(extension == strext) {
        decoder = new VorbisWaveDecoder();
        if(!decoder->SetStream(storagename)) {
            delete decoder;
            decoder = nullptr;
        }
    }
    return decoder;
}

bool VorbisWaveDecoder::Render(void *buf, tjs_uint bufsamplelen,
                               tjs_uint &rendered) {
    // render output PCM
    if(!InputFileInit)
        return false; // InputFile is yet not inited

    int pos = 0; // decoded PCM (in bytes)
    if(Format.IsFloat) {
        int remain = bufsamplelen;
        float *p = (float *)buf;
        while(remain) {
            // float *p = (float*)buf;
            int samples;
            do {
                float **pcm;
                int current_section;
                samples =
                    ov_read_float(&InputFile, &pcm, remain, &current_section);
                for(int j = 0; j < samples; j++)
                    for(int i = 0; i < Format.Channels; i++) {
                        *p++ = pcm[i][j];
                    }
            } while(samples < 0); // ov_read would return a negative number
            // if the decoding is not ready
            if(samples == 0)
                break;
            pos += samples;
            remain -= samples;
        }
    } else {
        int remain = bufsamplelen * Format.Channels *
            pcmsize; // remaining PCM (in bytes)
        while(remain) {
            int res;
            do {
                res = ov_read(&InputFile, ((char *)buf + pos), remain, 0,
                              pcmsize, 1,
                              &CurrentSection); // decode via ov_read
            } while(res < 0); // ov_read would return a negative number
            // if the decoding is not ready
            if(res == 0)
                break;
            pos += res;
            remain -= res;
        }
        pos /= (Format.Channels * pcmsize); // convert to PCM position
    }

    rendered = pos; // return rendered PCM samples

    return (unsigned int)pos >= bufsamplelen; // if the decoding is ended
}

bool VorbisWaveDecoder::SetPosition(tjs_uint64 samplepos) {
    // set PCM position (seek)
    if(!InputFileInit)
        return false;

    if(0 != ov_pcm_seek(&InputFile, samplepos)) {
        return false;
    }

    return true;
}

bool VorbisWaveDecoder::SetStream(const ttstr &url) {
    // set input stream
    InputStream = TVPCreateStream(url, TJS_BS_READ);
    if(!InputStream)
        return false;

    ov_callbacks callbacks = { read_func, seek_func, close_func, tell_func };
    // callback functions

    // open input stream via ov_open_callbacks
    if(ov_open_callbacks(this, &InputFile, nullptr, 0, callbacks) < 0) {
        // error!
        delete InputStream;
        InputStream = nullptr;
        return false;
    }

    InputFileInit = true;

    // retrieve PCM information
    vorbis_info *vi;
    vi = ov_info(&InputFile, -1);
    if(!vi) {
        return false;
    }

    // set Format up
    Format.BitsPerSample = FloatExtraction ? 32 : 16;
    Format.BytesPerSample = Format.BitsPerSample / 8;
    Format.Channels = vi->channels;
    Format.IsFloat = FloatExtraction;
    Format.SamplesPerSec = vi->rate;
    Format.Seekable = 2;
    Format.SpeakerConfig = 0;
    //	Format.Signed = true;
    //	Format.BigEndian = false;

    ogg_int64_t pcmtotal = ov_pcm_total(&InputFile, -1); // PCM total samples
    if(pcmtotal < 0)
        pcmtotal = 0;
    Format.TotalSamples = pcmtotal;

    double timetotal = ov_time_total(&InputFile, -1); // total time in sec.
    if(timetotal < 0)
        Format.TotalTime = 0;
    else
        Format.TotalTime = timetotal * 1000.0;

    // check old vorbis stream, which is generated by old encoder.
    if(!OldEncoderWarned) {
        vorbis_comment *vc = ov_comment(&InputFile, -1);
        if(vc->vendor) {
            const char *vorbis_date = strstr(vc->vendor, "libVorbis I ");
            if(vorbis_date) {
                int date = atoi(vorbis_date + 12);
                if(date < WARN_OLD_VORBIS_DATE) {
                    OldEncoderWarned = true;
                    TVPAddLog(TJS_W("wuvorbis: warning: The Vorbis "
                                    "stream \"") +
                              ttstr(url) +
                              TJS_W("\" seems to be generated by old "
                                    "Vorbis encoder "
                                    "[") +
                              ttstr(vc->vendor) +
                              TJS_W("]. Check that your encoder is "
                                    "new enough, unless "
                                    "you prefer the old encoder. The "
                                    "old stream can "
                                    "still be decoded, but ") +
                              ttstr(WARN_PREFER_ENCODER_VERSION));
                }
            }
        }
    }

    //     if(Look_Replay_Gain)
    //     {
    //         for(int i = 0; i < InputFile.links; i++)
    //         {
    //             // for each stream
    //             float gain = 1.0;
    //             vorbis_comment *vc = ov_comment(&InputFile, i);
    //             const char * track = vorbis_comment_query(vc,
    //             "replaygain_track_gain", 0); const char * album =
    //             vorbis_comment_query(vc, "replaygain_album_gain",
    //             0);
    //
    //             const char * sel = Use_Album_Gain ? ( album ? album
    //             : track ) : track; if(sel)
    //             {
    //                 float db = (float)atof(sel); // in db
    //                 gain *= (float)pow(10.0, db / 20); // convert
    //                 db to multiplier
    //             }
    //
    //             vorbis_info_set_global_gain(ov_info(&InputFile,
    //             i)->, gain);
    //         }
    //     }

    return true;
}

#include <opus/opusfile.h>

class OpusWaveDecoder : public tTVPWaveDecoder // decoder interface
{
    OggOpusFile *InputFile; // OggOpusFile instance
    tTJSBinaryStream *InputStream; // input stream
    tTVPWaveFormat Format; // output PCM format
    opus_int16 *Buffer, *pBuffer;
    int BufferRemain; // in samples

private:
    int static read_func(void *datasource, unsigned char *ptr, int size) {
        OpusWaveDecoder *decoder = (OpusWaveDecoder *)datasource;
        if(!decoder->InputStream)
            return -1;
        return decoder->InputStream->Read(ptr, size);
    }

    int static seek_func(void *datasource, opus_int64 offset, int whence) {
        OpusWaveDecoder *decoder = (OpusWaveDecoder *)datasource;
        if(!decoder->InputStream)
            return -1;

        tjs_uint64 result;
        int seek_type = TJS_BS_SEEK_SET;

        switch(whence) {
            case SEEK_SET:
                seek_type = TJS_BS_SEEK_SET;
                break;
            case SEEK_CUR:
                seek_type = TJS_BS_SEEK_CUR;
                break;
            case SEEK_END:
                seek_type = TJS_BS_SEEK_END;
                break;
        }

        result = decoder->InputStream->Seek(offset, seek_type);
        return 0;
    }

    int static close_func(void *datasource) {
        OpusWaveDecoder *decoder = (OpusWaveDecoder *)datasource;
        if(!decoder->InputStream)
            return EOF;
        delete decoder->InputStream;
        decoder->InputStream = nullptr;
        return 0;
    }

    opus_int64 static tell_func(void *datasource) {
        OpusWaveDecoder *decoder = (OpusWaveDecoder *)datasource;
        if(!decoder->InputStream)
            return EOF;
        return decoder->InputStream->GetPosition();
    }

public:
    OpusWaveDecoder() :
        InputFile(nullptr), InputStream(nullptr), Buffer(nullptr) {}

    ~OpusWaveDecoder() {
        if(InputFile) {
            op_free(InputFile);
            InputFile = nullptr;
        }
        if(InputStream) {
            delete InputStream;
        }
        if(Buffer) {
            delete[] Buffer;
        }
    }

    int BufferRead(opus_int16 *buf, int samples_to_read) {
        int res = std::min(BufferRemain, samples_to_read);
        memcpy(buf, pBuffer, res * Format.Channels * sizeof(opus_int16));
        pBuffer += res * Format.Channels;
        BufferRemain -= res;
        if(BufferRemain <= 0)
            pBuffer = nullptr;
        return res;
    }

public:
    // ITSSWaveDecoder
    virtual void GetFormat(tTVPWaveFormat &format) { format = Format; }

    virtual bool Render(void *_buf, tjs_uint bufsamplelen, tjs_uint &rendered) {
        // render output PCM
        if(!InputFile)
            return false; // InputFile is yet not inited
        opus_int16 *buf = (opus_int16 *)_buf;
        int res;
        int pos = 0; // decoded PCM (in sample)
        int remain = bufsamplelen; // remaining PCM (in sample)

        while(remain) {
            if(pBuffer) {
                res = BufferRead(buf, remain);
            } else if(remain >= 5760) {
                res = op_read(InputFile, buf, remain * Format.Channels,
                              nullptr); // decode via ov_read
            } else {
                res =
                    op_read(InputFile, Buffer, 5760 * Format.Channels, nullptr);
                if(res > remain) {
                    memcpy(buf, Buffer,
                           remain * Format.Channels * sizeof(opus_int16));
                    BufferRemain = res - remain;
                    pBuffer = Buffer + remain * Format.Channels;
                    res = remain;
                } else {
                    memcpy(buf, Buffer,
                           res * Format.Channels * sizeof(opus_int16));
                }
            }
            // if the decoding is not ready
            if(res <= 0)
                break;
            pos += res;
            remain -= res;
            buf += res * Format.Channels;
        }

        rendered = pos; // return rendered PCM samples

        return !remain; // if the decoding is ended
    }

    virtual bool SetPosition(tjs_uint64 samplepos) {
        // set PCM position (seek)
        if(!InputFile)
            return false;

        if(0 != op_pcm_seek(InputFile, samplepos)) {
            return false;
        }

        return true;
    }

    // others
    bool SetStream(const ttstr &url) {
        // set input stream
        InputStream = TVPCreateStream(url, TJS_BS_READ);
        if(!InputStream)
            return false;

        OpusFileCallbacks callbacks = { read_func, seek_func, tell_func,
                                        close_func };
        // callback functions

        // open input stream via ov_open_callbacks
        int ret;
        InputFile = op_open_callbacks(this, &callbacks, nullptr, 0, &ret);
        if(!InputFile) {
            // error!
            delete InputStream;
            InputStream = nullptr;
            return false;
        }

        // set Format up
        Format.BitsPerSample = 16;
        Format.BytesPerSample = Format.BitsPerSample / 8;
        Format.Channels = op_channel_count(InputFile, -1);
        Format.IsFloat = false;
        Format.SamplesPerSec = 48000;
        Format.Seekable = op_seekable(InputFile) ? 0 : 2;
        Format.SpeakerConfig = 0;
        // 		Format.Signed = true;
        // 		Format.BigEndian = false;

        ogg_int64_t pcmtotal = op_pcm_total(InputFile, -1); // PCM total samples
        if(pcmtotal < 0)
            pcmtotal = 0;
        Format.TotalSamples = pcmtotal;
        Format.TotalTime = pcmtotal * (1000.0 / 48000);

        Buffer = new opus_int16[Format.Channels * 5760]; // 120 ms in 48kHz
        BufferRemain = 0;
        pBuffer = nullptr;

        return true;
    }
};

tTVPWaveDecoder *OpusWaveDecoderCreator::Create(const ttstr &storagename,
                                                const ttstr &extension) {
    OpusWaveDecoder *decoder = nullptr;
    decoder = new OpusWaveDecoder();
    if(!decoder->SetStream(storagename)) {
        delete decoder;
        decoder = nullptr;
    }
    return decoder;
}
