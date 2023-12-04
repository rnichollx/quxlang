//
// Created by Ryan Nicholl on 12/3/23.
//

#include "rylang/res/call_params_of_function_ast_resolver.hpp"
rpnx::resolver_coroutine< rylang::compiler, rylang::call_parameter_information > rylang::call_params_of_function_ast_resolver::co_process(compiler* c, input_type input)
{
    function_ast f = input.first;
    qualified_symbol_reference functum = input.second;

    call_parameter_information info;

    std::optional< qualified_symbol_reference > this_arg_type;

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

    co_return info;
}