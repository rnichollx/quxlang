// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/constexpr_eval_antestatal_spec.hpp>
#include <quxlang/queries/specs/constexpr_eval_v3_spec.hpp>

#include "quxlang/bytemath.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"
#include "quxlang/vmir2/source_index.hpp"

#include <exception>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "query_helpers.hpp"

namespace quxlang::detail
{
    struct constexpr_eval_antestatal_helpers
    {
        /// Returns true when the type symbol represents the compiler's readonly STRING_CONSTANT type.
        static auto is_string_constant_type(type_symbol const& type) -> bool
        {
            return typeis< readonly_constant >(type) && as< readonly_constant >(type).kind == constant_kind::string;
        }

        /// Decodes a STRINGLIKE serialization payload as UINTANY length followed by exactly that many string bytes.
        static auto decode_uintany_string(std::vector< std::byte > const& encoded) -> constexpr_string
        {
        std::uint64_t length = 0;
        std::uint64_t shift = 0;
        std::size_t pos = 0;
        while (pos < encoded.size())
        {
            auto byte_value = static_cast< std::uint64_t >(std::to_integer< std::uint8_t >(encoded.at(pos)));
            pos++;

            if (shift >= 64 || ((byte_value & 127U) << shift) >> shift != (byte_value & 127U))
            {
                throw quxlang::semantic_compilation_error("STRINGLIKE UINTANY length is too large");
            }
            length += (byte_value & 127U) << shift;

            if ((byte_value & 128U) == 0)
            {
                if (encoded.size() - pos != length)
                {
                    throw quxlang::semantic_compilation_error("STRINGLIKE serialized string length does not match byte count");
                }

                quxlang::constexpr_string result;
                result.bytes.insert(result.bytes.end(), encoded.begin() + static_cast< std::ptrdiff_t >(pos), encoded.end());
                return result;
            }

            shift += 7;
            if (shift >= 64)
            {
                throw quxlang::semantic_compilation_error("STRINGLIKE UINTANY length is too large");
            }
            length += std::uint64_t{1} << shift;
        }

        throw quxlang::semantic_compilation_error("STRINGLIKE serialized string is missing its UINTANY length terminator");
        }

        /// Converts legacy constexpr input into the versioned constexpr v3 input shape.
        static auto make_v3_input(constexpr_input2 input) -> constexpr_input_v3
        {
        constexpr_input_v3 result;
        result.expr = std::move(input.expr);
        result.context = std::move(input.context);
        result.expected_result_type = input.require_antestatal_result ? std::optional< quxlang::type_symbol >(std::move(input.type)) : std::nullopt;
        result.antestatal_global_symbol = std::move(input.antestatal_global_symbol);
        for (auto& [name, def] : input.scoped_definitions)
        {
            if (def.template type_is< quxlang::type_symbol >())
            {
                result.scoped_definitions[std::move(name)] = quxlang::scoped_typedef{.type = std::move(def.template get_as< quxlang::type_symbol >())};
                continue;
            }
            throw rpnx::unimplemented();
        }
        for (auto& [name, symbol] : input.scoped_static_symbols)
        {
            result.scoped_definitions[std::move(name)] = quxlang::scoped_static{.symbol = std::move(symbol)};
        }
        result.statics = std::move(input.static_inputs);
        if (!input.emit_static_results)
        {
            for (auto& [_, binding] : result.statics)
            {
                binding.mutation_result_id.reset();
            }
        }
            return result;
        }

        /// Adds nested value types that may require struct layout during antestatal materialization.
        static auto add_type_for_layout_scan(std::vector< type_symbol >& pending, type_symbol type) -> void
        {
        if (type.type_is< quxlang::nvalue_slot >())
        {
            pending.push_back(type.get_as< quxlang::nvalue_slot >().target);
            return;
        }
        if (type.type_is< quxlang::dvalue_slot >())
        {
            pending.push_back(type.get_as< quxlang::dvalue_slot >().target);
            return;
        }
        if (type.type_is< quxlang::ptrref_type >())
        {
            pending.push_back(type.get_as< quxlang::ptrref_type >().target);
            return;
        }
        if (type.type_is< quxlang::array_type >())
        {
            pending.push_back(type.get_as< quxlang::array_type >().element_type);
            return;
        }
        if (type.type_is< quxlang::storage >())
        {
            for (auto const& storable_type : type.get_as< quxlang::storage >().storable_types)
            {
                pending.push_back(storable_type);
            }
            return;
        }
        }

        /// Returns true when a type symbol may need a struct_layout query before interpretation.
        static auto type_might_have_layout(type_symbol const& type) -> bool
        {
            return type.type_is< subsymbol >() || type.type_is< subtag_type >() || type.type_is< instanciation_reference >() || type.type_is< readonly_constant >() ||
                (type.type_is< builtin_symbol >() && is_builtin_enum_name(type.get_as< builtin_symbol >().name));
        }
    };
} // namespace quxlang::detail

/// Evaluates a constexpr v3 expression and returns every requested result ID.
rpnx::querygraph::coroutine< quxlang::constexpr_eval_v3_spec > quxlang::constexpr_eval_v3_impl(constexpr_input_v3 input)
{
    vmir2::ir2_constexpr_interpreter interp;
    [[maybe_unused]] std::vector< std::string > debug_lines;
    if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
    {
        interp.set_debug_line_handler([&debug_lines](std::string line)
                                      {
                                          debug_lines.push_back(std::move(line));
                                      });
    }
    auto source_file_index = co_await rpnx::querygraph::request< source_file_index_query >(std::monostate{});
    auto source_bundle = co_await rpnx::querygraph::request< source_bundle_query >(std::monostate{});
    interp.set_source_index(vmir2::source_index(source_file_index, source_bundle));

    interp.set_constexpr_result_global_symbol(input.antestatal_global_symbol);

    std::set< type_symbol > layout_types;
    std::set< type_symbol > loaded_layouts;
    std::vector< instanciation_reference > pending_functanoids;
    std::set< type_symbol > queued_functanoids;
    std::set< type_symbol > loaded_functanoids;
    std::set< type_symbol > asm_procedure_symbols;
    std::vector< type_symbol > pending_antestatal_globals;
    std::set< type_symbol > queued_antestatal_globals;
    std::set< type_symbol > loaded_antestatal_globals;

    auto enqueue_functanoid = [&](type_symbol const& funcname)
    {
        if (!typeis< instanciation_reference >(funcname))
        {
            throw compiler_bug("functanoid dependency is not an instanciation reference: " + to_string(funcname));
        }
        if (loaded_functanoids.contains(funcname) || !queued_functanoids.insert(funcname).second)
        {
            return;
        }
        pending_functanoids.push_back(funcname.get_as< instanciation_reference >());
    };

    auto enqueue_functanoids = [&](std::set< type_symbol > const& functanoids)
    {
        for (type_symbol const& funcname : functanoids)
        {
            enqueue_functanoid(funcname);
        }
    };

    auto enqueue_antestatal_global = [&](type_symbol const& symbol)
    {
        if (input.antestatal_global_symbol.has_value() && *input.antestatal_global_symbol == symbol)
        {
            return;
        }
        if (loaded_antestatal_globals.contains(symbol) || !queued_antestatal_globals.insert(symbol).second)
        {
            return;
        }
        pending_antestatal_globals.push_back(symbol);
    };

    auto enqueue_antestatal_globals = [&](std::set< type_symbol > const& symbols)
    {
        for (type_symbol const& symbol : symbols)
        {
            enqueue_antestatal_global(symbol);
        }
    };

    auto enqueue_layouts = [&](std::set< type_symbol > const& types)
    {
        for (type_symbol const& type : types)
        {
            layout_types.insert(type);
        }
    };

    /// Adds all queued struct layouts needed to materialize or execute constexpr values.
    auto add_queued_layouts = [&]() -> rpnx::querygraph::coroutine< quxlang::constexpr_eval_v3_spec >::cosubroutine< void >
    {
        for (type_symbol const& root_type : layout_types)
        {
            std::vector< type_symbol > pending{root_type};

            while (!pending.empty())
            {
                type_symbol type = std::move(pending.back());
                pending.pop_back();

                detail::constexpr_eval_antestatal_helpers::add_type_for_layout_scan(pending, type);

                if (!detail::constexpr_eval_antestatal_helpers::type_might_have_layout(type) || loaded_layouts.contains(type))
                {
                    continue;
                }

                loaded_layouts.insert(type);
                class_kind const kind = co_await rpnx::querygraph::request< class_type_query >(type);
                if (kind == class_kind::enum_)
                {
                    enum_info const info = co_await rpnx::querygraph::request< enum_info_query >(type);
                    interp.add_enum_info(type, info);
                    continue;
                }
                if (kind == class_kind::flagset)
                {
                    flagset_info const info = co_await rpnx::querygraph::request< flagset_info_query >(type);
                    interp.add_nominal_integer_type(type, info.bits);
                    continue;
                }
                if (kind == class_kind::union_)
                {
                    union_info const info = co_await rpnx::querygraph::request< union_info_query >(type);
                    fusion_layout const layout = co_await rpnx::querygraph::request< fusion_layout_query >(type);
                    interp.add_union_info(type, info);
                    interp.add_fusion_layout(type, layout);
                    for (union_option_info const& option : info.options)
                    {
                        pending.push_back(option.type);
                    }
                    continue;
                }
                if (kind == class_kind::variant)
                {
                    variant_info const info = co_await rpnx::querygraph::request< variant_info_query >(type);
                    fusion_layout const layout = co_await rpnx::querygraph::request< fusion_layout_query >(type);
                    interp.add_variant_info(type, info);
                    interp.add_fusion_layout(type, layout);
                    for (type_symbol const& alternative : info.alternatives)
                    {
                        pending.push_back(alternative);
                    }
                    continue;
                }
                if (kind != class_kind::struct_)
                {
                    continue;
                }
                struct_layout const layout = co_await rpnx::querygraph::request< struct_layout_query >(type);
                interp.add_struct_layout(type, layout);
                for (auto const& field : layout.fields)
                {
                    pending.push_back(field.type);
                }
            }
        }
    };

    /// Provides one querygraph-backed global antestatal static requested by the dependency scanner.
    auto provide_antestatal_global = [&](type_symbol const& symbol) -> rpnx::querygraph::coroutine< quxlang::constexpr_eval_v3_spec >::cosubroutine< std::pair< std::set< type_symbol >, std::set< type_symbol > > >
    {
        if (!(co_await rpnx::querygraph::request< global_is_antestatal_static_query >(symbol)))
        {
            throw quxlang::compiler_bug("constexpr interpreter requested non-antestatal global data: " + quxlang::to_string(symbol));
        }

        type_symbol type = co_await rpnx::querygraph::request< variable_type_query >(symbol);
        layout_types.insert(type);
        antestatal_value value = co_await rpnx::querygraph::request< antestatal_static_value_query >(symbol);
        dependencies const& dependencies = co_await rpnx::querygraph::request< direct_dependencies_query >(
            direct_dependencies_input{.symbol = symbol, .set = dependency_set::constexpr_});
        std::set< type_symbol > value_functanoids;
        for (auto const& [functanoid, _] : dependencies.functanoids) value_functanoids.insert(functanoid);
        std::set< type_symbol > value_antestatal_globals = dependencies.antestatal_globals;
        interp.add_constexpr_antestatal_global(symbol, type, std::move(value), false);
        co_return std::pair(std::move(value_functanoids), std::move(value_antestatal_globals));
    };

    auto add_zero_initialized_global_storages = [&](dependencies const& dependencies) -> rpnx::querygraph::coroutine< quxlang::constexpr_eval_v3_spec >::cosubroutine< void >
    {
        for (type_symbol const& symbol : dependencies.global_roots)
        {
            initialization_type const init_type = co_await rpnx::querygraph::request< global_init_type_query >(symbol);
            if (init_type != initialization_type::init_trivial)
            {
                continue;
            }

            type_symbol type = co_await rpnx::querygraph::request< variable_type_query >(symbol);
            layout_types.insert(type);
            interp.add_zero_initialized_global(symbol, type);
        }
    };

    constexpr_routine_v3_result routine_result = co_await rpnx::querygraph::request< constexpr_routine_v3_query >(input);
    vmir2::functanoid_routine3 const& ir3 = routine_result.routine;
    dependencies const& root_dependencies = routine_result.direct_dependencies;
    if (input.expected_result_type.has_value() && !typeis< auto_temploidic >(*input.expected_result_type))
    {
        layout_types.insert(*input.expected_result_type);
    }
    enqueue_layouts(root_dependencies.struct_layouts);
    enqueue_layouts(root_dependencies.fusion_layouts);
    for (auto const& [functanoid, _] : root_dependencies.functanoids) enqueue_functanoid(functanoid);
    enqueue_antestatal_globals(root_dependencies.antestatal_globals);
    co_await add_zero_initialized_global_storages(root_dependencies);
    for (auto const& [_, localdata] : input.statics)
    {
        layout_types.insert(localdata.type);
    }

    for (auto const& [symbol, localdata] : input.statics)
    {
        if (!typeis< antestatal_value >(localdata.value))
        {
            throw quxlang::semantic_compilation_error("constexpr evaluation of function-local serialoid statics is not implemented yet: " + symbol.name);
        }
        antestatal_value const& antestatal_localdata = constexpr_value_as_antestatal(localdata.value);
        dependencies const& local_dependencies = routine_result.static_dependencies.at(symbol);
        loaded_antestatal_globals.insert(type_symbol(symbol));
        interp.add_constexpr_antestatal_global(type_symbol(symbol), localdata.type, antestatal_localdata, localdata.mutation_result_id.has_value());
        for (auto const& [functanoid, _] : local_dependencies.functanoids) enqueue_functanoid(functanoid);
        enqueue_antestatal_globals(local_dependencies.antestatal_globals);
    }
    interp.add_functanoid3(void_type{}, ir3, root_dependencies.static_snapshots);
    loaded_functanoids.insert(type_symbol(void_type{}));

    while (!pending_functanoids.empty() || !pending_antestatal_globals.empty())
    {
        std::vector< type_symbol > round_antestatal_globals;
        while (!pending_antestatal_globals.empty())
        {
            type_symbol symbol = std::move(pending_antestatal_globals.back());
            pending_antestatal_globals.pop_back();
            if (loaded_antestatal_globals.contains(symbol))
            {
                continue;
            }
            round_antestatal_globals.push_back(std::move(symbol));
        }

        for (type_symbol const& symbol : round_antestatal_globals)
        {
            co_yield rpnx::querygraph::dependency< global_is_antestatal_static_query >(rpnx::querygraph::request< global_is_antestatal_static_query >(symbol));
            co_yield rpnx::querygraph::dependency< variable_type_query >(rpnx::querygraph::request< variable_type_query >(symbol));
            co_yield rpnx::querygraph::dependency< antestatal_static_value_query >(rpnx::querygraph::request< antestatal_static_value_query >(symbol));
        }

        std::vector< instanciation_reference > round_functanoids;
        while (!pending_functanoids.empty())
        {
            instanciation_reference functanoid = std::move(pending_functanoids.back());
            pending_functanoids.pop_back();

            type_symbol funcname = functanoid;
            if (loaded_functanoids.contains(funcname))
            {
                continue;
            }
            round_functanoids.push_back(std::move(functanoid));
        }

        std::vector< instanciation_reference > round_functions;
        for (instanciation_reference const& invocable : round_functanoids)
        {
            type_symbol const symbol = invocable;
            ast2_symboid const& symboid = co_await rpnx::querygraph::request< symboid_query >(invocable.temploid.templexoid);
            if (typeis< ast2_asm_procedure_declaration >(symboid) || typeis< ast2_extern_procedure >(symboid))
            {
                if (asm_procedure_symbols.insert(symbol).second)
                {
                    interp.add_constexpr_asm_procedure(symbol);
                }
                loaded_functanoids.insert(symbol);
                continue;
            }

            round_functions.push_back(invocable);
        }

        for (instanciation_reference const& functanoid : round_functions)
        {
            co_yield rpnx::querygraph::dependency< vm_procedure3_query >(rpnx::querygraph::request< vm_procedure3_query >(functanoid));
        }

        for (type_symbol const& symbol : round_antestatal_globals)
        {
            if (loaded_antestatal_globals.contains(symbol))
            {
                continue;
            }
            std::pair< std::set< type_symbol >, std::set< type_symbol > > requirements = co_await provide_antestatal_global(symbol);
            loaded_antestatal_globals.insert(symbol);
            enqueue_functanoids(requirements.first);
            enqueue_antestatal_globals(requirements.second);
        }

        for (instanciation_reference const& functanoid : round_functions)
        {
            type_symbol funcname = functanoid;
            if (loaded_functanoids.contains(funcname))
            {
                continue;
            }

            vmir2::functanoid_routine3 const& ir2_other = co_await rpnx::querygraph::request< vm_procedure3_query >(functanoid);
            dependencies const& dependencies = co_await rpnx::querygraph::request< direct_dependencies_query >(
                direct_dependencies_input{.symbol = funcname, .set = dependency_set::constexpr_});
            interp.add_functanoid3(funcname, ir2_other, dependencies.static_snapshots);
            loaded_functanoids.insert(funcname);
            enqueue_layouts(dependencies.struct_layouts);
            enqueue_layouts(dependencies.fusion_layouts);
            enqueue_antestatal_globals(dependencies.antestatal_globals);
            co_await add_zero_initialized_global_storages(dependencies);
            for (auto const& [dependency, _] : dependencies.functanoids) enqueue_functanoid(dependency);
        }
    }

    co_await add_queued_layouts();

    std::exception_ptr execution_exception;
    try
    {
        interp.exec3(void_type{});
    }
    catch (...)
    {
        execution_exception = std::current_exception();
    }
    if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
    {
        for (std::string const& debug_line : debug_lines)
        {
            co_yield rpnx::querygraph::debug_message("{}", debug_line);
        }
    }
    if (execution_exception)
    {
        std::rethrow_exception(execution_exception);
    }

    constexpr_result_v3 result;
    for (auto& [id, value] : interp.get_cr_antestatal_values())
    {
        result.values[id] = constexpr_value(std::move(value));
    }
    bool const expects_string_result = input.expected_result_type.has_value() && detail::constexpr_eval_antestatal_helpers::is_string_constant_type(*input.expected_result_type);
    bool const expects_numeric_result = input.expected_result_type.has_value() &&
        quxlang::typeis< quxlang::readonly_constant >(*input.expected_result_type) &&
        quxlang::as< quxlang::readonly_constant >(*input.expected_result_type).kind == quxlang::constant_kind::numeric;
    for (auto& [id, value] : interp.get_cr_serialoid_values())
    {
        if (expects_string_result && id == constexpr_primary_result_id)
        {
            result.values[id] = constexpr_value(detail::constexpr_eval_antestatal_helpers::decode_uintany_string(value.bytes));
        }
        else if (expects_numeric_result && id == constexpr_primary_result_id)
        {
            quxlang::constexpr_numeric numeric_result;
            numeric_result.bytes = detail::constexpr_eval_antestatal_helpers::decode_uintany_string(value.bytes).bytes;
            result.values[id] = constexpr_value(std::move(numeric_result));
        }
        else
        {
            result.values[id] = constexpr_value(std::move(value));
        }
    }
    result.deduced_type = std::move(routine_result.deduced_type);
    result.type_binding_result = std::move(routine_result.type_binding_result);
    co_return result;
}

/// Adapts multi-result antestatal constexpr evaluation to the legacy single-result query.
rpnx::querygraph::coroutine< quxlang::constexpr_eval_antestatal_spec > quxlang::constexpr_eval_antestatal_impl(constexpr_input2 input)
{
    auto result = co_await rpnx::querygraph::request< constexpr_eval_v3_query >(detail::constexpr_eval_antestatal_helpers::make_v3_input(std::move(input)));
    if (result.values.contains(constexpr_primary_result_id))
    {
        co_return constexpr_value_as_antestatal(result.values.at(constexpr_primary_result_id));
    }
    throw compiler_bug("constexpr antestatal evaluation did not produce result id 0");
}
