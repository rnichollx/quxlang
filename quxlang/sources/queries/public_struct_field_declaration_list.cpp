// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/data/lambda_types.hpp>
#include <quxlang/queries/specs/public_struct_field_declaration_list_spec.hpp>

rpnx::querygraph::coroutine< quxlang::public_struct_field_declaration_list_spec > quxlang::public_struct_field_declaration_list_impl(type_symbol input)
{
    if ((!input.type_is< subsymbol >() && !input.type_is< submember >() && !input.type_is< instanciation_reference >()) || parse_lambda_closure_symbol(input).has_value())
    {
        throw semantic_compilation_error("Public field reflection requires a named STRUCT or IBC_STRUCT type");
    }
    ast2_symboid const& declaration = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!declaration.type_is< ast2_struct_declaration >())
    {
        throw semantic_compilation_error("Public field reflection requires a named STRUCT or IBC_STRUCT type");
    }

    std::vector< struct_field_declaration > const& fields = co_await rpnx::querygraph::request< struct_field_declaration_list_query >(input);
    std::vector< struct_field_declaration > output;
    output.reserve(fields.size());
    for (struct_field_declaration const& field : fields)
    {
        std::optional< resolved_privacy_scope > privacy = co_await rpnx::querygraph::request< declaration_privacy_query >(submember{.of = input, .name = field.name});
        if (!privacy.has_value())
        {
            output.push_back(field);
        }
    }
    co_return output;
}
