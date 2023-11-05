//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_TYPE_ALIGNMENT_HEADER
#define RPNX_RYANSCRIPT1031_VM_TYPE_ALIGNMENT_HEADER

#include "rylang/data/vm_type.hpp"
#include <cstddef>

namespace rylang
{
    inline std::size_t vm_type_alignment(vm_type t)
    {
        // TODO: Include VM machine info as argument

        if (t.type() == boost::typeindex::type_id< vm_type_int >())
        {
            // TODO: This probably isn't right
            return (boost::get< vm_type_int >(t).size + 7) / 8;
        }
        else if (t.type() == boost::typeindex::type_id< vm_type_pointer >())
        {
            // TODO: This obviously isn't correct on all machines, but we'll fix that later.
            return 8;
        }
        else
        {
            throw std::runtime_error("Invalid type");
        }
    }
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_TYPE_ALIGNMENT_HEADER
