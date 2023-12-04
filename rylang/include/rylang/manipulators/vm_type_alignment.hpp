//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_TYPE_ALIGNMENT_HEADER
#define RPNX_RYANSCRIPT1031_VM_TYPE_ALIGNMENT_HEADER

#include "qmanip.hpp"
#include "rylang/data/vm_type.hpp"
#include <cstddef>

namespace rylang
{
    inline std::size_t vm_type_alignment(qualified_symbol_reference t)
    {
        // TODO: Include VM machine info as argument

        std::string type_string = boost::apply_visitor(qualified_symbol_stringifier(), t);

        if (t.type() == boost::typeindex::type_id< primitive_type_integer_reference >())
        {
            // TODO: This probably isn't right
            return (boost::get< primitive_type_integer_reference >(t).bits + 7) / 8;
        }
        else if (is_ptr(t) || is_ref(t))
        {
            // TODO: This obviously isn't correct on all machines, but we'll fix that later.
            return 8;
        }
        else
        {
            return rpnx::unimplemented();
        }
    }
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_TYPE_ALIGNMENT_HEADER
