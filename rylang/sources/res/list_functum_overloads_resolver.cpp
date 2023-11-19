//
// Created by Ryan Nicholl on 11/18/23.
//
#include "rylang/compiler.hpp"

#include "rylang/res/list_functum_overloads_resolver.hpp"

#include "rylang/operators.hpp"

void rylang::list_functum_overloads_resolver::process(compiler* c)
{
    bool defaulted = false;
    bool constructor = false;
    std::optional< qualified_symbol_reference > parent_opt;

    if (typeis< subdotentity_reference >(m_functum))
    {
        auto parent = as< subdotentity_reference >(m_functum).parent;
        parent_opt = parent;
        auto name = as< subdotentity_reference >(m_functum).subdotentity_name;

        if (typeis< primitive_type_integer_reference >(parent) && name.starts_with("OPERATOR"))
        {
            auto int_type = as< primitive_type_integer_reference >(parent);
            std::vector< call_parameter_information > allowed_operations;

            std::string operator_name = name.substr(8);
            bool is_rhs = false;
            if (operator_name.ends_with("RHS"))
            {
                operator_name = operator_name.substr(0, operator_name.size() - 3);
                allowed_operations.push_back(call_parameter_information{{int_type, int_type}});
                is_rhs = true;
            }

            if (assignment_operators.contains(operator_name) && is_rhs)
            {
                // Cannot assign from RHS for now
                set_value(std::nullopt);
                return;
            }

            if (assignment_operators.contains(operator_name))
            {
                allowed_operations.push_back(call_parameter_information{{make_oref(int_type), int_type}});
                set_value(allowed_operations);
                return;
            }

            allowed_operations.push_back(call_parameter_information{{int_type, int_type}});
            // allowed_operations.push_back(call_parameter_information{{int_type, numeric_literal_reference{}}});
            set_value(allowed_operations);
            return;
        }

        else if (typeis< numeric_literal_reference >(parent) && name.starts_with("OPERATOR"))
        {
            std::vector< call_parameter_information > allowed_operations;

            allowed_operations.push_back(call_parameter_information{{numeric_literal_reference{}, numeric_literal_reference{}}});
            // TODO: MAYBE: Allow composing any integer operation?
            // allowed_operations.push_back(call_parameter_information{{numeric_literal_reference{}, }});
            set_value(allowed_operations);
            return;
        }
        else if (name == "CONSTRUCTOR")
        {
            constructor = true;
            // TODO: Allow disabling default constructor
            defaulted = true;
        }
    }

    auto exists_dp = get_dependency(
        [&]
        {
            return c->lk_entity_canonical_chain_exists(m_functum);
        });

    if (!ready())
    {
        return;
    }

    bool exists = exists_dp->get();

    if (!exists && !defaulted)
    {
        set_value(std::nullopt);
        return;
    }
    std::vector< call_parameter_information > result;

    if (exists)
    {

        auto functum_dp = get_dependency(
            [&]
            {
                return c->lk_entity_ast_from_canonical_chain(m_functum);
            });

        if (!ready())
        {
            return;
        }

        entity_ast functum = functum_dp->get();

        if (!std::holds_alternative< functum_entity_ast >(functum.m_specialization.get()))
        {
            set_value(std::nullopt);
            return;
        }

        functum_entity_ast const& functum_e = std::get< functum_entity_ast >(functum.m_specialization.get());

        for (function_ast const& f : functum_e.m_function_overloads)
        {
            call_parameter_information info;
            for (auto const& arg : f.args)
            {
                contextual_type_reference ctx_type;
                ctx_type.context = m_functum;
                ctx_type.type = arg.type;
                auto type_dp = get_dependency(
                    [&]
                    {
                        return c->lk_canonical_type_from_contextual_type(ctx_type);
                    });
                if (!ready())
                {
                    return;
                }
                info.argument_types.push_back(type_dp->get());
            }

            result.push_back(info);
        }
    }
    if (defaulted)
    {
        call_parameter_information default_consturctor_parameters;
        default_consturctor_parameters.argument_types.push_back(make_mref(parent_opt.value()));

        bool found = false;
        for (auto const& overload : result)
        {
            if (overload.argument_types == default_consturctor_parameters.argument_types)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            result.push_back(default_consturctor_parameters);
        }
    }
    set_value(result);
}
