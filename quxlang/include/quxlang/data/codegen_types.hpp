// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_CODEGEN_TYPES_HEADER_GUARD
#define QUXLANG_DATA_CODEGEN_TYPES_HEADER_GUARD

#include "quxlang/data/constexpr.hpp"
#include <quxlang/data/basic_types.hpp>
#include "quxlang/macros.hpp"
#include "quxlang/vmir2/vmir2.hpp"
#include "rpnx/uint64_base.hpp"

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace quxlang
{
    RPNX_UNIQUE_U64(value_index);

    struct codegen_invocation_args
    {
        std::map< std::string, value_index > named;
        std::vector< value_index > positional;

        auto size() const -> std::size_t
        {
            return positional.size() + named.size();
        }

        RPNX_MEMBER_METADATA(codegen_invocation_args, named, positional);
    };

} // namespace quxlang

#endif // QUXLANG_DATA_CODEGEN_TYPES_HEADER_GUARD
