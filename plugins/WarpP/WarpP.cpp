// PluginWarpP.cpp
// Aleksandar Koruga (aleksandar.koruga@gmail.com)

#include "SC_PlugIn.hpp"
#include "WarpP.hpp"

namespace WarpP {

    WarpP::WarpP() : m_fbufnum(-1e9f), m_buf(nullptr), mGrains{ 0 } {

    mCalcFunc = make_calc_function<WarpP, &WarpP::next>();
    for (int i = 0; i < 16; i++) {
        mNumActive[i] = 0;
        mNextGrain[i] = 1;
    }

    ClearUnitOutputs(this, 1);
   // m_fbufnum = -1e9f;

}

void WarpP::next(int inNumSamples) {
    auto unit = this;
    ClearUnitOutputs(unit, inNumSamples);

        GET_BUF
        SETUP_OUT
        CHECK_BUF

        for (uint32 n = 0; n < numOutputs; n++) {
            int nextGrain = unit->mNextGrain[n];
            
            for (int i = 0; i < unit->mNumActive[n];) {
                WarpWinGrain* grain = unit->mGrains[n] + i;

                double loopMax = (double)bufFrames;

                double rate = grain->rate;
                double phase = grain->phase;
                double muteAmp = grain->mute ? 0.0 : 1.0;
                DECLARE_WINDOW
                GET_GRAIN_AMP_PARAMS
                    float* out1 = out[n];
                int nsmps = sc_min(grain->counter, inNumSamples);
                if (grain->interp >= 4) {
                    for (int j = 0; j < nsmps; ++j) {
                        BUF_GRAIN_LOOP_BODY_4_N
                            CALC_NEXT_GRAIN_AMP
                            phase += rate;
                    }
                }
                else if (grain->interp >= 2) {
                    for (int j = 0; j < nsmps; ++j) {
                        BUF_GRAIN_LOOP_BODY_2_N
                            CALC_NEXT_GRAIN_AMP
                            phase += rate;
                    }
                }
                else {
                    for (int j = 0; j < nsmps; ++j) {
                        BUF_GRAIN_LOOP_BODY_1_N
                            CALC_NEXT_GRAIN_AMP
                            phase += rate;
                    }
                }

                grain->phase = phase;
                SAVE_GRAIN_AMP_PARAMS
                    if (grain->counter <= 0)
                    {
                       
                       // unit->mNumActive[n] = std::max(unit->mNumActive[n], 0);
                        *grain = unit->mGrains[n][--unit->mNumActive[n] ]; // remove grain
                    }
                    else
                        ++i;
            }

            for (int i = 0; i < inNumSamples; ++i) {
                --nextGrain;
                //nextGrain = std::max(nextGrain,0);
                if (nextGrain == 0)
                {
                    // start a grain
                    if (unit->mNumActive[n] + 1 >= kMaxGrains)
                        break;
                    //			uint32 bufnum = (uint32)GRAIN_IN_AT(unit, 0, i);
                    //			if (bufnum >= numBufs) break;

                    float bufSampleRate = buf->samplerate;
                    float bufRateScale = bufSampleRate * SAMPLEDUR;
                    double loopMax = (double)bufFrames;

                    WarpWinGrain* grain = unit->mGrains[n] + unit->mNumActive[n]++;
                    //			grain->bufnum = bufnum;

                    float overlaps = GRAIN_IN_AT(unit, 5, i);
                    float counter = GRAIN_IN_AT(unit, 3, i) * SAMPLERATE;
                    double winrandamt = unit->mParent->mRGen->frand2() * (double)GRAIN_IN_AT(unit, 6, i);
                    counter = sc_max(4., floor(counter + (counter * winrandamt)));
                    grain->counter = (int)counter;

                    nextGrain = (int)(counter / overlaps);

                    unit->mNextGrain[n] = nextGrain;

                    float rate = grain->rate = GRAIN_IN_AT(unit, 2, i) * bufRateScale;
                    float phase = GRAIN_IN_AT(unit, 1, i) * (float)bufFrames;
                    grain->interp = (int)GRAIN_IN_AT(unit, 7, i);
                    float winType = grain->winType = (int)GRAIN_IN_AT(unit, 4, i); // the buffer that holds the grain shape

                    grain->mute = false;
                    if ((unit->mParent->mRGen->frand()) > GRAIN_IN_AT(unit, 8, i))
                    {
                        grain->mute = true;
                    }

                    double muteAmp = grain->mute ? 0.0 : 1.0;

                    DECLARE_WINDOW

                    if (winType >= unit->mWorld->mNumSndBufs) {
                        Print("Envelope buffer out of range!\n");
                        break;
                    }

                    GET_GRAIN_WIN_RELAXED(winType)
                        if (windowData || (winType < 0.)) {
                            GET_GRAIN_INIT_AMP

                                float* out1 = out[n] + i;

                            int nsmps = sc_min(grain->counter, inNumSamples - i);
                            if (grain->interp >= 4) {
                                for (int j = 0; j < nsmps; ++j) {
                                    BUF_GRAIN_LOOP_BODY_4_N
                                        CALC_NEXT_GRAIN_AMP
                                        phase += rate;
                                }
                            }
                            else if (grain->interp >= 2) {
                                for (int j = 0; j < nsmps; ++j) {
                                    BUF_GRAIN_LOOP_BODY_2_N
                                        CALC_NEXT_GRAIN_AMP
                                        phase += rate;
                                }
                            }
                            else {
                                for (int j = 0; j < nsmps; ++j) {
                                    BUF_GRAIN_LOOP_BODY_1_N
                                        CALC_NEXT_GRAIN_AMP
                                        phase += rate;
                                }
                            }

                            grain->phase = phase;
                            SAVE_GRAIN_AMP_PARAMS
                                // end change
                                if (grain->counter <= 0)
                                { 
                                    grain->mute = false;
                                    *grain = unit->mGrains[n][--unit->mNumActive[n]]; // remove grain
                                }
                        }
                }
            }

            unit->mNextGrain[n] = nextGrain;
        }
}

} // namespace WarpP

PluginLoad(WarpPUGens) {
    // Plugin magic
    ft = inTable;
    registerUnit<WarpP::WarpP>(ft, "WarpP", true);
}
