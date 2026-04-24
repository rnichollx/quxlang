// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/templex_builtins_spec.hpp>

using namespace quxlang;

rpnx::querygraph::coroutine< quxlang::templex_builtins_spec > quxlang::templex_builtins_impl(type_symbol input)
{
    std::vector< builtin_template_info > results;

    if (!typeis< builtin_symbol >(input))
    {
        co_return results;
    }

    auto const& builtin = as< builtin_symbol >(input);
    if (!is_builtin_allocator_name(builtin.name))
    {
        co_return results;
    }

    builtin_template_info info;
    info.template_args.named["T"] = declared_parameter{
        .name = "T",
        .kind = template_parameter_kind::type,
        .type = type_temploidic{},
    };
    results.push_back(std::move(info));

    co_return results;
}
