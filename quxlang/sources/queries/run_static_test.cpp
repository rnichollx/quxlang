// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/profiling.hpp"

#include <quxlang/queries/specs/run_static_test_spec.hpp>

#include <quxlang/co_vmir_generator2.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/exception.hpp>
#include <quxlang/vmir2/ir2_constexpr_interpreter.hpp>
#include <quxlang/vmir2/routine_requirements.hpp>
#include <quxlang/vmir2/source_index.hpp>

#include <optional>
#include <rpnx/serialization4.hpp>
#include <set>
#include <string>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>
#include <memory>
#include <utility>

namespace
{
    auto static_test_failure_message(quxlang::type_symbol const& test_symbol, char const* phase, char const* detail) -> std::string
    {
        return "STATIC_TEST " + quxlang::to_string(test_symbol) + " failed while " + phase + ": " + detail;
    }

    auto static_test_failure_error(quxlang::type_symbol const& test_symbol, char const* phase, char const* detail) -> quxlang::compilation_error
    {
        quxlang::compilation_error error = quxlang::semantic_compilation_error(static_test_failure_message(test_symbol, phase, detail));
        error.traceback.push_back(quxlang::trace_frame{.trace_context = "static test " + quxlang::to_string(test_symbol)});
        return error;
    }

    auto static_test_failure_error(quxlang::type_symbol const& test_symbol, char const* phase, quxlang::compilation_error error) -> quxlang::compilation_error
    {
        std::string message = static_test_failure_message(test_symbol, phase, error.what());
        error.message = message;
        error.structured_error = quxlang::semantic_error{std::move(message)};
        error.traceback.push_back(quxlang::trace_frame{.trace_context = "static test " + quxlang::to_string(test_symbol)});
        return error;
    }

    auto static_test_compiler_bug(quxlang::type_symbol const& test_symbol, char const* phase, quxlang::compiler_bug const& error) -> quxlang::compiler_bug
    {
        std::string detail = error.what();
        std::string const prefix = "Compiler Bug: ";
        if (detail.starts_with(prefix))
        {
            detail.erase(0, prefix.size());
        }
        return quxlang::compiler_bug(static_test_failure_message(test_symbol, phase, detail.c_str()));
    }
} // namespace

rpnx::querygraph::coroutine< quxlang::run_static_test_spec > quxlang::run_static_test_impl(type_symbol input)
{


    auto const& sym = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_test >(sym) || !(co_await rpnx::querygraph::request< test_is_enabled_for_static_testing_query >(input)))
    {
        throw quxlang::compiler_bug("run_static_test received a symbol that is not a static test: " + quxlang::to_string(input));
    }

    auto const& test = as< ast2_test >(sym);
    std::optional< vmir2::functanoid_routine3 > routine;

    try
    {
        routine = co_await rpnx::querygraph::request< quxlang::static_test_vmir_query >(input);
    }
    catch (compiler_bug const& error)
    {
        throw static_test_compiler_bug(input, "compiling", error);
    }
    catch (constexpr_runtime_error const& error)
    {
        if (test.expected_mode == static_test_expected_mode::expect_fail || test.expected_mode == static_test_expected_mode::expect_compilation_failure)
        {
            co_return true;
        }
        throw static_test_failure_error(input, "compiling", error.what());
    }
    catch (compilation_error const& error)
    {
        if (test.expected_mode == static_test_expected_mode::expect_compilation_failure)
        {
            co_return true;
        }
        throw static_test_failure_error(input, "compiling", error);
    }
    catch (std::logic_error const& error)
    {
        if (test.expected_mode == static_test_expected_mode::expect_compilation_failure)
        {
            co_return true;
        }
        throw static_test_failure_error(input, "compiling", error.what());
    }

    try
    {
        auto const& source_file_index = co_await rpnx::querygraph::request< source_file_index_query >(std::monostate{});
        auto const& source_bundle = co_await rpnx::querygraph::request< source_bundle_query >(std::monostate{});

        vmir2::ir2_constexpr_interpreter interp;
        interp.set_source_index(vmir2::source_index(source_file_index, source_bundle));

        std::unordered_set< type_symbol, rpnx::serial4::hash > functanoids;
        std::unordered_set< type_symbol, rpnx::serial4::hash > antestatal_globals;
        std::unordered_set< type_symbol, rpnx::serial4::hash > layout_types;
        std::unordered_set< type_symbol, rpnx::serial4::hash > loaded_layouts;
        std::unordered_set< type_symbol, rpnx::serial4::hash > loaded_functanoids;
        std::set< type_symbol > asm_procedure_symbols;
        std::vector< type_symbol > functanoid_list;
        std::vector< type_symbol > antestatal_global_list;

        functanoids.reserve(64);
        antestatal_globals.reserve(64);
        layout_types.reserve(64);
        loaded_layouts.reserve(64);
        loaded_functanoids.reserve(64);

        dependencies const& root_dependencies = co_await rpnx::querygraph::request< direct_dependencies_query >(
            direct_dependencies_input{.symbol = input, .set = dependency_set::constexpr_});
        for (type_symbol const& type : root_dependencies.struct_layouts)
        {
            run_under_profiling_void("run_static_test seed layout_types insert",
                                     [&]
                                     {
                                         layout_types.insert(type);
                                     });
        }
        for (type_symbol const& type : root_dependencies.fusion_layouts)
        {
            run_under_profiling_void("run_static_test seed fusion layout_types insert",
                                     [&]
                                     {
                                         layout_types.insert(type);
                                     });
        }
        for (auto const& [funcname, _] : root_dependencies.functanoids)
        {
            run_under_profiling_void("run_static_test seed functanoid loop body",
                                     [&]
                                     {
                                         if (functanoids.insert(funcname).second)
                                         {
                                             functanoid_list.push_back(funcname);
                                         }
                                     });
        }
        for (type_symbol const& symbol : root_dependencies.antestatal_globals)
        {
            run_under_profiling_void("run_static_test seed antestatal global loop body",
                                     [&]
                                     {
                                         if (antestatal_globals.insert(symbol).second)
                                         {
                                             antestatal_global_list.push_back(symbol);
                                         }
                                     });
        }

        std::size_t functanoid_index = 0;
        std::size_t antestatal_global_index = 0;
        while (functanoid_index != functanoid_list.size() || antestatal_global_index != antestatal_global_list.size())
        {
            while (functanoid_index != functanoid_list.size())
            {
                std::size_t const round_end = functanoid_list.size();
                for (std::size_t i = functanoid_index; i != round_end; ++i)
                {
                    type_symbol const& funcname = functanoid_list[i];
                    if (!typeis< instanciation_reference >(funcname))
                    {
                        throw compiler_bug("functanoid dependency is not an instanciation reference: " + to_string(funcname));
                    }

                    instanciation_reference const& functanoid = funcname.get_as< instanciation_reference >();
                    ast2_symboid const& symboid = co_await rpnx::querygraph::request< symboid_query >(functanoid.temploid.templexoid);
                    if (typeis< ast2_asm_procedure_declaration >(symboid) || typeis< ast2_extern_procedure >(symboid))
                    {
                        if (asm_procedure_symbols.insert(funcname).second)
                        {
                            interp.add_constexpr_asm_procedure(funcname);
                        }
                        loaded_functanoids.insert(funcname);
                        continue;
                    }

                    co_yield rpnx::querygraph::dependency< vm_procedure3_query >(rpnx::querygraph::request< vm_procedure3_query >(functanoid));
                }

                while (functanoid_index != round_end)
                {
                    type_symbol funcname = functanoid_list[functanoid_index];
                    ++functanoid_index;

                    if (!typeis< instanciation_reference >(funcname))
                    {
                        throw compiler_bug("functanoid dependency is not an instanciation reference: " + to_string(funcname));
                    }

                    if (loaded_functanoids.contains(funcname))
                    {
                        continue;
                    }

                    instanciation_reference const& functanoid = funcname.get_as< instanciation_reference >();
                    auto const& dependency_routine = co_await rpnx::querygraph::request< vm_procedure3_query >(functanoid);
                    dependencies const& dependencies = co_await rpnx::querygraph::request< direct_dependencies_query >(
                        direct_dependencies_input{.symbol = functanoid, .set = dependency_set::constexpr_});
                    for (auto const& [dependency, _] : dependencies.functanoids)
                    {
                        run_under_profiling_void("run_static_test required functanoid loop body",
                                                 [&]
                                                 {
                                                     if (functanoids.insert(dependency).second)
                                                     {
                                                         functanoid_list.push_back(dependency);
                                                     }
                                                 });
                    }
                    for (type_symbol const& type : dependencies.struct_layouts)
                    {
                        run_under_profiling_void("run_static_test required layout loop body",
                                                 [&]
                                                 {
                                                     layout_types.insert(type);
                                                 });
                    }
                    for (type_symbol const& type : dependencies.fusion_layouts)
                    {
                        run_under_profiling_void("run_static_test required fusion layout loop body",
                                                 [&]
                                                 {
                                                     layout_types.insert(type);
                                                 });
                    }

                    if (loaded_functanoids.insert(funcname).second)
                    {
                        run_under_profiling_void("run_static_test add functanoid3",
                                                 [&]
                                                 {
                                                     interp.add_functanoid3(funcname, dependency_routine, dependencies.static_snapshots);
                                                 });
                        for (type_symbol const& symbol : dependencies.antestatal_globals)
                        {
                            run_under_profiling_void("run_static_test dependency routine antestatal global loop body",
                                                     [&]
                                                     {
                                                         if (antestatal_globals.insert(symbol).second)
                                                         {
                                                             antestatal_global_list.push_back(symbol);
                                                         }
                                                     });
                        }
                    }
                }
            }

            while (antestatal_global_index != antestatal_global_list.size())
            {
                type_symbol symbol = antestatal_global_list[antestatal_global_index];
                ++antestatal_global_index;

                if (!(co_await rpnx::querygraph::request< global_is_antestatal_static_query >(symbol)))
                {
                    throw compiler_bug("constexpr interpreter requested non-antestatal global data: " + to_string(symbol));
                }

                type_symbol const& type = co_await rpnx::querygraph::request< variable_type_query >(symbol);
                antestatal_value const& value = co_await rpnx::querygraph::request< antestatal_static_value_query >(symbol);
                run_under_profiling_void("run_static_test add constexpr antestatal global",
                                         [&]
                                         {
                                             layout_types.insert(type);
                                             interp.add_constexpr_antestatal_global(symbol, type, value);
                                         });

                dependencies const& dependencies = co_await rpnx::querygraph::request< direct_dependencies_query >(
                    direct_dependencies_input{.symbol = symbol, .set = dependency_set::constexpr_});
                for (auto const& [dependency, _] : dependencies.functanoids)
                {
                    run_under_profiling_void("run_static_test antestatal functanoid loop body",
                                             [&]
                                             {
                                                 if (functanoids.insert(dependency).second)
                                                 {
                                                     functanoid_list.push_back(dependency);
                                                 }
                                             });
                }
                for (type_symbol const& dependency : dependencies.antestatal_globals)
                {
                    run_under_profiling_void("run_static_test antestatal global dependency loop body",
                                             [&]
                                             {
                                                 if (antestatal_globals.insert(dependency).second)
                                                 {
                                                     antestatal_global_list.push_back(dependency);
                                                 }
                                             });
                }
            }
        }

        run_under_profiling_void("run_static_test add root functanoid3",
                                 [&]
                                 {
                                     interp.add_functanoid3(void_type{}, *routine, root_dependencies.static_snapshots);
                                 });

        for (type_symbol const& root_type : layout_types)
        {
            std::vector< type_symbol > pending{root_type};

            while (!pending.empty())
            {
                type_symbol type = std::move(pending.back());
                pending.pop_back();

                run_under_profiling_void("run_static_test layout expansion loop body",
                                         [&]
                                         {
                                             if (type.type_is< nvalue_slot >())
                                             {
                                                 pending.push_back(type.get_as< nvalue_slot >().target);
                                             }
                                             else if (type.type_is< dvalue_slot >())
                                             {
                                                 pending.push_back(type.get_as< dvalue_slot >().target);
                                             }
                                             else if (type.type_is< ptrref_type >())
                                             {
                                                 pending.push_back(type.get_as< ptrref_type >().target);
                                             }
                                             else if (type.type_is< array_type >())
                                             {
                                                 pending.push_back(type.get_as< array_type >().element_type);
                                             }
                                             else if (type.type_is< storage >())
                                             {
                                                 for (type_symbol const& storable_type : type.get_as< storage >().storable_types)
                                                 {
                                                     pending.push_back(storable_type);
                                                 }
                                             }
                                         });

                if (!(type.type_is< subsymbol >() || type.type_is< subtag_type >() || type.type_is< instanciation_reference >() || type.type_is< readonly_constant >()) || loaded_layouts.contains(type))
                {
                    continue;
                }

                run_under_profiling_void("run_static_test loaded_layouts insert",
                                         [&]
                                         {
                                             loaded_layouts.insert(type);
                                         });
                class_kind const kind = co_await rpnx::querygraph::request< class_type_query >(type);
                if (kind == class_kind::enum_)
                {
                    enum_info const& info = co_await rpnx::querygraph::request< enum_info_query >(type);
                    run_under_profiling_void("run_static_test add nominal integer type enum",
                                             [&]
                                             {
                                                 interp.add_nominal_integer_type(type, info.bits);
                                             });
                    continue;
                }
                if (kind == class_kind::flagset)
                {
                    flagset_info const& info = co_await rpnx::querygraph::request< flagset_info_query >(type);
                    run_under_profiling_void("run_static_test add nominal integer type flagset",
                                             [&]
                                             {
                                                 interp.add_nominal_integer_type(type, info.bits);
                                             });
                    continue;
                }
                if (kind == class_kind::union_)
                {
                    union_info const& info = co_await rpnx::querygraph::request< union_info_query >(type);
                    fusion_layout const& layout = co_await rpnx::querygraph::request< fusion_layout_query >(type);
                    run_under_profiling_void("run_static_test add UNION fusion layout",
                                             [&]
                                             {
                                                 for (union_option_info const& option : info.options)
                                                 {
                                                     pending.push_back(option.type);
                                                 }
                                                 interp.add_union_info(type, info);
                                                 interp.add_fusion_layout(type, layout);
                                             });
                    continue;
                }
                if (kind == class_kind::variant)
                {
                    variant_info const& info = co_await rpnx::querygraph::request< variant_info_query >(type);
                    fusion_layout const& layout = co_await rpnx::querygraph::request< fusion_layout_query >(type);
                    run_under_profiling_void("run_static_test add VARIANT fusion layout",
                                             [&]
                                             {
                                                 for (type_symbol const& alternative : info.alternatives)
                                                 {
                                                     pending.push_back(alternative);
                                                 }
                                                 interp.add_variant_info(type, info);
                                                 interp.add_fusion_layout(type, layout);
                                             });
                    continue;
                }
                if (kind != class_kind::struct_)
                {
                    continue;
                }

                struct_layout const& layout = co_await rpnx::querygraph::request< struct_layout_query >(type);
                run_under_profiling_void("run_static_test add struct layout",
                                         [&]
                                         {
                                             for (auto const& field : layout.fields)
                                             {
                                                 pending.push_back(field.type);
                                             }
                                             interp.add_struct_layout(type, layout);
                                         });
            }
        }

        run_under_profiling_void([&] { return "run_static_test " + quxlang::to_string(input); },
                                 [&]
                                 {
                                     interp.exec3(void_type{});
                                 });
    }
    catch (constexpr_runtime_error const& error)
    {
        if (test.expected_mode == static_test_expected_mode::expect_fail)
        {
            co_return true;
        }
        throw static_test_failure_error(input, "executing", error.what());
    }
    catch (compiler_bug const& error)
    {
        throw static_test_compiler_bug(input, "executing", error);
    }
    catch (compilation_error const& error)
    {
        if (test.expected_mode == static_test_expected_mode::expect_compilation_failure)
        {
            co_return true;
        }
        throw static_test_failure_error(input, "executing", error);
    }
    catch (std::logic_error const& error)
    {
        if (test.expected_mode == static_test_expected_mode::expect_compilation_failure)
        {
            co_return true;
        }
        throw static_test_failure_error(input, "executing", error.what());
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
