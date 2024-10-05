// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#include "quxlang/compiler.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(module_sources)
{
    QUX_CO_GETDEP(src, module_source_name, (input_val));
    auto it = c->m_source_code->module_sources.find(src);

    if (it == c->m_source_code->module_sources.end())
    {
        throw std::logic_error("Module not found");
    }

    QUX_CO_ANSWER(it->second);
}
