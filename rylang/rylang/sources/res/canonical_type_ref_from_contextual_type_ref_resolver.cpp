//
// Created by Ryan Nicholl on 10/20/23.
//

#include "rylang/res/canonical_type_ref_from_contextual_type_ref_resolver.hpp"
#include "rylang/compiler.hpp"

void rylang::canonical_type_ref_from_contextual_type_ref_resolver::process(rylang::compiler* c)
{
    // TODO: implement this :D

    // For now just assume all types are canonical.

    canonical_lookup_chain context = m_ref.context;
    type_reference const& type = m_ref.type;

    if (type.type() == boost::typeindex::type_id< pointer_reference >())
    {
        pointer_reference const& ptr = boost::get< pointer_reference >(type);

        type_reference to_type = ptr.to;

        // we need to canonicalize the type_reference

        contextual_type_reference to_type_ref;
        to_type_ref.type = to_type;
        to_type_ref.context = m_ref.context;

        auto canonical_to_type_dep = get_dependency(
            [&]
            {
                return c->lk_canonical_type_from_contextual_type(to_type_ref);
            });

        if (!ready())
            return;

        canonical_type_reference canon_ptr_to_type = canonical_to_type_dep->get();

        canonical_pointer_type_reference canonical_ptr_type;
        canonical_ptr_type.to = canon_ptr_to_type;

        set_value(canonical_ptr_type);
    }
    else if (type.type() == boost::typeindex::type_id< proximate_lookup_reference >())
    {
        canonical_lookup_chain output;

        // TODO: impelment contextual logic here to get context.
        auto const& lookup = boost::get< proximate_lookup_reference >(type);

        for (int i = lookup.chain.chain.size(); i != 0; i--)
        {
            canonical_lookup_chain fused_chain = context;
            for (int j = 0; j < i; j++)
            {
                fused_chain.push_back(lookup.chain.chain[j].identifier);
            }

            auto fused_chain_exists_dp = get_dependency(
                [&]
                {
                    return c->lk_entity_canonical_chain_exists(fused_chain);
                });

            if (!ready())
                return;

            bool fused_chain_exists = fused_chain_exists_dp->get();

            if (fused_chain_exists)
            {
                output = fused_chain;
                set_value(output);
                return;
            }
        }

        for (auto& element : lookup.chain.chain)
        {
            assert(element.type == lookup_type::scope);
            output.push_back(element.identifier);
        }

        set_value(output);
        return;
    }
    else if (type.type() == boost::typeindex::type_id< absolute_lookup_reference >())
    {
        canonical_lookup_chain output;

        auto const& lookup = boost::get< absolute_lookup_reference >(type);

        for (auto& element : lookup.chain.chain)
        {
            assert(element.type == lookup_type::scope);
            output.push_back(element.identifier);
        }

        set_value(output);
    }
    else if (type.type() == boost::typeindex::type_id< integral_keyword_ast >())
    {

        set_value(boost::get< integral_keyword_ast >(type));
    }
    else
    {
        throw std::logic_error("unreachable/unimplemented");
    }
}
