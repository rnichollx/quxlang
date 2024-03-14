//
// Created by Ryan Nicholl on 12/3/23.
//

#include "quxlang/res/call_params_of_function_ast_resolver.hpp"
rpnx::resolver_coroutine< quxlang::compiler, quxlang::call_parameter_information > quxlang::call_params_of_function_ast_resolver::co_process(compiler* c, input_type input)
{
    ast2_function_declaration const & f = input.first;
    type_symbol const & functum = input.second;

    // Note: the input to this resolver must be a functum, not the function instanciation.
    // TODO: check that the input is a functum and not a function instanciation

    call_parameter_information info;

    std::optional< type_symbol > this_arg_type;



    if (f.this_type.has_value() || typeis<subdotentity_reference>(functum))
    {
        // Member function

        // TODO: Make sure this points to a class?
        auto parent = qualified_parent(functum);
        assert(parent.has_value());

        if (!f.this_type.has_value() || f.this_type.value().type() == boost::typeindex::type_id< context_reference >())
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
        auto type = co_await *c->lk_canonical_symbol_from_contextual_symbol(ctx_type);
        info.argument_types.push_back(type);
    }

    co_return info;
}