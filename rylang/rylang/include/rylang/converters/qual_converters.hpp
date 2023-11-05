//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RPNX_RYANSCRIPT1031_QUAL_CONVERTERS_HEADER
#define RPNX_RYANSCRIPT1031_QUAL_CONVERTERS_HEADER

#include "rylang/data/canonical_type_reference.hpp"
#include "rylang/data/qualified_reference.hpp"

namespace rylang
{

    qualified_symbol_reference convert_to_qualified_symbol_reference(canonical_type_reference const& ref);

    inline canonical_lookup_chain convert_to_canonical_lookup_chain(qualified_symbol_reference const& ref)
    {
        if (ref.type() == boost::typeindex::type_id< subentity_reference >())
        {
            auto subref = boost::get< subentity_reference >(ref);

            canonical_lookup_chain preimage = convert_to_canonical_lookup_chain(subref.parent);

            preimage.push_back(subref.subentity_name);

            return preimage;
        }
        else if (ref.type() == boost::typeindex::type_id< module_reference >())
        {
            canonical_lookup_chain chain;
            // Ignore the module
            return chain;
        }
        else
        {
            throw std::invalid_argument("Invalid conversion");
        }
    }

    inline qualified_symbol_reference convert_to_qualified_symbol_reference(canonical_lookup_chain const& ref)
    {
        qualified_symbol_reference output;
        output = module_reference{"main"};
        for (auto const& i : ref)
        {
            output = subentity_reference{std::move(output), i};
        }
        return output;
    }

    inline qualified_symbol_reference convert_to_qualified_symbol_reference(canonical_pointer_type_reference const& ref)
    {
        return pointer_to_reference{convert_to_qualified_symbol_reference(ref.to)};
    }

    inline qualified_symbol_reference convert_to_qualified_symbol_reference(canonical_type_reference const& ref)
    {
        if (ref.type() == boost::typeindex::type_id< canonical_pointer_type_reference >())
        {
            return convert_to_qualified_symbol_reference(boost::get< canonical_pointer_type_reference >(ref));
        }
        else if (ref.type() == boost::typeindex::type_id< integral_keyword_ast >())
        {
            auto kw = boost::get< integral_keyword_ast >(ref);
            return primitive_type_integer_reference{static_cast<size_t>(kw.size), kw.is_signed };
        }
        else if (ref.type() == boost::typeindex::type_id< canonical_lookup_chain >())
        {
            return convert_to_qualified_symbol_reference(boost::get< canonical_lookup_chain >(ref));
        }
        else
        {
            throw std::invalid_argument("Invalid conversion");
        }
    }

    inline qualified_symbol_reference convert_to_qualified_symbol_reference(canonical_lookup_chain const& ref, call_overload_set cs)
    {
        parameter_set_reference output;
        output = parameter_set_reference{convert_to_qualified_symbol_reference(ref), {}};
        for (auto & x: cs.argument_types)
        {
          output.parameters.push_back(convert_to_qualified_symbol_reference(x));
        }
        return output;
    }
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_QUAL_CONVERTERS_HEADER
