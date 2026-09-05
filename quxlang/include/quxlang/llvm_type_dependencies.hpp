// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_LLVM_TYPE_DEPENDENCIES_HEADER_GUARD
#define QUXLANG_LLVM_TYPE_DEPENDENCIES_HEADER_GUARD
#include <quxlang/data/basic_types.hpp>
#include <type_traits>

namespace quxlang::llvm_backend
{
    /** Visits directly contained native types and reports whether the containing type needs a storage placement. */
    template < typename Visitor >
    auto visit_storage_type_dependencies(type_symbol const& type, Visitor&& visit) -> bool
    {
        bool has_placement = true;
        rpnx::apply_visitor< void >(type,
                                    [&]< typename Type >(Type const& value)
                                    {
                                        if constexpr (std::is_same_v< Type, nvalue_slot > || std::is_same_v< Type, dvalue_slot >)
                                        {
                                            visit(value.target);
                                            has_placement = false;
                                        }
                                        else if constexpr (std::is_same_v< Type, attached_type_reference >)
                                        {
                                            if (!value.carrying_type.template type_is< void_type >())
                                            {
                                                visit(value.carrying_type);
                                            }
                                        }
                                        else if constexpr (std::is_same_v< Type, ptrref_type >)
                                        {
                                            visit(value.target);
                                        }
                                        else if constexpr (std::is_same_v< Type, array_type > || std::is_same_v< Type, array_initializer_type >)
                                        {
                                            visit(value.element_type);
                                            has_placement = !std::is_same_v< Type, array_initializer_type >;
                                        }
                                        else if constexpr (std::is_same_v< Type, procedure_type >)
                                        {
                                            for (type_symbol const& parameter : value.signature.params.positional)
                                            {
                                                visit(parameter);
                                            }
                                            for (std::pair< std::string const, type_symbol > const& parameter : value.signature.params.named)
                                            {
                                                visit(parameter.second);
                                            }
                                            if (value.signature.return_type.has_value())
                                            {
                                                visit(*value.signature.return_type);
                                            }
                                            has_placement = false;
                                        }
                                        else if constexpr (std::is_same_v< Type, storage >)
                                        {
                                            for (type_symbol const& alternative : value.storable_types)
                                            {
                                                visit(alternative);
                                            }
                                        }
                                        else if constexpr (std::is_same_v< Type, virtual_storage >)
                                        {
                                            has_placement = false;
                                        }
                                    });
        if (std::optional< type_symbol > atomic_value = atomic_type_argument(type); atomic_value.has_value())
        {
            visit(*atomic_value);
        }
        return has_placement;
    }
} // namespace quxlang::llvm_backend
#endif
