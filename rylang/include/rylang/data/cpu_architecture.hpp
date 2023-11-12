//
// Created by Ryan Nicholl on 10/28/23.
//

#ifndef RPNX_RYANSCRIPT1031_CPU_ARCHITECTURE_HEADER
#define RPNX_RYANSCRIPT1031_CPU_ARCHITECTURE_HEADER

#include <boost/variant.hpp>

namespace rylang
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
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CPU_ARCHITECTURE_HEADER
