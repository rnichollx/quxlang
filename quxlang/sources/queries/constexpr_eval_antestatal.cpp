// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/constexpr_eval_antestatal_spec.hpp>

#include "quxlang/bytemath.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"
#include "quxlang/vmir2/source_index.hpp"

#include <set>
#include <stdexcept>
#include <vector>

namespace
{
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
} // namespace

rpnx::querygraph::coroutine< quxlang::constexpr_eval_antestatal_spec > quxlang::constexpr_eval_antestatal_impl(constexpr_input2 input)
{
    vmir2::ir2_constexpr_interpreter interp;
    auto source_file_index = co_await rpnx::querygraph::request< source_file_index_query >(std::monostate{});
    auto source_bundle = co_await rpnx::querygraph::request< source_bundle_query >(std::monostate{});
    interp.set_source_index(vmir2::source_index(source_file_index, source_bundle));

    interp.set_constexpr_result_global_symbol(input.antestatal_global_symbol);

    std::set< type_symbol > layout_types;

    auto add_layouts_for_type = [&](type_symbol type) -> rpnx::querygraph::coroutine< quxlang::constexpr_eval_antestatal_spec >::cosubroutine< void >
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

    auto add_layouts_for_routine = [&](vmir2::functanoid_routine3 const& routine) -> rpnx::querygraph::coroutine< quxlang::constexpr_eval_antestatal_spec >::cosubroutine< void >
    {
        co_await add_layouts_for_type(input.type);
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
    };



    auto provide_missing_antestatal_globals = [&]() -> rpnx::querygraph::coroutine< quxlang::constexpr_eval_antestatal_spec >::cosubroutine< void >
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
                interp.add_constexpr_antestatal_global(symbol, type, std::move(value));
            }
        }
    };

    auto ir3 = co_await rpnx::querygraph::request< constexpr_routine_antestatal_query >(input);
    co_await add_layouts_for_routine(ir3);

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

    co_return interp.get_cr_antestatal_value();
}
