// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_MANIPULATORS_VM_TYPE_ALIGNMENT_HEADER_GUARD
#define QUXLANG_MANIPULATORS_VM_TYPE_ALIGNMENT_HEADER_GUARD

#include "qmanip.hpp"
#include "quxlang/data/vm_type.hpp"
#include <cstddef>

namespace quxlang
{
    inline std::size_t vm_type_alignment(type_symbol t)
    {
        // TODO: Include VM machine info as argument

        std::string type_string = to_string( t);

        if (t.template type_is< int_type >())
        {
            // TODO: This probably isn't right
            return (as< int_type >(t).bits + 7) / 8;
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
