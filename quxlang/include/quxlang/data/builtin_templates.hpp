// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_BUILTIN_TEMPLATES_HEADER_GUARD
#define QUXLANG_DATA_BUILTIN_TEMPLATES_HEADER_GUARD

#include <optional>

#include <rpnx/macros.hpp>

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    struct builtin_template_info
    {
        declared_parameters template_args;
        std::optional< std::int64_t > priority;

        RPNX_MEMBER_METADATA(builtin_template_info, template_args, priority)
    };
} // namespace quxlang

#endif // QUXLANG_DATA_BUILTIN_TEMPLATES_HEADER_GUARD
