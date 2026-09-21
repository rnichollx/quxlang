// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/struct_tags_spec.hpp>
#include <quxlang/data/basic_types.hpp>
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/unimplemented.hpp"


rpnx::querygraph::coroutine< quxlang::struct_tags_spec > quxlang::struct_tags_impl(type_symbol input)
{
    ast2_symboid the_struct = co_await rpnx::querygraph::request< symboid_query >(input);

    if (!typeis< ast2_struct_declaration >(the_struct))
    {
       co_return {};
    }
    ast2_struct_declaration const& struct_obj = as< ast2_struct_declaration >(the_struct);

    struct_tags_result_type tags = struct_obj.struct_keywords;
    for (std::pair< std::string const, expression > const& property : struct_obj.conditional_struct_keywords)
    {
        if (co_await rpnx::querygraph::request< constexpr_bool_query >(constexpr_input{.expr = property.second, .context = input}))
        {
            tags.insert(property.first);
        }
    }
    co_return tags;
}
