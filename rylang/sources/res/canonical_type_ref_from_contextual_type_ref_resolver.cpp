//
// Created by Ryan Nicholl on 10/20/23.
//

#include "rylang/compiler.hpp"
#include "rylang/manipulators/qmanip.hpp"
#include "rylang/res/canonical_symbol_from_contextual_symbol_resolver.hpp"

void rylang::canonical_symbol_from_contextual_symbol_resolver::process(rylang::compiler* c)
{

    qualified_symbol_reference context = m_ref.context;
    qualified_symbol_reference const& type = m_ref.type;

    if (type.type() == boost::typeindex::type_id< pointer_to_reference >())
    {
        pointer_to_reference const& ptr = boost::get< pointer_to_reference >(type);

        qualified_symbol_reference to_type = ptr.target;

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

        qualified_symbol_reference canon_ptr_to_type = canonical_to_type_dep->get();

        pointer_to_reference canonical_ptr_type;
        canonical_ptr_type.target = canon_ptr_to_type;

        set_value(canonical_ptr_type);
    }
    else if (type.type() == boost::typeindex::type_id< subentity_reference >())
    {

        subentity_reference const& sub = boost::get< subentity_reference >(type);

        qualified_symbol_reference const& parent = sub.parent;

        if (parent.type() == boost::typeindex::type_id< context_reference >())
        {
            std::optional< qualified_symbol_reference > current_context = context;
            assert(current_context.has_value());
            assert(!qualified_is_contextual(current_context.value()));

            while (current_context.has_value())
            {
                subentity_reference sub2{current_context.value(), sub.subentity_name};

                auto exists_dp = get_dependency(
                    [&]
                    {
                        return c->lk_entity_canonical_chain_exists(sub2);
                    });
                if (!ready())
                    return;

                bool exists = exists_dp->get();

                if (exists)
                {
                    set_value(sub2);
                    return;
                }

                current_context = qualified_parent(current_context.value());
            }

            std::string str = "Could not find '" + sub.subentity_name + "'";
            throw std::logic_error(str.c_str());
        }

        auto parent_dp = get_dependency(
            [&]
            {
                return c->lk_canonical_type_from_contextual_type(parent, context);
            });

        if (!ready())
            return;

        set_value(subentity_reference{parent_dp->get(), sub.subentity_name});

        return;
    }
    else if (type.type() == boost::typeindex::type_id< parameter_set_reference >())
    {
        parameter_set_reference const& param_set = boost::get< parameter_set_reference >(type);

        parameter_set_reference output;

        auto callee_dp = get_dependency(
            [&]
            {
                return c->lk_canonical_type_from_contextual_type(param_set.callee, context);
            });

        if (!ready())
            return;

        output.callee = callee_dp->get();

        for (auto& p : param_set.parameters)
        {
            auto param_dp = get_dependency(
                [&]
                {
                    return c->lk_canonical_type_from_contextual_type(p, context);
                });

            if (!ready())
                return;

            output.parameters.push_back(param_dp->get());
        }

        set_value(output);
    }
    else if (type.type() == boost::typeindex::type_id< primitive_type_integer_reference >())
    {
        set_value(type);
    }
    else if (type.type() == boost::typeindex::type_id< primitive_type_bool_reference >())
    {
        set_value(type);
    }
    else if (type.type() == boost::typeindex::type_id< module_reference >())
    {
        set_value(type);
    }
    else
    {
        throw std::logic_error("unreachable/unimplemented");
    }
}
