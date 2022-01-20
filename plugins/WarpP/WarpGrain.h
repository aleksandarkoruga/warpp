#pragma once
////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// Granular UGens ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

inline float GRAIN_IN_AT(Unit* unit, int index, int offset) {
    if (INRATE(index) == calc_FullRate)
        return IN(index)[offset];
    if (INRATE(index) == calc_DemandRate)
        return DEMANDINPUT_A(index, offset + 1);
    return IN0(index);
}

template <bool full_rate> inline float grain_in_at(Unit* unit, int index, int offset) {
    if (full_rate)
        return GRAIN_IN_AT(unit, index, offset);

    if (INRATE(index) == calc_DemandRate)
        return DEMANDINPUT_A(index, offset + 1);
    else
        return IN0(index);
}

inline double sc_gloop(double in, double hi) {
    // avoid the divide if possible
    if (in >= hi) {
        in -= hi;
        if (in < hi)
            return in;
    }
    else if (in < 0.) {
        in += hi;
        if (in >= 0.)
            return in;
    }
    else
        return in;

    return in - hi * floor(in / hi);
}

#define GRAIN_BUF                                                                                                      \
    const SndBuf* buf;                                                                                                 \
    if (bufnum >= world->mNumSndBufs) {                                                                                \
        int localBufNum = bufnum - world->mNumSndBufs;                                                                 \
        Graph* parent = unit->mParent;                                                                                 \
        if (localBufNum <= parent->localBufNum) {                                                                      \
            buf = parent->mLocalSndBufs + localBufNum;                                                                 \
        } else {                                                                                                       \
            bufnum = 0;                                                                                                \
            buf = world->mSndBufs + bufnum;                                                                            \
        }                                                                                                              \
    } else {                                                                                                           \
        if (bufnum < 0) {                                                                                              \
            bufnum = 0;                                                                                                \
        }                                                                                                              \
        buf = world->mSndBufs + bufnum;                                                                                \
    }                                                                                                                  \
    LOCK_SNDBUF_SHARED(buf);                                                                                           \
    const float* bufData __attribute__((__unused__)) = buf->data;                                                      \
    uint32 bufChannels __attribute__((__unused__)) = buf->channels;                                                    \
    uint32 bufSamples __attribute__((__unused__)) = buf->samples;                                                      \
    uint32 bufFrames = buf->frames;                                                                                    \
    int guardFrame __attribute__((__unused__)) = bufFrames - 2;

#define DECLARE_WINDOW                                                                                                 \
    double winPos, winInc, w, b1, y1, y2, y0;                                                                          \
    float amp;                                                                                                         \
    winPos = winInc = w = b1 = y1 = y2 = y0 = amp = 0.;                                                                \
    SndBuf* window;                                                                                                    \
    const float* windowData __attribute__((__unused__)) = 0;                                                           \
    uint32 windowSamples __attribute__((__unused__)) = 0;                                                              \
    uint32 windowFrames __attribute__((__unused__)) = 0;                                                               \
    int windowGuardFrame = 0;


#define CHECK_BUF                                                                                                      \
    if (!bufData) {                                                                                                    \
        unit->mDone = true;                                                                                            \
        ClearUnitOutputs(unit, inNumSamples);                                                                          \
        return;                                                                                                        \
    }

#define GET_GRAIN_WIN_RELAXED(WINTYPE)                                                                                 \
    do {                                                                                                               \
        assert(WINTYPE < unit->mWorld->mNumSndBufs);                                                                   \
        window = unit->mWorld->mSndBufs + (int)WINTYPE;                                                                \
        windowData = window->data;                                                                                     \
        windowSamples = window->samples;                                                                               \
        windowFrames = window->frames;                                                                                 \
        windowGuardFrame = windowFrames - 1;                                                                           \
    } while (0);


static inline bool getGrainWin(Unit* unit, float wintype, SndBuf*& window, const float*& windowData,
    uint32& windowSamples, uint32& windowFrames, int& windowGuardFrame) {
    if (wintype >= unit->mWorld->mNumSndBufs) {
        Print("Envelope buffer out of range!\n");
        return false;
    }

    assert(wintype < unit->mWorld->mNumSndBufs);

    if (wintype < 0)
        return true; // use default hann window

    window = unit->mWorld->mSndBufs + (int)wintype;
    windowData = window->data;
    if (!windowData)
        return false;

    windowSamples = window->samples;
    windowFrames = window->frames;
    windowGuardFrame = windowFrames - 1;

    return true;
}

#define GRAIN_LOOP_BODY_4                                                                                              \
    float amp = y1 * y1;                                                                                               \
    phase = sc_gloop(phase, loopMax);                                                                                  \
    int32 iphase = (int32)phase;                                                                                       \
    float* table1 = bufData + iphase;                                                                                  \
    float* table0 = table1 - 1;                                                                                        \
    float* table2 = table1 + 1;                                                                                        \
    float* table3 = table1 + 2;                                                                                        \
    if (iphase == 0) {                                                                                                 \
        table0 += bufSamples;                                                                                          \
    } else if (iphase >= guardFrame) {                                                                                 \
        if (iphase == guardFrame) {                                                                                    \
            table3 -= bufSamples;                                                                                      \
        } else {                                                                                                       \
            table2 -= bufSamples;                                                                                      \
            table3 -= bufSamples;                                                                                      \
        }                                                                                                              \
    }                                                                                                                  \
    float fracphase = phase - (double)iphase;                                                                          \
    float a = table0[0];                                                                                               \
    float b = table1[0];                                                                                               \
    float c = table2[0];                                                                                               \
    float d = table3[0];                                                                                               \
    float outval = muteAmp * amp * cubicinterp(fracphase, a, b, c, d) ZXP(out1) += outval;                             \
    double y0 = b1 * y1 - y2;                                                                                          \
    y2 = y1;                                                                                                           \
    y1 = y0;

#define GRAIN_LOOP_BODY_2                                                                                              \
    float amp = y1 * y1;                                                                                               \
    phase = sc_gloop(phase, loopMax);                                                                                  \
    int32 iphase = (int32)phase;                                                                                       \
    float* table1 = bufData + iphase;                                                                                  \
    float* table2 = table1 + 1;                                                                                        \
    if (iphase > guardFrame) {                                                                                         \
        table2 -= bufSamples;                                                                                          \
    }                                                                                                                  \
    double fracphase = phase - (double)iphase;                                                                         \
    float b = table1[0];                                                                                               \
    float c = table2[0];                                                                                               \
    float outval = muteAmp * amp * (b + fracphase * (c - b));                                                          \
    ZXP(out1) += outval;                                                                                               \
    double y0 = b1 * y1 - y2;                                                                                          \
    y2 = y1;                                                                                                           \
    y1 = y0;


#define GRAIN_LOOP_BODY_1                                                                                              \
    float amp = y1 * y1;                                                                                               \
    phase = sc_gloop(phase, loopMax);                                                                                  \
    int32 iphase = (int32)phase;                                                                                       \
    float outval = muteAmp * amp * bufData[iphase];                                                                    \
    ZXP(out1) += outval;                                                                                               \
    double y0 = b1 * y1 - y2;                                                                                          \
    y2 = y1;                                                                                                           \
    y1 = y0;

#define BUF_GRAIN_AMP                                                                                                  \
    winPos += winInc;                                                                                                  \
    int iWinPos = (int)winPos;                                                                                         \
    double winFrac = winPos - (double)iWinPos;                                                                         \
    float* winTable1 = windowData + iWinPos;                                                                           \
    float* winTable2 = winTable1 + 1;                                                                                  \
    if (winPos > windowGuardFrame) {                                                                                   \
        winTable2 -= windowSamples;                                                                                    \
    }                                                                                                                  \
    amp = lininterp(winFrac, winTable1[0], winTable2[0]);

#define BUF_GRAIN_LOOP_BODY_4                                                                                          \
    phase = sc_gloop(phase, loopMax);                                                                                  \
    int32 iphase = (int32)phase;                                                                                       \
    float* table1 = bufData + iphase;                                                                                  \
    float* table0 = table1 - 1;                                                                                        \
    float* table2 = table1 + 1;                                                                                        \
    float* table3 = table1 + 2;                                                                                        \
    if (iphase == 0) {                                                                                                 \
        table0 += bufSamples;                                                                                          \
    } else if (iphase >= guardFrame) {                                                                                 \
        if (iphase == guardFrame) {                                                                                    \
            table3 -= bufSamples;                                                                                      \
        } else {                                                                                                       \
            table2 -= bufSamples;                                                                                      \
            table3 -= bufSamples;                                                                                      \
        }                                                                                                              \
    }                                                                                                                  \
    float fracphase = phase - (double)iphase;                                                                          \
    float a = table0[0];                                                                                               \
    float b = table1[0];                                                                                               \
    float c = table2[0];                                                                                               \
    float d = table3[0];                                                                                               \
    float outval = muteAmp * amp * cubicinterp(fracphase, a, b, c, d);                                                 \
    ZXP(out1) += outval;

#define BUF_GRAIN_LOOP_BODY_2                                                                                          \
    phase = sc_gloop(phase, loopMax);                                                                                  \
    int32 iphase = (int32)phase;                                                                                       \
    float* table1 = bufData + iphase;                                                                                  \
    float* table2 = table1 + 1;                                                                                        \
    if (iphase > guardFrame) {                                                                                         \
        table2 -= bufSamples;                                                                                          \
    }                                                                                                                  \
    float fracphase = phase - (double)iphase;                                                                          \
    float b = table1[0];                                                                                               \
    float c = table2[0];                                                                                               \
    float outval = muteAmp * amp * (b + fracphase * (c - b));                                                          \
    ZXP(out1) += outval;

// amp needs to be calculated by looking up values in window

#define BUF_GRAIN_LOOP_BODY_1                                                                                          \
    phase = sc_gloop(phase, loopMax);                                                                                  \
    int32 iphase = (int32)phase;                                                                                       \
    float outval = muteAmp * amp * bufData[iphase];                                                                    \
    ZXP(out1) += outval;


#define SETUP_OUT                                                                                                      \
    uint32 numOutputs = unit->mNumOutputs;                                                                             \
    if (numOutputs > bufChannels) {                                                                                    \
        unit->mDone = true;                                                                                            \
        ClearUnitOutputs(unit, inNumSamples);                                                                          \
        return;                                                                                                        \
    }                                                                                                                  \
    float* out[16];                                                                                                    \
    for (uint32 i = 0; i < numOutputs; ++i)                                                                            \
        out[i] = ZOUT(i);


#define CALC_GRAIN_PAN                                                                                                 \
    float panangle, pan1, pan2;                                                                                        \
    float *out1, *out2;                                                                                                \
    if (numOutputs > 1) {                                                                                              \
        if (numOutputs > 2) {                                                                                          \
            pan = sc_wrap(pan * 0.5f, 0.f, 1.f);                                                                       \
            float cpan = numOutputs * pan + 0.5f;                                                                      \
            float ipan = floor(cpan);                                                                                  \
            float panfrac = cpan - ipan;                                                                               \
            panangle = panfrac * pi2_f;                                                                                \
            grain->chan = (int)ipan;                                                                                   \
            if (grain->chan >= (int)numOutputs)                                                                        \
                grain->chan -= numOutputs;                                                                             \
        } else {                                                                                                       \
            grain->chan = 0;                                                                                           \
            pan = sc_clip(pan * 0.5f + 0.5f, 0.f, 1.f);                                                                \
            panangle = pan * pi2_f;                                                                                    \
        }                                                                                                              \
        pan1 = grain->pan1 = cos(panangle);                                                                            \
        pan2 = grain->pan2 = sin(panangle);                                                                            \
    } else {                                                                                                           \
        grain->chan = 0;                                                                                               \
        pan1 = grain->pan1 = 1.;                                                                                       \
        pan2 = grain->pan2 = 0.;                                                                                       \
    }

#define GET_GRAIN_INIT_AMP                                                                                             \
    if (grain->winType < 0.) {                                                                                         \
        w = pi / counter;                                                                                              \
        b1 = grain->b1 = 2. * cos(w);                                                                                  \
        y1 = sin(w);                                                                                                   \
        y2 = 0.;                                                                                                       \
        amp = y1 * y1;                                                                                                 \
    } else {                                                                                                           \
        amp = windowData[0];                                                                                           \
        winPos = grain->winPos = 0.f;                                                                                  \
        winInc = grain->winInc = (double)windowSamples / counter;                                                      \
    }

#define CALC_NEXT_GRAIN_AMP_INTERNAL                                                                                   \
    do {                                                                                                               \
        y0 = b1 * y1 - y2;                                                                                             \
        y2 = y1;                                                                                                       \
        y1 = y0;                                                                                                       \
        amp = y1 * y1;                                                                                                 \
    } while (0)

#define CALC_NEXT_GRAIN_AMP_CUSTOM                                                                                     \
    do {                                                                                                               \
        winPos += winInc;                                                                                              \
        int iWinPos = (int)winPos;                                                                                     \
        double winFrac = winPos - (double)iWinPos;                                                                     \
        const float* winTable1 = windowData + iWinPos;                                                                 \
        const float* winTable2 = winTable1 + 1;                                                                        \
        if (!windowData)                                                                                               \
            break;                                                                                                     \
        if (winPos > windowGuardFrame)                                                                                 \
            winTable2 -= windowSamples;                                                                                \
        amp = lininterp(winFrac, winTable1[0], winTable2[0]);                                                          \
    } while (0);                                                                                                       \
    if (!windowData)                                                                                                   \
        break;

#define CALC_NEXT_GRAIN_AMP                                                                                            \
    if (grain->winType < 0.) {                                                                                         \
        CALC_NEXT_GRAIN_AMP_INTERNAL;                                                                                  \
    } else {                                                                                                           \
        CALC_NEXT_GRAIN_AMP_CUSTOM;                                                                                    \
    }


#define GET_GRAIN_AMP_PARAMS                                                                                           \
    if (grain->winType < 0.) {                                                                                         \
        b1 = grain->b1;                                                                                                \
        y1 = grain->y1;                                                                                                \
        y2 = grain->y2;                                                                                                \
        amp = grain->curamp;                                                                                           \
    } else {                                                                                                           \
        GET_GRAIN_WIN_RELAXED(grain->winType);                                                                         \
        if (!windowData)                                                                                               \
            break;                                                                                                     \
        winPos = grain->winPos;                                                                                        \
        winInc = grain->winInc;                                                                                        \
        amp = grain->curamp;                                                                                           \
    }

#define SAVE_GRAIN_AMP_PARAMS                                                                                          \
    grain->y1 = y1;                                                                                                    \
    grain->y2 = y2;                                                                                                    \
    grain->winPos = winPos;                                                                                            \
    grain->winInc = winInc;                                                                                            \
    grain->curamp = amp;                                                                                               \
    grain->counter -= nsmps;

#define WRAP_CHAN(offset)                                                                                              \
    out1 = OUT(grain->chan) + offset;                                                                                  \
    if (numOutputs > 1) {                                                                                              \
        if ((grain->chan + 1) >= (int)numOutputs)                                                                      \
            out2 = OUT(0) + offset;                                                                                    \
        else                                                                                                           \
            out2 = OUT(grain->chan + 1) + offset;                                                                      \
    }

#define GET_PAN_PARAMS                                                                                                 \
    float pan1 = grain->pan1;                                                                                          \
    uint32 chan1 = grain->chan;                                                                                        \
    float* out1 = OUT(chan1);                                                                                          \
    if (numOutputs > 1) {                                                                                              \
        pan2 = grain->pan2;                                                                                            \
        uint32 chan2 = chan1 + 1;                                                                                      \
        if (chan2 >= numOutputs)                                                                                       \
            chan2 = 0;                                                                                                 \
        out2 = OUT(chan2);                                                                                             \
    }


#define BUF_GRAIN_LOOP_BODY_4_N                                                                                        \
    phase = sc_gloop(phase, loopMax);                                                                                  \
    int32 iphase = (int32)phase;                                                                                       \
    const float* table1 = bufData + iphase * bufChannels;                                                              \
    const float* table0 = table1 - bufChannels;                                                                        \
    const float* table2 = table1 + bufChannels;                                                                        \
    const float* table3 = table2 + bufChannels;                                                                        \
    if (iphase == 0) {                                                                                                 \
        table0 += bufSamples;                                                                                          \
    } else if (iphase >= guardFrame) {                                                                                 \
        if (iphase == guardFrame) {                                                                                    \
            table3 -= bufSamples;                                                                                      \
        } else {                                                                                                       \
            table2 -= bufSamples;                                                                                      \
            table3 -= bufSamples;                                                                                      \
        }                                                                                                              \
    }                                                                                                                  \
    float fracphase = phase - (double)iphase;                                                                          \
    float a = table0[n];                                                                                               \
    float b = table1[n];                                                                                               \
    float c = table2[n];                                                                                               \
    float d = table3[n];                                                                                               \
    float outval = amp * cubicinterp(fracphase, a, b, c, d);                                                           \
    ZXP(out1) += outval;

#define BUF_GRAIN_LOOP_BODY_2_N                                                                                        \
    phase = sc_gloop(phase, loopMax);                                                                                  \
    int32 iphase = (int32)phase;                                                                                       \
    const float* table1 = bufData + iphase * bufChannels;                                                              \
    const float* table2 = table1 + bufChannels;                                                                        \
    if (iphase > guardFrame) {                                                                                         \
        table2 -= bufSamples;                                                                                          \
    }                                                                                                                  \
    float fracphase = phase - (double)iphase;                                                                          \
    float b = table1[n];                                                                                               \
    float c = table2[n];                                                                                               \
    float outval = muteAmp * amp * (b + fracphase * (c - b));                                                          \
    ZXP(out1) += outval;

// amp needs to be calculated by looking up values in window

#define BUF_GRAIN_LOOP_BODY_1_N                                                                                        \
    phase = sc_gloop(phase, loopMax);                                                                                  \
    int32 iphase = (int32)phase;                                                                                       \
    float outval = muteAmp * amp * bufData[iphase + n];                                                                \
    ZXP(out1) += outval;


#define GET_WARP_WIN_RELAXED(WINTYPE)                                                                                  \
    do {                                                                                                               \
        assert(WINTYPE < unit->mWorld->mNumSndBufs);                                                                   \
        window = unit->mWorld->mSndBufs + (int)WINTYPE;                                                                \
        while (!TRY_ACQUIRE_SNDBUF_SHARED(window)) {                                                                   \
            RELEASE_SNDBUF_SHARED(buf);                                                                                \
            ACQUIRE_SNDBUF_SHARED(buf);                                                                                \
        }                                                                                                              \
        windowData = window->data;                                                                                     \
        if (windowData == NULL)                                                                                        \
            RELEASE_SNDBUF_SHARED(window);                                                                             \
        windowSamples = window->samples;                                                                               \
        windowFrames = window->frames;                                                                                 \
        windowGuardFrame = windowFrames - 1;                                                                           \
    } while (0);

#define GET_WARP_AMP_PARAMS                                                                                            \
    if (grain->winType < 0.) {                                                                                         \
        b1 = grain->b1;                                                                                                \
        y1 = grain->y1;                                                                                                \
        y2 = grain->y2;                                                                                                \
        amp = grain->curamp;                                                                                           \
    } else {                                                                                                           \
        GET_WARP_WIN_RELAXED(grain->winType);                                                                          \
        if (!windowData)                                                                                               \
            break;                                                                                                     \
        winPos = grain->winPos;                                                                                        \
        winInc = grain->winInc;                                                                                        \
        amp = grain->curamp;                                                                                           \
    }

struct WarpWinGrain {
    double phase, rate;
    double winPos, winInc, b1, y1, y2, curamp; // tells the grain where to look in the winBuf for an amp value
    int counter, bufnum, interp;
    float winType;
    bool mute;
};


