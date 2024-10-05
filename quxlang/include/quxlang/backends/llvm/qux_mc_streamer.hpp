// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

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
