//
// Created by Ryan Nicholl on 2/18/24.
//

#ifndef RPNX_QUXLANG_LLVM_LOOKUP_HEADER
#define RPNX_QUXLANG_LLVM_LOOKUP_HEADER

#include "quxlang/data/machine.hpp"
#include <string>

namespace quxlang
{
    inline std::string lookup_llvm_triple(output_info const& info)
    {
        std::string ret;
        "armv8-a-unknown-unknown-unknown";
        switch (info.cpu)
        {
        case cpu::x86_32:
            ret += "i386";
            break;

        case cpu::x86_64:
            ret += "x86_64";
            break;

        case cpu::arm_32:
            ret += "arm";
            break;

        case cpu::arm_64:
            ret += "aarch64";
            break;

        case cpu::riscv_32:
            ret += "riscv32";
            break;

        case cpu::riscv_64:
            ret += "riscv64";
            break;
        }

        ret += "-unknown-";

        switch (info.os)
        {
        case os::linux:
            ret += "linux";
            break;
        case os::windows:
            ret += "windows";
            break;
        case os::macos:
            ret += "darwin";
            break;
        case os::freebsd:
            ret += "freebsd";
            break;
        case os::netbsd:
            ret += "netbsd";
            break;
        case os::openbsd:
            ret += "openbsd";
            break;
        case os::solaris:
            ret += "solaris";
            break;
        default:
            ret += "unknown";
        }

        ret += "-unknown";

        return ret;
    }
} // namespace quxlang

#endif // RPNX_QUXLANG_LLVM_LOOKUP_HEADER
