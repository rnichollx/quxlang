//
// Created by Ryan Nicholl on 2/24/24.
//

#ifndef QUXLANG_BACKENDS_LLVM_QUX_MC_STREAMER_HEADER_GUARD
#define QUXLANG_BACKENDS_LLVM_QUX_MC_STREAMER_HEADER_GUARD

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
