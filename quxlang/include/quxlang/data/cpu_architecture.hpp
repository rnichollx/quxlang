//
// Created by Ryan Nicholl on 10/28/23.
//

#ifndef QUXLANG_CPU_ARCHITECTURE_HEADER_GUARD
#define QUXLANG_CPU_ARCHITECTURE_HEADER_GUARD

#include <boost/variant.hpp>

namespace quxlang
{
    struct cpu_arch_x86
    {
    };
    struct cpu_arch_x64
    {
    };
    struct cpu_arch_arm
    {
    };
    struct cpu_arch_armv8a
    {
    };

    using cpu_arch = boost::variant< cpu_arch_x86, cpu_arch_x64, cpu_arch_arm, cpu_arch_armv8a >;
} // namespace quxlang

#endif // QUXLANG_CPU_ARCHITECTURE_HEADER_GUARD
