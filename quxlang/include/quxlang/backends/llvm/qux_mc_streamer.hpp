//
// Created by Ryan Nicholl on 2/24/24.
//

#ifndef RPNX_QUXLANG_QUX_MC_STREAMER_HEADER
#define RPNX_QUXLANG_QUX_MC_STREAMER_HEADER

#include "llvm/MC/MCObjectStreamer.h"
#include "llvm/MC/MCInst.h"

#include <iostream>

namespace quxlang
{
    class mc_collector : public llvm::MCObjectStreamer
    {

      public:

    };

} // namespace quxlang

#endif // RPNX_QUXLANG_QUX_MC_STREAMER_HEADER
