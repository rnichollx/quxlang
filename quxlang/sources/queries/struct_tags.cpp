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

    co_return struct_obj.struct_keywords;
}
