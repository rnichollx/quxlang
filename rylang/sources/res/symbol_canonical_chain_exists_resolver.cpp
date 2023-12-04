//
// Created by Ryan Nicholl on 11/12/23.
//

#include "rylang/data/qualified_symbol_reference.hpp"
#include "rylang/variant_utils.hpp"

void rylang::symbol_canonical_chain_exists_resolver::process(compiler* c)
{
    auto const & chain = this->m_chain;

    if (typeis< functanoid_reference >(chain))
    {
        functanoid_reference const& param_set = boost::get< functanoid_reference >(chain);
        // We only care if the parent exists, we can exist but be an invalid functaniod otherwise?
        // Maybe this resolver should be renamed...

        assert(param_set.callee.type() != boost::typeindex::type_id< context_reference >());

        // TODO: Distingiush between the functum/template existing but not the particular
        //  functanoid/temtanoid existing, and the functum/template not existing at all.
        //  Currently it's not possible for this distinction to matter, since it is impossible
        //  for a contextual type to be converted to a parameter set with the callee being
        //  a context_reference, due to the way the collector parses conxtual qualified
        //  references into the abstract syntax tree from the input text, thus the only
        //  case in which this would be significant is where a template is used which
        //  isn't implemented yet.
        auto parent_exists_dp = get_dependency(
            [&]
            {
                return c->lk_entity_canonical_chain_exists(param_set.callee);
            });

        if (!ready())
        {
            return;
        }

        bool parent_exists = parent_exists_dp->get();

        set_value(parent_exists);
        return;
    } else
    {
        auto entity_exists_dp = get_dependency(
            [&]
            {
                return c->lk_entity_canonical_chain_exists(chain);
            });

        if (!ready())
        {
            return;
        }

        bool entity_exists = entity_exists_dp->get();
        set_value(entity_exists);
    }
}
