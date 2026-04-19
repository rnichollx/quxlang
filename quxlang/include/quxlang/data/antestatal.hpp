// Copyright (c) 2026 Ryan P. Nicholl <rnicholl@protonmail.com>
/** @file antestatal.hpp
 *  @brief This file defines data types for interacting with antestatal values and antestatal types.
 *  Antestatal types are those which can potentially be constructed by the compiler at compile time
 *  and bound as static globals which can participate in constexpr evaluation without a serialization
 *  based initializer.
 *  Antestatal values are those which satisfy the constraints needed to build such values. Mainly,
 *  in order for pointers to antestatally valid, they must point to objects which are either
 *  themselves antestatal statics or subojbects thereof. It is therefore not allowed, for example
 *  for an antestatal pointer to point to a memory allocation.
 */

#ifndef QUXLANG_ANTESTATAL_HPP
#define QUXLANG_ANTESTATAL_HPP

#include <quxlang/data/basic_types.hpp>

#include <rpnx/macros.hpp>
#include <rpnx/variant.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace quxlang
{
    struct antestatal_struct;
    struct antestatal_array;
    struct antestatal_ptrref;
    struct antestatal_primitive;

    using antestatal_value = rpnx::variant< antestatal_primitive, antestatal_array, antestatal_ptrref, antestatal_struct >;

    struct antestatal_access_global;
    struct antestatal_access_field;
    struct antestatal_access_array_element;
    struct antestatal_nullptr;

    using antestatal_access = rpnx::variant< antestatal_nullptr, antestatal_access_global, antestatal_access_array_element, antestatal_access_field >;

    struct antestatal_nullptr
    {
        RPNX_EMPTY_METADATA(antestatal_nullptr);
    };

    struct antestatal_access_global
    {
        type_symbol symbol;

        RPNX_MEMBER_METADATA(antestatal_access_global, symbol);
    };

    struct antestatal_access_field
    {
        antestatal_access object;
        std::string field_name;

        RPNX_MEMBER_METADATA(antestatal_access_field, object, field_name);
    };

    struct antestatal_access_array_element
    {
        antestatal_access array;
        std::uint64_t index = 0;

        RPNX_MEMBER_METADATA(antestatal_access_array_element, array, index);
    };

    struct antestatal_ptrref
    {
        antestatal_access target;

        RPNX_MEMBER_METADATA(antestatal_ptrref, target);
    };

    struct antestatal_primitive
    {
        std::vector< std::byte > value;

        RPNX_MEMBER_METADATA(antestatal_primitive, value);
    };

    struct antestatal_struct
    {
        std::map< std::string, antestatal_value > fields;

        RPNX_MEMBER_METADATA(antestatal_struct, fields);
    };

    struct antestatal_array
    {
        std::vector< antestatal_value > elements;

        RPNX_MEMBER_METADATA(antestatal_array, elements);
    };

} // namespace quxlang

#endif // QUXLANG_ANTESTATAL_HPP
