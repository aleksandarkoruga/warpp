// PluginWarpP.hpp
// Aleksandar Koruga (aleksandar.koruga@gmail.com)

#pragma once

#include "SC_PlugIn.hpp"
static InterfaceTable* ft;
const int kMaxGrains = 64;
#include "WarpGrain.h"

namespace WarpP {

class WarpP : public SCUnit {
public:
    WarpP();
    
    // Destructor
    // ~WarpP();

private:
    // Calc function
    void next(int inNumSamples);
    // Member variables

    float m_fbufnum;
    SndBuf* m_buf;
    int mNumActive[16];
    int mNextGrain[16];
    WarpWinGrain mGrains[16][kMaxGrains];
};

} // namespace WarpP
