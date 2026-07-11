// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_CLASS_TYPE_HEADER_GUARD
#define QUXLANG_DATA_CLASS_TYPE_HEADER_GUARD

#include <cstdint>

#include <rpnx/macros.hpp>

/** Identifies the concrete representation family of a constructible class type. */
RPNX_ENUM(quxlang, class_kind, std::uint8_t,
    noexist,
    primitive,
    enum_,
    flagset,
    struct_
)

#endif // QUXLANG_DATA_CLASS_TYPE_HEADER_GUARD
