// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/constexpr_eval_spec.hpp>
#include "quxlang/bytemath.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"
#include "quxlang/vmir2/source_index.hpp"

#include <set>
#include <stdexcept>
#include <vector>

namespace
{
    using constexpr_eval_coroutine = rpnx::querygraph::coroutine< quxlang::constexpr_eval_spec >;

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

    auto type_might_have_layout(quxlang::type_symbol const& type) -> bool
    {
        return type.type_is< quxlang::subsymbol >() || type.type_is< quxlang::instanciation_reference >() || type.type_is< quxlang::readonly_constant >();
    }

    struct constexpr_eval_dependency_provider
    {
        std::set< quxlang::type_symbol > layout_types;

        auto add_layouts_for_type(quxlang::vmir2::ir2_constexpr_interpreter& interp, quxlang::type_symbol type) -> constexpr_eval_coroutine::cosubroutine< void >;
        auto add_layouts_for_routine(quxlang::vmir2::ir2_constexpr_interpreter& interp, quxlang::type_symbol result_type, quxlang::vmir2::functanoid_routine3 const& routine) -> constexpr_eval_coroutine::cosubroutine< void >;
        auto provide_missing_antestatal_globals(quxlang::vmir2::ir2_constexpr_interpreter& interp) -> constexpr_eval_coroutine::cosubroutine< void >;
    };

    auto constexpr_eval_dependency_provider::add_layouts_for_type(quxlang::vmir2::ir2_constexpr_interpreter& interp, quxlang::type_symbol type) -> constexpr_eval_coroutine::cosubroutine< void >
    {
        std::vector< quxlang::type_symbol > pending{std::move(type)};

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
            auto layout = co_await rpnx::querygraph::request< quxlang::class_layout_query >(type);
            interp.add_class_layout(type, layout);
            for (auto const& field : layout.fields)
            {
                pending.push_back(field.type);
            }
        }
    }

    auto constexpr_eval_dependency_provider::add_layouts_for_routine(quxlang::vmir2::ir2_constexpr_interpreter& interp, quxlang::type_symbol result_type, quxlang::vmir2::functanoid_routine3 const& routine) -> constexpr_eval_coroutine::cosubroutine< void >
    {
        co_await add_layouts_for_type(interp, result_type);
        for (auto const& local_type : routine.local_types)
        {
            co_await add_layouts_for_type(interp, local_type.type);
        }
        for (auto const& [_, param] : routine.parameters.named)
        {
            co_await add_layouts_for_type(interp, param.type);
        }
        for (auto const& param : routine.parameters.positional)
        {
            co_await add_layouts_for_type(interp, param.type);
        }
    }

    auto constexpr_eval_dependency_provider::provide_missing_antestatal_globals(quxlang::vmir2::ir2_constexpr_interpreter& interp) -> constexpr_eval_coroutine::cosubroutine< void >
    {
        while (!interp.missing_antestatal_globals().empty())
        {
            auto missing_antestatal_globals = interp.missing_antestatal_globals();

            for (quxlang::type_symbol const& symbol : missing_antestatal_globals)
            {
                if (!(co_await rpnx::querygraph::request< quxlang::global_is_antestatal_static_query >(symbol)))
                {
                    throw std::logic_error("constexpr interpreter requested non-antestatal global data: " + quxlang::to_string(symbol));
                }

                auto type = co_await rpnx::querygraph::request< quxlang::variable_type_query >(symbol);
                co_await add_layouts_for_type(interp, type);
                auto value = co_await rpnx::querygraph::request< quxlang::antestatal_static_value_query >(symbol);
                interp.add_constexpr_antestatal_global(symbol, type, std::move(value));
            }
        }
    }
} // namespace

rpnx::querygraph::coroutine< quxlang::constexpr_eval_spec > quxlang::constexpr_eval_impl(constexpr_input2 input)
{
    vmir2::ir2_constexpr_interpreter interp;
    auto source_file_index = co_await rpnx::querygraph::request< source_file_index_query >(std::monostate{});
    auto source_bundle = co_await rpnx::querygraph::request< source_bundle_query >(std::monostate{});
    interp.set_source_index(vmir2::source_index(source_file_index, source_bundle));

    constexpr_eval_dependency_provider dependency_provider;

    auto ir3 = co_await rpnx::querygraph::request< constexpr_routine_query >(input);
    co_await dependency_provider.add_layouts_for_routine(interp, input.type, ir3);

    interp.add_functanoid3(void_type{}, ir3);

    while (!interp.missing_functanoids().empty() || !interp.missing_antestatal_globals().empty())
    {
        co_await dependency_provider.provide_missing_antestatal_globals(interp);

        auto missing_functanoids = interp.missing_functanoids();

        for (type_symbol const& funcname : missing_functanoids)
        {
            if (!typeis< instanciation_reference >(funcname))
            {
                throw compiler_bug("Internal Compiler Error: Missing functanoid is not an instanciation reference");
            }
            vmir2::functanoid_routine3 const& ir2_other = co_await rpnx::querygraph::request< vm_procedure3_query >(funcname.template get_as< instanciation_reference >());
            co_await dependency_provider.add_layouts_for_routine(interp, input.type, ir2_other);

            interp.add_functanoid3(funcname, ir2_other);
        }
    }

    interp.exec3(void_type{});

    auto val = interp.get_cr_value();
    val.type = type_symbol(bool_type{});
    co_return val;
}
