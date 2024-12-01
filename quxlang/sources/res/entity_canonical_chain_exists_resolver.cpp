// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/compiler.hpp"

#include "quxlang/manipulators/qmanip.hpp"
#include "quxlang/res/entity_canonical_chain_exists_resolver.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(entity_canonical_chain_exists)
{
   co_return co_await QUX_CO_DEP(exists, (input_val));
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(exists)
{

    std::string check_sym = quxlang::to_string(input);
    // Probably consider this deprecated

    assert(!qualified_is_contextual(input_val));
    if (typeis< module_reference >(input_val))
    {
        std::string module_name = as< module_reference >(input_val).module_name;
        // TODO: Check if module exists
        co_await QUX_CO_DEP(module_ast, (module_name));

        QUX_CO_ANSWER(true);
    }
    else if (typeis< submember >(input_val) || typeis< subsymbol >(input_val))
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
    else if (typeis< instantiation_type >(input_val))
    {
        instantiation_type const& ref = as< instantiation_type >(input_val);
        // Return false if non-match

        // TODO: check this
        auto parent_exists = co_await QUX_CO_DEP(entity_canonical_chain_exists, (ref.callee));

        if (!parent_exists)
        {
            QUX_CO_ANSWER(false);
        }

        /// TODO: Fix this
        // auto val = co_await QUX_CO_DEP(templexoid_instanciation, (ref));

        // co_return val.has_value();

        rpnx::unimplemented();
    }
    else if (typeis< bool_type >(input_val) || typeis< int_type >(input_val) || typeis< numeric_literal_reference >(input_val))
    {
        co_return true;
    }
    else if (typeis<void_type>(input_val))
    {
        // It's defined, but doesn't "exist".
        co_return false;
    }
    else if (typeis< pointer_type >(input_val))
    {
        auto pointed_value_exists = co_await QUX_CO_DEP(entity_canonical_chain_exists, (as< pointer_type >(input_val).target));

        co_return pointed_value_exists;
    }
    else
    {
        throw rpnx::unimplemented();
    }

    throw rpnx::unimplemented();
}
