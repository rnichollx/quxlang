//
// Created by Ryan Nicholl on 10/24/23.
//

#include "quxlang/compiler.hpp"

#include "quxlang/manipulators/qmanip.hpp"
#include "quxlang/res/entity_canonical_chain_exists_resolver.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(entity_canonical_chain_exists)
{

   // Probably consider this deprecated

    assert(!qualified_is_contextual(input_val));
    if (typeis< module_reference >(input_val))
    {
        std::string module_name = as< module_reference >(input_val).module_name;
        // TODO: Check if module exists
        co_await QUX_CO_DEP(module_ast, (module_name));


        QUX_CO_ANSWER(true);
    }
    else if (typeis< subdotentity_reference >(input_val) || typeis< subentity_reference >(input_val))
    {
        auto parent = qualified_parent(input_val);

        auto parent_exists = co_await QUX_CO_DEP(entity_canonical_chain_exists, (parent.value()));

        if (!parent_exists)
        {
            QUX_CO_ANSWER(false);
        }

        auto declaroids_of = co_await QUX_CO_DEP(declaroids, (input_val));

        // TODO: Filter by include_if here or use symboid_filtered_declaroids

        co_return declaroids_of.size() > 0;
    }
    else if (typeis< instanciation_reference >(input_val))
    {
        instanciation_reference const& ref = as< instanciation_reference >(input_val);
        // Return false if non-match

        // TODO: check this
        auto parent_exists = co_await QUX_CO_DEP(entity_canonical_chain_exists, (ref.callee))

            if (!parent_exists)
        {
            QUX_CO_ANSWER(false);
        }


    /// TODO: Fix this
       // auto val = co_await QUX_CO_DEP(templexoid_instanciation, (ref));

        //co_return val.has_value();

        rpnx::unimplemented();
    }
    else if (typeis< primitive_type_bool_reference >(input_val) || typeis< primitive_type_integer_reference >(input_val) || typeis< numeric_literal_reference >(input_val))
    {
        co_return true;
    }
    else
    {
        throw rpnx::unimplemented();
    }
}
