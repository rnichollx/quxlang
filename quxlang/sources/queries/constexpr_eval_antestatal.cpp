// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/constexpr_eval_antestatal_spec.hpp>
#include <quxlang/queries/specs/constexpr_eval_v3_spec.hpp>

#include "quxlang/bytemath.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"
#include "quxlang/vmir2/source_index.hpp"

#include <set>
#include <stdexcept>
#include <vector>

namespace
{
    /// Converts legacy constexpr input into the versioned constexpr v3 input shape.
    auto make_v3_input(quxlang::constexpr_input2 input) -> quxlang::constexpr_input_v3
    {
        quxlang::constexpr_input_v3 result;
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

    /// Adds nested value types that may require class layout during antestatal materialization.
    auto add_type_for_layout_scan(std::vector< quxlang::type_symbol >& pending, quxlang::type_symbol type) -> void
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

    /// Returns true when a type symbol may need a class_layout query before interpretation.
    auto type_might_have_layout(quxlang::type_symbol const& type) -> bool
    {
        return type.type_is< quxlang::subsymbol >() || type.type_is< quxlang::instanciation_reference >() || type.type_is< quxlang::readonly_constant >();
    }
} // namespace

/// Evaluates a constexpr v3 expression and returns every requested result ID.
rpnx::querygraph::coroutine< quxlang::constexpr_eval_v3_spec > quxlang::constexpr_eval_v3_impl(constexpr_input_v3 input)
{
    vmir2::ir2_constexpr_interpreter interp;
    auto source_file_index = co_await rpnx::querygraph::request< source_file_index_query >(std::monostate{});
    auto source_bundle = co_await rpnx::querygraph::request< source_bundle_query >(std::monostate{});
    interp.set_source_index(vmir2::source_index(source_file_index, source_bundle));

    interp.set_constexpr_result_global_symbol(input.antestatal_global_symbol);

    std::set< type_symbol > layout_types;

    /// Adds all class layouts needed to materialize or execute values of one type.
    auto add_layouts_for_type = [&](type_symbol type) -> rpnx::querygraph::coroutine< quxlang::constexpr_eval_v3_spec >::cosubroutine< void >
    {
        std::vector< type_symbol > pending{std::move(type)};

        while (!pending.empty())
        {
            auto type = pending.back();
            pending.pop_back();

            add_type_for_layout_scan(pending, type);

            if (!type_might_have_layout(type) || layout_types.contains(type))
            {
                continue;
            }

            layout_types.insert(type);
            auto layout = co_await rpnx::querygraph::request< class_layout_query >(type);
            interp.add_class_layout(type, layout);
            for (auto const& field : layout.fields)
            {
                pending.push_back(field.type);
            }
        }
    };

    /// Adds all class layouts reachable from a generated VMIR routine.
    auto add_layouts_for_routine = [&](vmir2::functanoid_routine3 const& routine) -> rpnx::querygraph::coroutine< quxlang::constexpr_eval_v3_spec >::cosubroutine< void >
    {
        if (input.expected_result_type.has_value() && !typeis< auto_temploidic >(*input.expected_result_type))
        {
            co_await add_layouts_for_type(*input.expected_result_type);
        }
        for (auto const& local_type : routine.local_types)
        {
            co_await add_layouts_for_type(local_type.type);
        }
        for (auto const& [_, param] : routine.parameters.named)
        {
            co_await add_layouts_for_type(param.type);
        }
        for (auto const& param : routine.parameters.positional)
        {
            co_await add_layouts_for_type(param.type);
        }
        for (auto const& [_, localdata] : routine.static_snapshots)
        {
            co_await add_layouts_for_type(localdata.type);
        }
    };



    /// Provides querygraph-backed global antestatal statics requested by the interpreter.
    auto provide_missing_antestatal_globals = [&]() -> rpnx::querygraph::coroutine< quxlang::constexpr_eval_v3_spec >::cosubroutine< void >
    {
        while (!interp.missing_antestatal_globals().empty())
        {
            auto missing_antestatal_globals = interp.missing_antestatal_globals();

            for (auto const& symbol : missing_antestatal_globals)
            {
                if (!(co_await rpnx::querygraph::request< global_is_antestatal_static_query >(symbol)))
                {
                    throw std::logic_error("constexpr interpreter requested non-antestatal global data: " + quxlang::to_string(symbol));
                }

                auto type = co_await rpnx::querygraph::request< variable_type_query >(symbol);
                co_await add_layouts_for_type(type);
                auto value = co_await rpnx::querygraph::request< antestatal_static_value_query >(symbol);
                interp.add_constexpr_antestatal_global(symbol, type, std::move(value), false);
            }
        }
    };

    auto routine_result = co_await rpnx::querygraph::request< constexpr_routine_v3_query >(input);
    auto const& ir3 = routine_result.routine;
    co_await add_layouts_for_routine(ir3);
    for (auto const& [_, localdata] : input.statics)
    {
        co_await add_layouts_for_type(localdata.type);
    }

    for (auto const& [symbol, localdata] : input.statics)
    {
        interp.add_constexpr_antestatal_global(type_symbol(symbol), localdata.type, constexpr_value_as_antestatal(localdata.value), localdata.mutation_result_id.has_value());
    }
    interp.add_functanoid3(void_type{}, ir3);

    while (!interp.missing_functanoids().empty() || !interp.missing_antestatal_globals().empty())
    {
        co_await provide_missing_antestatal_globals();

        auto missing_functanoids = interp.missing_functanoids();

        for (type_symbol const& funcname : missing_functanoids)
        {
            if (!typeis< instanciation_reference >(funcname))
            {
                throw compiler_bug("Internal Compiler Error: Missing functanoid is not an instanciation reference");
            }
            vmir2::functanoid_routine3 const& ir2_other = co_await rpnx::querygraph::request< vm_procedure3_query >(funcname.template get_as< instanciation_reference >());
            co_await add_layouts_for_routine(ir2_other);

            interp.add_functanoid3(funcname, ir2_other);
        }
    }

    interp.exec3(void_type{});

    constexpr_result_v3 result;
    for (auto& [id, value] : interp.get_cr_antestatal_values())
    {
        result.values[id] = constexpr_value(std::move(value));
    }
    result.deduced_type = std::move(routine_result.deduced_type);
    co_return result;
}

/// Adapts multi-result antestatal constexpr evaluation to the legacy single-result query.
rpnx::querygraph::coroutine< quxlang::constexpr_eval_antestatal_spec > quxlang::constexpr_eval_antestatal_impl(constexpr_input2 input)
{
    auto result = co_await rpnx::querygraph::request< constexpr_eval_v3_query >(make_v3_input(std::move(input)));
    if (result.values.contains(constexpr_primary_result_id))
    {
        co_return constexpr_value_as_antestatal(result.values.at(constexpr_primary_result_id));
    }
    throw compiler_bug("constexpr antestatal evaluation did not produce result id 0");
}
