// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <gtest/gtest.h>

#include <quxlang/compiler_querygraph.hpp>
#include <quxlang/parsers/parse_file.hpp>
#include <quxlang/source_loader.hpp>

#include "graph_dump_test_utils.hpp"

#include <cctype>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace
{
    std::string const static_test_target = "linux-arm64";
    std::string const static_test_suite_name = "Module_main_linux_arm64";

    auto main_static_test_module() -> quxlang::type_symbol
    {
        return quxlang::with_context(quxlang::context_reference{}, quxlang::absolute_module_reference{"main"});
    }

    auto load_static_test_sources() -> quxlang::source_bundle
    {
        std::filesystem::path testdata = QUXLANG_TESTS_TESTDDATA_PATH;
        return quxlang::load_bundle_sources_for_targets(testdata / "example", {});
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

            auto sources = load_static_test_sources();
            static_test_querygraph_compiler c(sources, static_test_target);
            EXPECT_TRUE(c.run_static_test(*m_test_symbol));
        }

      private:
        std::optional< quxlang::type_symbol > m_test_symbol;
        std::optional< std::string > m_discovery_error;
    };

    auto collect_static_test_symbols(quxlang::type_symbol context, std::vector< quxlang::subdeclaroid > const& declarations, std::set< quxlang::type_symbol >& output) -> void
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
                output.insert(child);
            }
            else if (decl->type_is< quxlang::ast2_class_declaration >())
            {
                collect_static_test_symbols(child, decl->get_as< quxlang::ast2_class_declaration >().declarations, output);
            }
            else if (decl->type_is< quxlang::ast2_namespace_declaration >())
            {
                collect_static_test_symbols(child, decl->get_as< quxlang::ast2_namespace_declaration >().declarations, output);
            }
        }
    }

    auto discover_main_static_tests() -> std::set< quxlang::type_symbol >
    {
        auto sources = load_static_test_sources();
        auto const& target = sources.targets.at(static_test_target);
        std::string const& source_module_name = target.module_configurations.at("main").source;
        auto const& main_sources = sources.module_sources.at(source_module_name);

        std::set< quxlang::type_symbol > output;
        for (auto const& [_, source_file] : main_sources.files)
        {
            std::string contents = source_file->contents;
            auto pos = contents.begin();
            auto file_ast = quxlang::parsers::parse_file(pos, contents.end());
            collect_static_test_symbols(main_static_test_module(), file_ast.declarations, output);
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
            auto tests = discover_main_static_tests();
            if (tests.empty())
            {
                register_main_static_test_discovery_failure("No STATIC_TEST declarations were discovered in the main module");
                return true;
            }

            std::set< std::string > registered_names;
            for (quxlang::type_symbol const& test : tests)
            {
                std::string base_name = main_static_test_gtest_name(test);
                std::string test_name = base_name;
                for (std::size_t suffix = 2; !registered_names.insert(test_name).second; ++suffix)
                {
                    test_name = base_name + "_" + std::to_string(suffix);
                }

                std::string value_param = quxlang::to_string(test);
                ::testing::RegisterTest(static_test_suite_name.c_str(), test_name.c_str(), nullptr, value_param.c_str(), __FILE__, __LINE__,
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
