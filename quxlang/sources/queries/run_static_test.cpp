// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/run_static_test_spec.hpp>

#include <quxlang/co_vmir_generator2.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/exception.hpp>
#include <quxlang/vmir2/ir2_constexpr_interpreter.hpp>
#include <quxlang/vmir2/routine_requirements.hpp>
#include <quxlang/vmir2/source_index.hpp>

#include <optional>
#include <set>
#include <stdexcept>
#include <utility>
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
        auto provide_antestatal_global(quxlang::vmir2::ir2_constexpr_interpreter& interp, quxlang::type_symbol const& symbol)
            -> run_static_test_coroutine::cosubroutine< std::pair< std::set< quxlang::type_symbol >, std::set< quxlang::type_symbol > > >;
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
            quxlang::symbol_kind const kind = co_await rpnx::querygraph::request< quxlang::symbol_type_query >(type);
            if (kind == quxlang::symbol_kind::enum_)
            {
                quxlang::enum_info const info = co_await rpnx::querygraph::request< quxlang::enum_info_query >(type);
                interp.add_nominal_integer_type(type, info.bits);
                continue;
            }
            if (kind == quxlang::symbol_kind::flagset_)
            {
                quxlang::flagset_info const info = co_await rpnx::querygraph::request< quxlang::flagset_info_query >(type);
                interp.add_nominal_integer_type(type, info.bits);
                continue;
            }
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
        for (auto const& [_, localdata] : routine.static_snapshots)
        {
            co_await add_layouts_for_type(interp, localdata.type);
        }
    }

    auto static_test_dependency_provider::provide_antestatal_global(quxlang::vmir2::ir2_constexpr_interpreter& interp, quxlang::type_symbol const& symbol)
        -> run_static_test_coroutine::cosubroutine< std::pair< std::set< quxlang::type_symbol >, std::set< quxlang::type_symbol > > >
    {
        if (!(co_await rpnx::querygraph::request< quxlang::global_is_antestatal_static_query >(symbol)))
        {
            throw quxlang::compiler_bug("constexpr interpreter requested non-antestatal global data: " + quxlang::to_string(symbol));
        }

        quxlang::type_symbol type = co_await rpnx::querygraph::request< quxlang::variable_type_query >(symbol);
        co_await add_layouts_for_type(interp, type);
        quxlang::antestatal_value value = co_await rpnx::querygraph::request< quxlang::antestatal_static_value_query >(symbol);
        std::set< quxlang::type_symbol > value_functanoids = quxlang::vmir2::directly_instantiated_functanoids(value, type);
        std::set< quxlang::type_symbol > value_antestatal_globals = quxlang::vmir2::directly_referenced_antestatal_globals(value, type);
        interp.add_constexpr_antestatal_global(symbol, type, std::move(value));
        co_return std::pair(std::move(value_functanoids), std::move(value_antestatal_globals));
    }

    auto execute_static_test_routine(quxlang::vmir2::functanoid_routine3 const& routine, quxlang::vmir2::source_index source_index) -> run_static_test_coroutine::cosubroutine< void >
    {
        quxlang::vmir2::ir2_constexpr_interpreter interp;
        interp.set_source_index(std::move(source_index));
        static_test_dependency_provider dependency_provider;
        std::vector< quxlang::instanciation_reference > pending_functanoids;
        std::set< quxlang::type_symbol > queued_functanoids;
        std::set< quxlang::type_symbol > loaded_functanoids;
        std::vector< quxlang::type_symbol > pending_antestatal_globals;
        std::set< quxlang::type_symbol > queued_antestatal_globals;
        std::set< quxlang::type_symbol > loaded_antestatal_globals;

        auto enqueue_functanoid = [&](quxlang::type_symbol const& funcname)
        {
            if (!quxlang::typeis< quxlang::instanciation_reference >(funcname))
            {
                throw quxlang::compiler_bug("functanoid dependency is not an instanciation reference: " + quxlang::to_string(funcname));
            }
            if (loaded_functanoids.contains(funcname) || !queued_functanoids.insert(funcname).second)
            {
                return;
            }
            pending_functanoids.push_back(funcname.get_as< quxlang::instanciation_reference >());
        };

        auto enqueue_functanoids = [&](std::set< quxlang::type_symbol > const& functanoids)
        {
            for (quxlang::type_symbol const& funcname : functanoids)
            {
                enqueue_functanoid(funcname);
            }
        };

        auto enqueue_antestatal_global = [&](quxlang::type_symbol const& symbol)
        {
            if (loaded_antestatal_globals.contains(symbol) || !queued_antestatal_globals.insert(symbol).second)
            {
                return;
            }
            pending_antestatal_globals.push_back(symbol);
        };

        auto enqueue_antestatal_globals = [&](std::set< quxlang::type_symbol > const& symbols)
        {
            for (quxlang::type_symbol const& symbol : symbols)
            {
                enqueue_antestatal_global(symbol);
            }
        };

        auto add_layouts_for_functanoid = [&](quxlang::instanciation_reference const& functanoid) -> run_static_test_coroutine::cosubroutine< void >
        {
            std::set< quxlang::type_symbol > const required_layouts = co_await rpnx::querygraph::request< quxlang::functanoid_required_class_layouts_query >(
                quxlang::functanoid_requirement_input{.functanoid = functanoid, .compilation_type = quxlang::functanoid_compilation_type::all});
            for (quxlang::type_symbol const& type : required_layouts)
            {
                co_await dependency_provider.add_layouts_for_type(interp, type);
            }
        };

        co_await dependency_provider.add_layouts_for_routine(interp, routine);
        enqueue_functanoids(quxlang::vmir2::directly_instantiated_functanoids(routine));
        enqueue_antestatal_globals(quxlang::vmir2::directly_referenced_antestatal_globals(routine));
        interp.add_functanoid3(quxlang::void_type{}, routine);
        loaded_functanoids.insert(quxlang::type_symbol(quxlang::void_type{}));

        while (!pending_functanoids.empty() || !pending_antestatal_globals.empty())
        {
            while (!pending_antestatal_globals.empty())
            {
                quxlang::type_symbol symbol = std::move(pending_antestatal_globals.back());
                pending_antestatal_globals.pop_back();
                if (loaded_antestatal_globals.contains(symbol))
                {
                    continue;
                }

                std::pair< std::set< quxlang::type_symbol >, std::set< quxlang::type_symbol > > requirements = co_await dependency_provider.provide_antestatal_global(interp, symbol);
                loaded_antestatal_globals.insert(symbol);
                enqueue_functanoids(requirements.first);
                enqueue_antestatal_globals(requirements.second);
            }

            while (!pending_functanoids.empty())
            {
                quxlang::instanciation_reference functanoid = std::move(pending_functanoids.back());
                pending_functanoids.pop_back();

                quxlang::type_symbol funcname = functanoid;
                if (loaded_functanoids.contains(funcname))
                {
                    continue;
                }

                co_await add_layouts_for_functanoid(functanoid);
                quxlang::vmir2::functanoid_routine3 const& missing_routine = co_await rpnx::querygraph::request< quxlang::vm_procedure3_query >(functanoid);
                interp.add_functanoid3(funcname, missing_routine);
                loaded_functanoids.insert(funcname);
                enqueue_antestatal_globals(quxlang::vmir2::directly_referenced_antestatal_globals(missing_routine));

                std::set< quxlang::type_symbol > const direct_functanoids = co_await rpnx::querygraph::request< quxlang::functanoid_indirectly_instantiated_functanoids_query >(
                    quxlang::functanoid_requirement_input{.functanoid = functanoid, .compilation_type = quxlang::functanoid_compilation_type::all});
                enqueue_functanoids(direct_functanoids);
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
        throw quxlang::compiler_bug("run_static_test received a symbol that is not a static test: " + quxlang::to_string(input));
    }

    auto const& test = as< ast2_static_test >(sym);
    std::optional< vmir2::functanoid_routine3 > routine;

    try
    {
        routine = co_await rpnx::querygraph::request< quxlang::static_test_vmir_query >(input);
    }
    catch (compiler_bug const&)
    {
        throw;
    }
    catch (constexpr_runtime_error const& error)
    {
        if (test.expected_mode == static_test_expected_mode::expect_fail)
        {
            co_return true;
        }
        throw semantic_compilation_error(error.what());
    }
    catch (compilation_error const&)
    {
        if (test.expected_mode == static_test_expected_mode::expect_compilation_failure)
        {
            co_return true;
        }
        throw;
    }
    catch (std::logic_error const&)
    {
        if (test.expected_mode == static_test_expected_mode::expect_compilation_failure)
        {
            co_return true;
        }
        throw;
    }

    try
    {
        auto source_file_index = co_await rpnx::querygraph::request< source_file_index_query >(std::monostate{});
        auto source_bundle = co_await rpnx::querygraph::request< source_bundle_query >(std::monostate{});
        co_await execute_static_test_routine(*routine, quxlang::vmir2::source_index(source_file_index, source_bundle));
    }
    catch (constexpr_runtime_error const& error)
    {
        if (test.expected_mode == static_test_expected_mode::expect_fail)
        {
            co_return true;
        }
        throw semantic_compilation_error(error.what());
    }
    catch (compiler_bug const&)
    {
        throw;
    }
    catch (compilation_error const&)
    {
        if (test.expected_mode == static_test_expected_mode::expect_compilation_failure)
        {
            co_return true;
        }
        throw;
    }
    catch (std::logic_error const&)
    {
        if (test.expected_mode == static_test_expected_mode::expect_compilation_failure)
        {
            co_return true;
        }
        throw;
    }

    if (test.expected_mode == static_test_expected_mode::expect_fail)
    {
        throw quxlang::semantic_compilation_error("STATIC_TEST EXPECT_FAIL completed successfully: " + quxlang::to_string(input));
    }
    if (test.expected_mode == static_test_expected_mode::expect_compilation_failure)
    {
        throw quxlang::semantic_compilation_error("STATIC_TEST EXPECT_COMPILATION_FAILURE compiled and executed successfully: " + quxlang::to_string(input));
    }

    co_return true;
}
