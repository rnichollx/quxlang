//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RPNX_RYANSCRIPT1031_QUAL_CONVERTERS_HEADER
#define RPNX_RYANSCRIPT1031_QUAL_CONVERTERS_HEADER

#include "rylang/data/canonical_type_reference.hpp"
#include "rylang/data/qualified_reference.hpp"

namespace rylang
{
    inline canonical_lookup_chain convert_to_canonical_lookup_chain(qualified_symbol_reference const& ref)
    {
        if (ref.type() == boost::typeindex::type_id< subentity_reference >())
        {
            auto subref = boost::get< subentity_reference >(ref);

            canonical_lookup_chain preimage = convert_to_canonical_type_reference(subref.parent);

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
        for (auto const& i: ref)
        {
           output = subentity_reference{std::move(output), i};
        }
        return output;
    }
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_QUAL_CONVERTERS_HEADER
