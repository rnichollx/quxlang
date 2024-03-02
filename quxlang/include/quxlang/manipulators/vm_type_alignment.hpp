//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef QUXLANG_VM_TYPE_ALIGNMENT_HEADER_GUARD
#define QUXLANG_VM_TYPE_ALIGNMENT_HEADER_GUARD

#include "qmanip.hpp"
#include "quxlang/data/vm_type.hpp"
#include <cstddef>

namespace quxlang
{
    inline std::size_t vm_type_alignment(type_symbol t)
    {
        // TODO: Include VM machine info as argument

        std::string type_string = to_string( t);

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
} // namespace quxlang

#endif // QUXLANG_VM_TYPE_ALIGNMENT_HEADER_GUARD
