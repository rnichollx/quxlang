//
// Created by Ryan Nicholl on 11/25/23.
//

#include "rylang/operators.hpp"
#include "rylang/res/functum_builtin_overloads_resolver.hpp"
namespace rylang
{

    template < typename Graph >
    auto list_builtin_functum_overloads_question_f(Graph* g, qualified_symbol_reference functum) -> rpnx::resolver_coroutine< Graph, std::optional< std::set< call_parameter_information > > >
    {
        bool defaulted = false;
        bool constructor = false;
        std::optional< qualified_symbol_reference > parent_opt;

        std::string name = to_string(functum);

        if (typeis< subdotentity_reference >(functum))
        {
            auto parent = as< subdotentity_reference >(functum).parent;
            parent_opt = parent;
            auto name = as< subdotentity_reference >(functum).subdotentity_name;

            if (typeis< primitive_type_integer_reference >(parent) && name.starts_with("OPERATOR"))
            {
                auto int_type = as< primitive_type_integer_reference >(parent);
                std::set< call_parameter_information > allowed_operations;

                std::string operator_name = name.substr(8);
                bool is_rhs = false;
                if (operator_name.ends_with("RHS"))
                {
                    operator_name = operator_name.substr(0, operator_name.size() - 3);
                    allowed_operations.insert(call_parameter_information{{int_type, int_type}});
                    is_rhs = true;
                }

                if (assignment_operators.contains(operator_name) && is_rhs)
                {
                    // Cannot assign from RHS for now
                    co_return std::nullopt;
                }

                if (assignment_operators.contains(operator_name))
                {
                    allowed_operations.insert(call_parameter_information{{make_oref(int_type), int_type}});
                    co_return allowed_operations;
                }

                allowed_operations.insert(call_parameter_information{{int_type, int_type}});
                // allowed_operations.push_back(call_parameter_information{{int_type, numeric_literal_reference{}}});
                co_return allowed_operations;
            }
            else if (typeis< numeric_literal_reference >(parent) && name.starts_with("OPERATOR"))
            {
                std::set< call_parameter_information > allowed_operations;

                allowed_operations.insert(call_parameter_information{{numeric_literal_reference{}, numeric_literal_reference{}}});
                // TODO: MAYBE: Allow composing any integer operation?
                // allowed_operations.push_back(call_parameter_information{{numeric_literal_reference{}, }});
                co_return allowed_operations;
            }
            else if (name == "CONSTRUCTOR")
            {
                constructor = true;
                // TODO: Allow disabling default constructor
                defaulted = true;

                if (typeis< primitive_type_integer_reference >(parent))
                {
                    auto int_type = as< primitive_type_integer_reference >(parent);

                    std::set< call_parameter_information > result;
                    result.insert({{make_mref(parent), parent}});
                    result.insert({{make_mref(parent)}});
                    co_return result;
                }
            }
            else if (name == "DESTRUCTOR")
            {
                // TODO: Allow disabling default constructor
                defaulted = true;
            }
        }

        bool exists = co_await g->lk_entity_canonical_chain_exists(functum);
        std::set< call_parameter_information > result;
        co_return result;
    }

    // template auto list_builtin_functum_overloads_question_f< compiler >(compiler* g, qualified_symbol_reference functum) -> rpnx::resolver_coroutine< compiler, std::optional< std::set< call_parameter_information > > >;
} // namespace rylang