// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/run_static_test_spec.hpp>

#include <quxlang/co_vmir_generator2.hpp>
#include <quxlang/exception.hpp>
#include <quxlang/vmir2/ir2_constexpr_interpreter.hpp>

#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

namespace
{
    using run_static_test_coroutine = rpnx::querygraph::coroutine< quxlang::run_static_test_spec >;

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

    struct static_test_dependency_provider
    {
        std::set< quxlang::type_symbol > layout_types;

        auto add_layouts_for_type(quxlang::vmir2::ir2_constexpr_interpreter& interp, quxlang::type_symbol type) -> run_static_test_coroutine::cosubroutine< void >;
        auto add_layouts_for_routine(quxlang::vmir2::ir2_constexpr_interpreter& interp, quxlang::vmir2::functanoid_routine3 const& routine) -> run_static_test_coroutine::cosubroutine< void >;
        auto provide_missing_antestatal_globals(quxlang::vmir2::ir2_constexpr_interpreter& interp) -> run_static_test_coroutine::cosubroutine< void >;
    };

    auto static_test_dependency_provider::add_layouts_for_type(quxlang::vmir2::ir2_constexpr_interpreter& interp, quxlang::type_symbol type) -> run_static_test_coroutine::cosubroutine< void >
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

    auto static_test_dependency_provider::add_layouts_for_routine(quxlang::vmir2::ir2_constexpr_interpreter& interp, quxlang::vmir2::functanoid_routine3 const& routine) -> run_static_test_coroutine::cosubroutine< void >
    {
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

    auto static_test_dependency_provider::provide_missing_antestatal_globals(quxlang::vmir2::ir2_constexpr_interpreter& interp) -> run_static_test_coroutine::cosubroutine< void >
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

    auto generate_static_test_routine(quxlang::type_symbol input, quxlang::ast2_static_test const& test) -> run_static_test_coroutine::cosubroutine< quxlang::vmir2::functanoid_routine3 >
    {
        auto machine_info = co_await rpnx::querygraph::request< quxlang::machine_info_query >(quxlang::machine_info_query::input_type{});
        quxlang::co_vmir_generator2< run_static_test_coroutine > gen(machine_info, input);
        co_return co_await gen.co_generate_static_test(test);
    }

    auto execute_static_test_routine(quxlang::vmir2::functanoid_routine3 const& routine) -> run_static_test_coroutine::cosubroutine< void >
    {
        quxlang::vmir2::ir2_constexpr_interpreter interp;
        static_test_dependency_provider dependency_provider;

        co_await dependency_provider.add_layouts_for_routine(interp, routine);
        interp.add_functanoid3(quxlang::void_type{}, routine);

        while (!interp.missing_functanoids().empty() || !interp.missing_antestatal_globals().empty())
        {
            co_await dependency_provider.provide_missing_antestatal_globals(interp);

            auto missing_functanoids = interp.missing_functanoids();

            for (quxlang::type_symbol const& funcname : missing_functanoids)
            {
                if (!quxlang::typeis< quxlang::instanciation_reference >(funcname))
                {
                    throw quxlang::compiler_bug("Internal Compiler Error: Missing functanoid is not an instanciation reference");
                }
                quxlang::vmir2::functanoid_routine3 const& missing_routine = co_await rpnx::querygraph::request< quxlang::vm_procedure3_query >(funcname.template get_as< quxlang::instanciation_reference >());
                co_await dependency_provider.add_layouts_for_routine(interp, missing_routine);

                interp.add_functanoid3(funcname, missing_routine);
            }
        }

        interp.exec3(quxlang::void_type{});
    }
} // namespace

rpnx::querygraph::coroutine< quxlang::run_static_test_spec > quxlang::run_static_test_impl(type_symbol input)
{
    auto sym = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_static_test >(sym))
    {
        throw std::logic_error("run_static_test received a symbol that is not a static test: " + quxlang::to_string(input));
    }

    auto const& test = as< ast2_static_test >(sym);
    std::optional< vmir2::functanoid_routine3 > routine;

    try
    {
        routine = co_await generate_static_test_routine(input, test);
    }
    catch (compiler_bug const&)
    {
        throw;
    }
    catch (compilation_error const&)
    {
        if (test.expected_mode == static_test_expected_mode::expect_nocompile)
        {
            co_return true;
        }
        throw;
    }
    catch (std::logic_error const&)
    {
        if (test.expected_mode == static_test_expected_mode::expect_nocompile)
        {
            co_return true;
        }
        throw;
    }

    if (test.expected_mode == static_test_expected_mode::expect_nocompile)
    {
        throw std::logic_error("STATIC_TEST EXPECT_NOCOMPILE generated successfully: " + quxlang::to_string(input));
    }

    try
    {
        co_await execute_static_test_routine(*routine);
    }
    catch (constexpr_logic_execution_error const&)
    {
        if (test.expected_mode == static_test_expected_mode::expect_fail)
        {
            co_return true;
        }
        throw;
    }

    if (test.expected_mode == static_test_expected_mode::expect_fail)
    {
        throw std::logic_error("STATIC_TEST EXPECT_FAIL completed successfully: " + quxlang::to_string(input));
    }

    co_return true;
}
