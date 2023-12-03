//
// Created by Ryan Nicholl on 11/18/23.
//
#include "rylang/compiler.hpp"

#include "rylang/res/list_user_functum_overloads_resolver.hpp"

#include "rylang/operators.hpp"

rpnx::resolver_coroutine< rylang::compiler, std::set< rylang::call_parameter_information > > rylang::list_user_functum_overloads_resolver::co_process(compiler* c, qualified_symbol_reference functum)
{

    auto exists = co_await *c->lk_entity_canonical_chain_exists(functum);

    if (!exists)
    {
        co_return {};
    }

    std::set< call_parameter_information > result;

    auto functum_ast = co_await *c->lk_entity_ast_from_canonical_chain(functum);

    if (!std::holds_alternative< functum_entity_ast >(functum_ast.m_specialization.get()))
    {
        // TODO: Redirect to constructor?
        co_return {};
    }

    functum_entity_ast const& functum_e = std::get< functum_entity_ast >(functum_ast.m_specialization.get());

    for (function_ast const& f : functum_e.m_function_overloads)
    {
        call_parameter_information info;

        std::optional<qualified_symbol_reference > this_arg_type;



        if (f.this_type.has_value())
        {
            // Member function

            // TODO: Make sure this points to a class?
            auto parent = qualified_parent(functum);
            assert(parent.has_value());

            if (f.this_type.value().type() == boost::typeindex::type_id< context_reference >())
            {
                this_arg_type = make_mref(parent.value());
            }
            else
            {
                this_arg_type = f.this_type.value();
            }


            info.argument_types.push_back(*this_arg_type);


        }
        for (auto const& arg : f.args)
        {
            contextual_type_reference ctx_type;
            ctx_type.context = functum;
            ctx_type.type = arg.type;
            auto type = co_await *c->lk_canonical_type_from_contextual_type(ctx_type);
            info.argument_types.push_back(type);
        }
        result.insert(info);
    }

    co_return result;
}
