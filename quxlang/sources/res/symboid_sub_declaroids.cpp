// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
// This file implements the symboid_sub_declaroids resolver.

#include <quxlang/compiler.hpp>
#include <quxlang/res/symboid_subdeclaroids.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(symboid_subdeclaroids)
{
    auto sym = co_await QUX_CO_DEP(symboid, (input_val));

    if (typeis< ast2_class_declaration >(sym))
    {
        co_return as< ast2_class_declaration >(sym).declarations;
    }
    else if (typeis< ast2_module_declaration >(sym))
    {
        co_return as< ast2_module_declaration >(sym).declarations;
    }
    else if (typeis< ast2_template_declaration >(sym))
    {
        // Templates don't have subdeclaroids, only a template instanciation could,
        // but that would produce a class, not a template.
        // e.g. ::foo#(I32) would produce a class, not a template.
        // whereas ::foo doesn't have any subdeclaroids, even if ::foo#(I32) does.
        co_return {};
    }
    else if (typeis<functum>(sym))
    {
        co_return {};
    }
    else
    {
       co_return {};
    }
}