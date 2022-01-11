// PluginWarpP.cpp
// Aleksandar Koruga (aleksandar.koruga@gmail.com)

#include "SC_PlugIn.hpp"
#include "WarpP.hpp"

static InterfaceTable* ft;

namespace WarpP {

WarpP::WarpP() {
    mCalcFunc = make_calc_function<WarpP, &WarpP::next>();
    next(1);
}

void WarpP::next(int nSamples) {
    const float* input = in(0);
    const float* gain = in(1);
    float* outbuf = out(0);

    // simple gain function
    for (int i = 0; i < nSamples; ++i) {
        outbuf[i] = input[i] * gain[i];
    }
}

} // namespace WarpP

PluginLoad(WarpPUGens) {
    // Plugin magic
    ft = inTable;
    registerUnit<WarpP::WarpP>(ft, "WarpP", false);
}
