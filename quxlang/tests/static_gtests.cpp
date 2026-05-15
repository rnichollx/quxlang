// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <gtest/gtest.h>

#include <quxlang/compiler_querygraph.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/parsers/parse_file.hpp>
#include <quxlang/queries/list_static_tests.hpp>
#include <quxlang/queries/run_static_test.hpp>
#include <quxlang/source_loader.hpp>

#include "graph_dump_test_utils.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    std::string const static_test_target = "linux-arm64";
    std::string const static_test_suite_name = "Module_main_linux_arm64";

    /// Identifies the source location GoogleTest should report for a discovered STATIC_TEST.
    struct static_test_gtest_location
    {
        std::filesystem::path file;
        int line = 1;
    };

    /// Stores the source location metadata found while statically discovering STATIC_TEST declarations.
    using static_test_discovery_map = std::map< quxlang::type_symbol, static_test_gtest_location >;

    /// Suppresses debug stdout while static gtest discovery loads the Quxlang source bundle.
    class scoped_static_test_source_loading_output_suppression
    {
      public:
        scoped_static_test_source_loading_output_suppression() : m_previous_buffer(std::cout.rdbuf(m_sink.rdbuf()))
        {
        }

        ~scoped_static_test_source_loading_output_suppression()
        {
            std::cout.rdbuf(m_previous_buffer);
        }

        scoped_static_test_source_loading_output_suppression(scoped_static_test_source_loading_output_suppression const&) = delete;
        auto operator=(scoped_static_test_source_loading_output_suppression const&) -> scoped_static_test_source_loading_output_suppression& = delete;

      private:
        std::ostringstream m_sink;
        std::streambuf* m_previous_buffer;
    };

    auto main_static_test_module() -> quxlang::type_symbol
    {
        return quxlang::with_context(quxlang::context_reference{}, quxlang::absolute_module_reference{"main"});
    }

    /// Returns the filesystem root corresponding to the loaded source bundle.
    auto static_test_source_root() -> std::filesystem::path
    {
        std::filesystem::path testdata = QUXLANG_TESTS_TESTDDATA_PATH;
        return std::filesystem::absolute(testdata / "example").lexically_normal();
    }

    auto load_static_test_sources() -> quxlang::source_bundle
    {
        scoped_static_test_source_loading_output_suppression const suppress_output;
        return quxlang::load_bundle_sources_for_targets(static_test_source_root(), {});
    }

    class static_test_querygraph_compiler
    {
      public:
        static_test_querygraph_compiler(quxlang::source_bundle const& sources, std::string target)
            : m_graph(sources, target, sources.targets.at(target).target_output_config, quxlang::tests::current_test_graph_dump_path())
        {
        }

        auto list_static_tests(quxlang::type_symbol input) const
        {
            return m_graph.make_request< quxlang::list_static_tests_query >(std::move(input));
        }

        auto run_static_test(quxlang::type_symbol input) const
        {
            return m_graph.make_request< quxlang::run_static_test_query >(std::move(input));
        }

      private:
        mutable quxlang::compiler_querygraph m_graph;
    };

    /// Owns the source bundle and compiler graph reused by all in-process static gtests.
    class static_test_execution_context
    {
      public:
        static_test_execution_context() : m_sources(load_static_test_sources()), m_compiler(m_sources, static_test_target)
        {
        }

        auto compiler() -> static_test_querygraph_compiler&
        {
            return m_compiler;
        }

      private:
        quxlang::source_bundle m_sources;
        static_test_querygraph_compiler m_compiler;
    };

    auto sanitize_gtest_name(std::string name) -> std::string
    {
        for (char& ch : name)
        {
            unsigned char const uch = static_cast< unsigned char >(ch);
            if (!std::isalnum(uch) && ch != '_')
            {
                ch = '_';
            }
        }
        if (name.empty() || !std::isalpha(static_cast< unsigned char >(name.front())))
        {
            name = "StaticTest_" + name;
        }
        return name;
    }

    /// Converts a byte offset within a source file to the one-based line number GoogleTest expects.
    auto static_test_line(std::string const& contents, std::size_t begin_index) -> int
    {
        std::size_t const capped_begin_index = std::min(begin_index, contents.size());
        int line = 1;
        for (std::size_t index = 0; index < capped_begin_index; ++index)
        {
            if (contents[index] == '\n')
            {
                ++line;
            }
        }
        return line;
    }

    /// Converts a source-bundle-relative path into the path GoogleTest should expose.
    auto static_test_file_path(std::filesystem::path const& source_root, std::string const& relative_path) -> std::filesystem::path
    {
        return (source_root / relative_path).lexically_normal();
    }

    class main_static_test_fixture : public ::testing::Test
    {
      public:
        explicit main_static_test_fixture(quxlang::type_symbol test_symbol) : m_test_symbol(std::move(test_symbol))
        {
        }

        explicit main_static_test_fixture(std::string discovery_error) : m_discovery_error(std::move(discovery_error))
        {
        }

        void TestBody() override
        {
            if (m_discovery_error.has_value())
            {
                FAIL() << *m_discovery_error;
                return;
            }

            EXPECT_TRUE(static_context().compiler().run_static_test(*m_test_symbol));
        }

      private:
        /// Returns the fixture-owned context shared by all registered static tests in this process.
        auto static_context() -> static_test_execution_context&
        {
            static static_test_execution_context context;
            return context;
        }

        //static_test_execution_context context;
        std::optional< quxlang::type_symbol > m_test_symbol;
        std::optional< std::string > m_discovery_error;
    };

    /// Recursively discovers STATIC_TEST declarations and records the source location for each test symbol.
    auto collect_static_test_symbols(quxlang::type_symbol context, std::vector< quxlang::subdeclaroid > const& declarations, std::filesystem::path const& source_file_path,
                                     std::string const& contents, static_test_discovery_map& output) -> void
    {
        for (quxlang::subdeclaroid const& declaration : declarations)
        {
            quxlang::declaroid const* decl = nullptr;
            std::string const* name = nullptr;
            bool is_member = false;

            if (declaration.type_is< quxlang::global_subdeclaroid >())
            {
                auto const& global = declaration.get_as< quxlang::global_subdeclaroid >();
                decl = &global.decl;
                name = &global.name;
            }
            else if (declaration.type_is< quxlang::member_subdeclaroid >())
            {
                auto const& member = declaration.get_as< quxlang::member_subdeclaroid >();
                decl = &member.decl;
                name = &member.name;
                is_member = true;
            }
            else
            {
                continue;
            }

            quxlang::type_symbol child = is_member ? quxlang::type_symbol{quxlang::submember{.of = context, .name = *name}} : quxlang::type_symbol{quxlang::subsymbol{.of = context, .name = *name}};
            if (decl->type_is< quxlang::ast2_static_test >())
            {
                quxlang::ast2_static_test const& static_test = decl->get_as< quxlang::ast2_static_test >();
                int line = 1;
                if (static_test.location.has_value())
                {
                    line = static_test_line(contents, static_test.location->begin_index);
                }
                output.emplace(std::move(child), static_test_gtest_location{.file = source_file_path, .line = line});
            }
            else if (decl->type_is< quxlang::ast2_class_declaration >())
            {
                collect_static_test_symbols(child, decl->get_as< quxlang::ast2_class_declaration >().declarations, source_file_path, contents, output);
            }
            else if (decl->type_is< quxlang::ast2_namespace_declaration >())
            {
                collect_static_test_symbols(child, decl->get_as< quxlang::ast2_namespace_declaration >().declarations, source_file_path, contents, output);
            }
        }
    }

    /// Discovers main-module STATIC_TEST declarations using located parsing contexts.
    auto discover_main_static_tests() -> static_test_discovery_map
    {
        quxlang::source_bundle sources = load_static_test_sources();
        quxlang::target_configuration const& target = sources.targets.at(static_test_target);
        std::string const& source_module_name = target.module_configurations.at("main").source;
        quxlang::module_source const& main_sources = sources.module_sources.at(source_module_name);
        std::filesystem::path source_root = static_test_source_root();

        static_test_discovery_map output;
        std::uint64_t file_id = 0;
        for (auto const& [source_file_name, source_file] : main_sources.files)
        {
            std::string contents = source_file->contents;
            quxlang::parsers::parsing_context ctx{
                .file_id = file_id,
                .source_locations_enabled = true,
                .iter_begin = contents.begin(),
                .iter_pos = contents.begin(),
                .iter_end = contents.end(),
            };
            quxlang::ast2_file_declaration file_ast = quxlang::parsers::parse_file(ctx);
            collect_static_test_symbols(main_static_test_module(), file_ast.declarations, static_test_file_path(source_root, source_file_name), contents, output);
            ++file_id;
        }

        return output;
    }

    auto main_static_test_gtest_name(quxlang::type_symbol const& symbol) -> std::string
    {
        std::string name = quxlang::to_string(symbol);
        std::string const module_prefix = "MODULE(main)::";
        if (name.starts_with(module_prefix))
        {
            name.erase(0, module_prefix.size());
        }
        return sanitize_gtest_name(std::move(name));
    }

    auto register_main_static_test_discovery_failure(std::string message) -> void
    {
        ::testing::RegisterTest(static_test_suite_name.c_str(), "StaticTestDiscoveryFailed", nullptr, message.c_str(), __FILE__, __LINE__,
                                [message = std::move(message)]() -> main_static_test_fixture*
                                {
                                    return new main_static_test_fixture(message);
                                });
    }

    auto register_main_static_tests() -> bool
    {
        try
        {
            static_test_discovery_map tests = discover_main_static_tests();
            if (tests.empty())
            {
                register_main_static_test_discovery_failure("No STATIC_TEST declarations were discovered in the main module");
                return true;
            }

            std::set< std::string > registered_names;
            for (std::pair< quxlang::type_symbol const, static_test_gtest_location > const& test_entry : tests)
            {
                quxlang::type_symbol const& test = test_entry.first;
                static_test_gtest_location const& location = test_entry.second;
                std::string base_name = main_static_test_gtest_name(test);
                std::string test_name = base_name;
                for (std::size_t suffix = 2; !registered_names.insert(test_name).second; ++suffix)
                {
                    test_name = base_name + "_" + std::to_string(suffix);
                }

                std::string source_file = location.file.string();
                ::testing::RegisterTest(static_test_suite_name.c_str(), test_name.c_str(), nullptr, nullptr, source_file.c_str(), location.line,
                                        [test]() -> main_static_test_fixture*
                                        {
                                            return new main_static_test_fixture(test);
                                        });
            }
        }
        catch (std::exception const& err)
        {
            register_main_static_test_discovery_failure(err.what());
        }
        catch (...)
        {
            register_main_static_test_discovery_failure("Unknown error while discovering STATIC_TEST declarations");
        }
        return true;
    }

    [[maybe_unused]] bool const registered_main_static_tests = register_main_static_tests();
} // namespace
