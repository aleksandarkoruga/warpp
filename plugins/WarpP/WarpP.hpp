// PluginWarpP.hpp
// Aleksandar Koruga (aleksandar.koruga@gmail.com)

#pragma once

#include "SC_PlugIn.hpp"

namespace WarpP {

class WarpP : public SCUnit {
public:
    WarpP();

    // Destructor
    // ~WarpP();

private:
    // Calc function
    void next(int nSamples);

    // Member variables
};

} // namespace WarpP
