// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_MANIPULATORS_LLVM_LOOKUP_HEADER_GUARD
#define QUXLANG_MANIPULATORS_LLVM_LOOKUP_HEADER_GUARD

#include "quxlang/data/machine.hpp"
#include "quxlang/exception.hpp"
#include <string>

#ifdef linux
#warning Unsetting linux macro
#undef linux
#endif

namespace quxlang
{
    /**
     * Builds the LLVM target triple for the requested CPU, OS, and binary format.
     */
    inline std::string lookup_llvm_triple(machine_target_info const& info)
    {
        std::string ret;
        switch (info.cpu_type)
        {
        case cpu::none:
            throw compiler_bug("LLVM target triple requires a CPU target");
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

        switch (info.os_type)
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
        case os::none:
            throw compiler_bug("LLVM target triple requires an OS target");
        }

        ret += "-unknown";

        switch (info.binary_type)
        {
        case binary::none:
            throw compiler_bug("LLVM target triple requires a binary type");
        case binary::elf:
            ret += "-elf";
            break;
        case binary::macho:
            ret += "-macho";
            break;
        case binary::pe:
            ret += "-coff";
            break;
        case binary::wasm:
            throw compiler_bug("LLVM target triple does not support Quxlang binary type wasm yet");
        }

        return ret;
    }
} // namespace quxlang

#endif // RPNX_QUXLANG_LLVM_LOOKUP_HEADER
