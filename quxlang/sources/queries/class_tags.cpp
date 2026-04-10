// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/class_tags_spec.hpp>
#include "quxlang/data/expression.hpp"
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/unimplemented.hpp"


rpnx::querygraph::coroutine< quxlang::class_tags_spec > quxlang::class_tags_impl(type_symbol input)
{
    std::string name = quxlang::to_string(input);
    std::set< std::string > tags;

    ast2_symboid the_class = co_await rpnx::querygraph::request< symboid_query >(input);

    if (!typeis< ast2_class_declaration >(the_class))
    {
       co_return {};
    }
    ast2_class_declaration const& class_obj = as< ast2_class_declaration >(the_class);

    co_return class_obj.class_keywords;
}
