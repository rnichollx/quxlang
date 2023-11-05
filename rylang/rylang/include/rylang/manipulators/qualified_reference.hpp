//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RPNX_RYANSCRIPT1031_MANIPULATORS_QUALIFIED_REFERENCE_HEADER
#define RPNX_RYANSCRIPT1031_MANIPULATORS_QUALIFIED_REFERENCE_HEADER

#include "rylang/data/qualified_reference.hpp"

namespace rylang
{
    std::optional< qualified_symbol_reference > qualified_parent(qualified_symbol_reference input);

    inline bool qualified_is_contextual(qualified_symbol_reference const& ref)
    {
        if (ref.type() == boost::typeindex::type_id< context_reference >())
        {
            return true;
        }
        else if (auto parent = qualified_parent(ref); !parent.has_value())
        {
            return false;
        }
        else
        {
            auto parent2 = qualified_parent(ref);
            assert(parent2.has_value());
            return qualified_is_contextual(parent2.value());
        }
    }

    inline std::optional< qualified_symbol_reference > qualified_parent(qualified_symbol_reference input)
    {
        if (input.type() == boost::typeindex::type_id< subentity_reference >())
        {
            return boost::get< subentity_reference >(input).parent;
        }
        else if (input.type() == boost::typeindex::type_id< pointer_to_reference >())
        {
            return boost::get< pointer_to_reference >(input).target;
        }
        else if (input.type() == boost::typeindex::type_id< parameter_set_reference >())
        {
            return boost::get< parameter_set_reference >(input).callee;
        }
        else
        {
            return std::nullopt;
        }
    }

    inline qualified_symbol_reference with_context(qualified_symbol_reference const& ref, qualified_symbol_reference const& context)
    {
        if (ref.type() == boost::typeindex::type_id< context_reference >())
        {
            return context;
        }
        else if (ref.type() == boost::typeindex::type_id< subentity_reference >())
        {
            return subentity_reference{with_context(boost::get< subentity_reference >(ref).parent, context), boost::get< subentity_reference >(ref).subentity_name};
        }
        else if (ref.type() == boost::typeindex::type_id< pointer_to_reference >())
        {
            return pointer_to_reference{with_context(boost::get< pointer_to_reference >(ref).target, context)};
        }
        else if (ref.type() == boost::typeindex::type_id< parameter_set_reference >())
        {
            parameter_set_reference output = boost::get< parameter_set_reference >(ref);
            output.callee = with_context(output.callee, context);
            for (auto& p : output.parameters)
            {
                p = with_context(p, context);
            }
            return output;
        }
        else
        {
            return ref;
        }
    }

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_QUALIFIED_REFERENCE_HEADER
