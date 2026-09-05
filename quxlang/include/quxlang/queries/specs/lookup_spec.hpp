// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LOOKUP_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LOOKUP_SPEC_HEADER_GUARD

#include <quxlang/queries/constexpr_u64.hpp>
#include <quxlang/queries/constexpr_eval_v3.hpp>
#include <quxlang/queries/declaration_is_accessible.hpp>
#include <quxlang/queries/exists.hpp>
#include <quxlang/queries/function_declaration.hpp>
#include <quxlang/queries/function_pack_info.hpp>
#include <quxlang/queries/instanciation.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/module_ast.hpp>
#include <quxlang/queries/subtag_binding.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct lookup_spec
    {
        using query = lookup_query;
        using dependencies = rpnx::typelist< constexpr_eval_v3_query, constexpr_u64_query, declaration_is_accessible_query, exists_query, function_declaration_query, function_pack_info_query, instanciation_query, lookup_query, machine_info_query, module_ast_query, subtag_binding_query, symboid_query, symbol_type_query >;
    };

    /** @brief Resolves a type reference using exhaustive variant dispatch. */
    rpnx::querygraph::coroutine< lookup_spec > lookup_impl(contextual_type_reference input);

    namespace detail
    {
        /** @brief Evaluates an unsigned integer expression used in a type. */
        auto evaluate_u64_type_expression(type_symbol context, expression expr) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::uint64_t >;

        /** @brief Checks whether a selected declaration is accessible from the lookup context. */
        auto require_accessible_declaration(contextual_type_reference const& input, type_symbol const& selected_declaration) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< void >;

        /** @brief Expands an alias using its lexical scope and instantiated template bindings. */
        auto expand_alias(contextual_type_reference const& input, type_symbol const& selected_declaration) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves the void type. */
        auto lookup_impl_overloads(contextual_type_reference const&, void_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves the null type. */
        auto lookup_impl_overloads(contextual_type_reference const&, null_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves the byte type. */
        auto lookup_impl_overloads(contextual_type_reference const&, byte_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves the initialization guard type. */
        auto lookup_impl_overloads(contextual_type_reference const&, initguard_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves the initialization guard lock type. */
        auto lookup_impl_overloads(contextual_type_reference const&, initguard_lock_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves a constexpr proxy. */
        auto lookup_impl_overloads(contextual_type_reference const&, constexpr_proxy const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Resolves an unqualified name through lexical scopes, bindings, imports, and builtins. */
        auto lookup_impl_overloads(contextual_type_reference const& input, freebound_identifier const& fb) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves a builtin symbol. */
        auto lookup_impl_overloads(contextual_type_reference const&, builtin_symbol const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Rejects a standalone context marker, which requires a qualified selection. */
        auto lookup_impl_overloads(contextual_type_reference const& input, context_reference const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves an AUTO deduction pattern. */
        auto lookup_impl_overloads(contextual_type_reference const&, auto_temploidic const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves a DECAY deduction pattern. */
        auto lookup_impl_overloads(contextual_type_reference const&, decay_temploidic const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves a TYPE deduction pattern. */
        auto lookup_impl_overloads(contextual_type_reference const&, type_temploidic const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves an absolute module reference. */
        auto lookup_impl_overloads(contextual_type_reference const&, absolute_module_reference const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Resolves a qualified declaration with access checks and alias expansion. */
        auto lookup_impl_overloads(contextual_type_reference const& input, subsymbol const& sub) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Resolves a named template binding in its parent context. */
        auto lookup_impl_overloads(contextual_type_reference const& input, subtag_type const& sub) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves an integer type. */
        auto lookup_impl_overloads(contextual_type_reference const&, int_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves a floating-point type. */
        auto lookup_impl_overloads(contextual_type_reference const&, float_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves the boolean type. */
        auto lookup_impl_overloads(contextual_type_reference const&, bool_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Resolves an initializee and instantiates template selections. */
        auto lookup_impl_overloads(contextual_type_reference const& input, initialization_reference const& param_set) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves a resolved instantiation while expanding aliases. */
        auto lookup_impl_overloads(contextual_type_reference const& input, instanciation_reference const& inst) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Resolves a temploid selection and checks its accessibility. */
        auto lookup_impl_overloads(contextual_type_reference const& input, temploid_reference const& selection) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Rejects value expressions that cannot be resolved by type lookup. */
        auto lookup_impl_overloads(contextual_type_reference const& input, value_expression_reference const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Resolves a member declaration and checks its accessibility. */
        auto lookup_impl_overloads(contextual_type_reference const& input, submember const& sub) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves the current-type marker. */
        auto lookup_impl_overloads(contextual_type_reference const&, thistype const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Resolves procedure parameter and return types. */
        auto lookup_impl_overloads(contextual_type_reference const& input, procedure_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Resolves the target of a pointer or reference while preserving its qualifiers. */
        auto lookup_impl_overloads(contextual_type_reference const& input, ptrref_type const& ptr) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Resolves the carrying type and attached symbol. */
        auto lookup_impl_overloads(contextual_type_reference const& input, attached_type_reference const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves a numeric literal type. */
        auto lookup_impl_overloads(contextual_type_reference const&, numeric_literal_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Rejects numeric literal deduction patterns in type lookup. */
        auto lookup_impl_overloads(contextual_type_reference const& input, numeric_literal_any_temploidic const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves a string literal type. */
        auto lookup_impl_overloads(contextual_type_reference const&, string_literal_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Rejects string literal deduction patterns in type lookup. */
        auto lookup_impl_overloads(contextual_type_reference const& input, string_literal_any_temploidic const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Resolves the target type of an nvalue slot. */
        auto lookup_impl_overloads(contextual_type_reference const& input, nvalue_slot const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Resolves the target type of a dvalue slot. */
        auto lookup_impl_overloads(contextual_type_reference const& input, dvalue_slot const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Evaluates an array count and resolves its element type. */
        auto lookup_impl_overloads(contextual_type_reference const& input, array_type const& arry) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Resolves SIZE to the target unsigned pointer-sized integer type. */
        auto lookup_impl_overloads(contextual_type_reference const&, size_type const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves the address type. */
        auto lookup_impl_overloads(contextual_type_reference const&, address_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves the type index type. */
        auto lookup_impl_overloads(contextual_type_reference const&, type_index_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves a readonly constant. */
        auto lookup_impl_overloads(contextual_type_reference const&, readonly_constant const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Resolves the set of types that storage can contain. */
        auto lookup_impl_overloads(contextual_type_reference const& input, storage const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Evaluates storage size and alignment for a target with a concrete layout. */
        auto lookup_impl_overloads(contextual_type_reference const& input, aligned_storage const& storage_type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Preserves virtual storage. */
        auto lookup_impl_overloads(contextual_type_reference const&, virtual_storage const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Rejects array initializer types in type lookup. */
        auto lookup_impl_overloads(contextual_type_reference const& input, array_initializer_type const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Rejects static local value references in type lookup. */
        auto lookup_impl_overloads(contextual_type_reference const& input, static_local_ref const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Rejects static snapshot value references in type lookup. */
        auto lookup_impl_overloads(contextual_type_reference const& input, static_snapshot_ref const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Resolves the declared field types of an anonymous structural record. */
        auto lookup_impl_overloads(contextual_type_reference const&, composite_type const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Selects a composite field type by compile-time name or ordinal. */
        auto lookup_impl_overloads(contextual_type_reference const&, composite_field_type_ref const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Selects the type of an instantiated positional pack element. */
        auto lookup_impl_overloads(contextual_type_reference const& input, pack_arg_type_ref const& ref) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Resolves the declared type of a parameter or value symbol. */
        auto lookup_impl_overloads(contextual_type_reference const& input, decltype_type_ref const& ref) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;

        /** @brief Rejects TYPEOF outside function generation. */
        auto lookup_impl_overloads(contextual_type_reference const&, typeof_type_ref const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >;
    } // namespace detail
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LOOKUP_SPEC_HEADER_GUARD
