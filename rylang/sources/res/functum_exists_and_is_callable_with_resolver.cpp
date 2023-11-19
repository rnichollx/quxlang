//
// Created by Ryan Nicholl on 11/16/23.
//
#include "rylang/res/functum_exists_and_is_callable_with_resolver.hpp"
#include "rylang/compiler.hpp"

void rylang::functum_exists_and_is_callable_with_resolver::process(compiler* c)
{
    // TODO: implement this case later
    assert(!typeis< functanoid_reference >(func));

    if (typeis< subdotentity_reference >(func))
    {
        auto subdot = as< subdotentity_reference >(func);

        if (typeis< primitive_type_integer_reference >(subdot.parent))
        {
            primitive_type_integer_reference int_type = as< primitive_type_integer_reference >(subdot.parent);
            std::string operator_keyword = "OPERATOR";
            std::string rhs_keyword = "RHS";
            if (subdot.subdotentity_name.starts_with(operator_keyword))
            {
                if (args.argument_types.size() != 2)
                {
                    set_value(false);
                    return;
                }

                bool is_rhs = false;
                std::string which_operator = subdot.subdotentity_name.substr(operator_keyword.size());
                if (which_operator.ends_with(rhs_keyword))
                {
                    which_operator = which_operator.substr(0, which_operator.size() - rhs_keyword.size());
                    is_rhs = true;
                }

                // TODO: Check if the operator is valid for integer types.
                auto lhs_convertible_to_int_dp = get_dependency(
                    [&]
                    {
                        return c->lk_canonical_type_is_implicitly_convertible_to(args.argument_types.at(0), int_type);
                    });

                auto rhs_convertible_to_int_dp = get_dependency(
                    [&]
                    {
                        return c->lk_canonical_type_is_implicitly_convertible_to(args.argument_types.at(1), int_type);
                    });

                if (!ready())
                {
                    return;
                }

                bool lhs_convertible_to_int = lhs_convertible_to_int_dp->get();
                bool rhs_convertible_to_int = rhs_convertible_to_int_dp->get();

                set_value(lhs_convertible_to_int && rhs_convertible_to_int);
            }
            else if (subdot.subdotentity_name == "CONSTRUCTOR")
            {
                if (args.argument_types.size() == 1)
                {
                    set_value(true);
                }

                set_value(false);
            }
        }

        auto func_entityt_exists_dp = get_dependency(
            [&]
            {
                return c->lk_entity_canonical_chain_exists(func);
            });

        if (!ready())
        {
            return;
        }

        bool func_exists = func_entityt_exists_dp->get();

        if (!func_exists)
        {
            set_value(false);
            return;
        }

        // TODO: Check if it is a function here

        auto func_callable_with_dp = get_dependency(
            [&]
            {
                return c->lk_overload_set_is_callable_with(std::make_pair(args, call_args));
            });

        if (!ready())
        {
            return;
        }

    }
}
