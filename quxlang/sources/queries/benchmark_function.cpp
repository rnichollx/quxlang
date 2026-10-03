// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/specs/benchmark_function_spec.hpp>

#include <algorithm>
#include <type_traits>

namespace quxlang
{
    namespace
    {
        /** Refers to a benchmark-local value without interpreting source text. */
        auto benchmark_local(std::string name) -> expression
        {
            return expression_symbol_reference{.symbol = freebound_identifier{.name = std::move(name)}};
        }

        /** Constructs a compiler-owned call into the runtime module. */
        auto benchmark_runtime_call(std::string name, std::vector< expression_arg > arguments = {}) -> expression
        {
            return expression_call{
                .callee = expression_symbol_reference{.symbol = subsymbol{.of = absolute_module_reference{.module_name = "RUNTIME"}, .name = std::move(name)}},
                .args = std::move(arguments),
            };
        }

        /** Constructs a runtime object method invocation with uppercase protocol arguments. */
        auto benchmark_method(std::string object, std::string method, std::vector< expression_arg > arguments = {}, std::vector< expression_arg > template_arguments = {}) -> expression
        {
            return expression_call{
                .callee = expression_dotreference{.lhs = benchmark_local(std::move(object)), .field_name = std::move(method), .template_arguments = std::move(template_arguments)},
                .args = std::move(arguments),
            };
        }

        /** Normalizes benchmark statements into ordinary lexical blocks and runtime calls. */
        class benchmark_block_lowering
        {
          public:
            /** Retains the declaration contract and the output-specific measurement mode. */
            benchmark_block_lowering(ast2_benchmark const& declaration, bool standalone) : declaration_(declaration), standalone_(standalone)
            {
            }

            /** Lowers one block while preserving scope and checking region-crossing transfers. */
            auto lower(function_block block, bool outer = false, bool in_loop = false, bool in_measurement = false, std::vector< std::optional< std::string > > loops = {}, bool loop_completed = false) -> function_block
            {
                for (function_statement& statement : block.statements)
                {
                    bool const completes_loop = statement.type_is< function_benchmark_loop >();
                    auto lower_statement = [&](auto value) -> function_statement
                    {
                        using statement_type = std::decay_t< decltype(value) >;
                        if constexpr (std::is_same_v< statement_type, function_benchmark_loop >)
                        {
                            if (!outer || declaration_.oneshot || in_loop)
                            {
                                throw semantic_compilation_error("BENCHMARK_LOOP must occur exactly once directly in BODY");
                            }
                            function_block result;
                            result.statements.push_back(function_var_statement{.name = "__BENCHMARK_ITERATION", .type = int_type{.bits = 64, .has_sign = false}});
                            function_loop_statement loop;
                            loop.test_condition = expression_binary{.operator_str = "<", .lhs = benchmark_local("__BENCHMARK_ITERATION"), .rhs = expression_dotreference{.lhs = benchmark_local("__BENCHMARK_STATE"), .field_name = "ITERATIONS"}};
                            loop.step_block = function_block{.statements = {function_expression_statement{.expr = expression_unary_postfix{.operator_str = "++", .lhs = benchmark_local("__BENCHMARK_ITERATION")}}}};
                            loop.loop_block = lower(std::move(value.body), false, true);
                            result.statements.push_back(std::move(loop));
                            return result;
                        }
                        else if constexpr (std::is_same_v< statement_type, function_measure_statement >)
                        {
                            if (in_measurement)
                            {
                                throw semantic_compilation_error("MEASURE regions cannot be nested");
                            }
                            if (!declaration_.oneshot && !in_loop)
                            {
                                throw semantic_compilation_error("BODY measurements must occur inside BENCHMARK_LOOP");
                            }
                            std::size_t channel = 0;
                            if (value.name.has_value())
                            {
                                std::vector< std::string >::const_iterator found = std::find(declaration_.measurements.begin(), declaration_.measurements.end(), *value.name);
                                if (found == declaration_.measurements.end())
                                {
                                    throw semantic_compilation_error("Unknown benchmark measurement: " + *value.name);
                                }
                                channel = static_cast< std::size_t >(found - declaration_.measurements.begin());
                            }
                            else if (declaration_.measurements.size() != 1)
                            {
                                throw semantic_compilation_error("MEASURE requires a name when the benchmark has multiple measurements");
                            }
                            function_block measured = lower(std::move(value.body), false, in_loop, true);
                            if (standalone_)
                            {
                                return measured;
                            }
                            function_block result;
                            result.statements.push_back(function_var_statement{
                                .name = "__BENCHMARK_COUNT",
                                .type = int_type{.bits = 64, .has_sign = false},
                                .equals_initializer = benchmark_runtime_call("BENCHMARK_OPERATION_COUNT", {{.name = "VALUE", .value = value.count.value_or(expression_numeric_literal{.value = "1"})}}),
                            });
                            result.statements.push_back(function_var_statement{
                                .name = "__BENCHMARK_START",
                                .type = int_type{.bits = 64, .has_sign = false},
                                .equals_initializer = benchmark_runtime_call("BENCHMARK_CLOCK"),
                            });
                            result.statements.push_back(std::move(measured));
                            result.statements.push_back(function_expression_statement{.expr = benchmark_runtime_call("BENCHMARK_RECORD", {
                                                                                                                                             {.name = "START", .value = benchmark_local("__BENCHMARK_START")},
                                                                                                                                             {.name = "COUNT", .value = benchmark_local("__BENCHMARK_COUNT")},
                                                                                                                                             {.name = "CHANNEL", .value = expression_numeric_literal{.value = std::to_string(channel)}},
                                                                                                                                             {.name = "STATE", .value = benchmark_local("__BENCHMARK_STATE")},
                                                                                                                                         })});
                            return result;
                        }
                        else if constexpr (std::is_same_v< statement_type, function_return_statement >)
                        {
                            if (in_measurement || (!declaration_.oneshot && !loop_completed))
                            {
                                throw semantic_compilation_error("Benchmark control flow cannot return before completing BENCHMARK_LOOP or leave MEASURE");
                            }
                            if (value.expr.has_value())
                            {
                                throw semantic_compilation_error("BENCHMARK cannot return a value");
                            }
                            if (standalone_)
                            {
                                value.expr = expression_numeric_literal{.value = "0"};
                            }
                        }
                        else if constexpr (std::is_same_v< statement_type, function_return_unequal_statement > || std::is_same_v< statement_type, function_goto_statement >)
                        {
                            throw semantic_compilation_error("Benchmark control flow cannot jump across execution regions");
                        }
                        else if constexpr (std::is_same_v< statement_type, function_break_statement > || std::is_same_v< statement_type, function_continue_statement >)
                        {
                            if (loops.empty() || (value.label_name.has_value() && std::find(loops.begin(), loops.end(), value.label_name) == loops.end()))
                            {
                                throw semantic_compilation_error("Benchmark control flow cannot leave a measurement or terminate BENCHMARK_LOOP");
                            }
                        }
                        else if constexpr (std::is_same_v< statement_type, function_block >)
                        {
                            return lower(std::move(value), false, in_loop, in_measurement, loops, loop_completed);
                        }
                        else if constexpr (std::is_same_v< statement_type, function_loop_statement >)
                        {
                            if (value.init_block)
                            {
                                value.init_block = lower(std::move(*value.init_block), false, in_loop, in_measurement, loops, loop_completed);
                            }
                            loops.push_back(value.label_name);
                            if (value.eval_block)
                            {
                                value.eval_block = lower(std::move(*value.eval_block), false, in_loop, in_measurement, loops, loop_completed);
                            }
                            if (value.step_block)
                            {
                                value.step_block = lower(std::move(*value.step_block), false, in_loop, in_measurement, loops, loop_completed);
                            }
                            value.loop_block = lower(std::move(value.loop_block), false, in_loop, in_measurement, loops, loop_completed);
                            loops.pop_back();
                        }
                        else if constexpr (std::is_same_v< statement_type, function_static_while_statement >)
                        {
                            loops.push_back(std::nullopt);
                            value.loop_block = lower(std::move(value.loop_block), false, in_loop, in_measurement, loops, loop_completed);
                            loops.pop_back();
                        }
                        else if constexpr (requires {
                                               value.then_block;
                                               value.else_block;
                                           })
                        {
                            value.then_block = lower(std::move(value.then_block), false, in_loop, in_measurement, loops, loop_completed);
                            if (value.else_block)
                            {
                                value.else_block = lower(std::move(*value.else_block), false, in_loop, in_measurement, loops, loop_completed);
                            }
                        }
                        else if constexpr (std::is_same_v< statement_type, function_try_statement >)
                        {
                            value.body = lower(std::move(value.body), false, in_loop, in_measurement, loops, loop_completed);
                            for (auto& handler : value.handlers)
                            {
                                rpnx::apply_visitor< void >(handler,
                                                            [&](auto& clause)
                                                            {
                                                                clause.body = lower(std::move(clause.body), false, in_loop, in_measurement, loops, loop_completed);
                                                            });
                            }
                        }
                        else if constexpr (std::is_same_v< statement_type, function_visit_statement >)
                        {
                            value.body = lower(std::move(value.body), false, in_loop, in_measurement, loops, loop_completed);
                        }
                        else if constexpr (std::is_same_v< statement_type, function_label_block_statement >)
                        {
                            value.block = lower(std::move(value.block), false, in_loop, in_measurement, loops, loop_completed);
                        }
                        else if constexpr (std::is_same_v< statement_type, function_match_statement >)
                        {
                            for (function_match_arm& arm : value.arms)
                            {
                                arm.block = lower(std::move(arm.block), false, in_loop, in_measurement, loops, loop_completed);
                            }
                            if (value.default_clause && value.default_clause->block)
                            {
                                value.default_clause->block = lower(std::move(*value.default_clause->block), false, in_loop, in_measurement, loops, loop_completed);
                            }
                        }
                        else if constexpr (std::is_same_v< statement_type, function_defer_statement >)
                        {
                            if (value.action.template type_is< defer_block_action >())
                            {
                                defer_block_action& action = value.action.template as< defer_block_action >();
                                action.body = lower(std::move(action.body), false, in_loop, in_measurement);
                            }
                        }
                        return value;
                    };
                    statement = rpnx::apply_visitor< function_statement >(statement, lower_statement);
                    loop_completed = loop_completed || completes_loop;
                }
                return block;
            }

          private:
            ast2_benchmark const& declaration_;
            bool standalone_;
        };
    } // namespace
} // namespace quxlang

rpnx::querygraph::coroutine< quxlang::benchmark_function_spec > quxlang::benchmark_function_impl(benchmark_function_input input)
{
    ast2_symboid const& symbol = co_await rpnx::querygraph::request< symboid_query >(input.benchmark);
    if (!symbol.type_is< ast2_benchmark >())
    {
        throw semantic_compilation_error("Expected a BENCHMARK declaration: " + to_string(input.benchmark));
    }
    ast2_benchmark const& declaration = symbol.get_as< ast2_benchmark >();
    if (input.standalone && !declaration.oneshot)
    {
        throw semantic_compilation_error("Standalone benchmark compilation requires ONESHOT: " + to_string(input.benchmark));
    }
    ast2_function_declaration result;
    result.location = declaration.location;
    result.definition.return_type = input.standalone ? type_symbol(int_type{.bits = 32, .has_sign = true}) : type_symbol(void_type{});
    function_block& body = result.definition.body;
    type_symbol runtime = absolute_module_reference{.module_name = "RUNTIME"};
    if (input.standalone)
    {
        body.statements.push_back(function_var_statement{.name = "__BENCHMARK_ARGUMENTS", .type = subsymbol{.of = runtime, .name = "BENCHMARK_CLI_ARGUMENTS"}});
        for (ast2_function_parameter const& parameter : declaration.parameters)
        {
            body.statements.push_back(function_expression_statement{.expr = benchmark_method("__BENCHMARK_ARGUMENTS", "REGISTER", {{.name = "NAME", .value = expression_string_literal{.value = *parameter.api_name}}})});
        }
        body.statements.push_back(function_expression_statement{.expr = benchmark_method("__BENCHMARK_ARGUMENTS", "VALIDATE")});
        body.statements.push_back(function_var_statement{.name = "__CLI_PARSER_OBJECT", .type = declaration.cli_parser.value_or(subsymbol{.of = runtime, .name = "DEFAULT_PARSER"})});
        for (std::size_t index = 0; index < declaration.parameters.size(); ++index)
        {
            ast2_function_parameter const& parameter = declaration.parameters[index];
            std::string const diagnostic = "Standalone benchmark " + to_string(input.benchmark) + " parameter @" + *parameter.api_name + " of type " + to_string(parameter.type) + " with CLI_PARSER(" + to_string(declaration.cli_parser.value_or(subsymbol{.of = runtime, .name = "DEFAULT_PARSER"})) + ")";
            try
            {
                std::optional< type_symbol > parser = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                    .context = input.benchmark,
                    .type = declaration.cli_parser.value_or(subsymbol{.of = runtime, .name = "DEFAULT_PARSER"}),
                });
                std::optional< type_symbol > parameter_type = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                    .context = input.benchmark,
                    .type = parameter.type,
                });
                if (!parser || !parameter_type)
                {
                    throw semantic_compilation_error("Cannot resolve the CLI parser or parameter type");
                }
                std::optional< type_symbol > method = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                    .context = input.benchmark,
                    .type =
                        initialization_reference{
                            .initializee = submember{.of = *parser, .name = "PARSE"},
                            .context = input.benchmark,
                            .arguments = {{.name = "T", .value = expression_symbol_reference{.symbol = *parameter_type}}},
                        },
                });
                if (!method)
                {
                    throw semantic_compilation_error("CLI parser requires PARSE#T(@INPUT CONST& std::string, @OUTPUT WRITE& T)");
                }
                type_symbol string_type = subsymbol{.of = absolute_module_reference{.module_name = "std"}, .name = "string"};
                std::optional< instanciation_reference > parse = co_await rpnx::querygraph::request< instanciation_query >(initialization_reference{
                    .initializee = *method,
                    .context = input.benchmark,
                    .parameters = instatype_from_invotype(invotype{.named =
                                                                       {
                                                                           {"THIS", make_mref(*parser)},
                                                                           {"INPUT", make_cref(string_type)},
                                                                           {"OUTPUT", make_mref(*parameter_type)},
                                                                       }}),
                });
                if (!parse)
                {
                    throw semantic_compilation_error("CLI parser has no compatible PARSE#T(@INPUT CONST& std::string, @OUTPUT WRITE& T)");
                }
                static_cast< void >(co_await rpnx::querygraph::request< vm_procedure3_query >(*parse));
            }
            catch (compilation_error& error)
            {
                error.traceback.push_back(trace_frame{.trace_context = diagnostic, .location = parameter.location});
                throw;
            }
            std::string name = parameter.name.value_or(*parameter.api_name);
            body.statements.push_back(function_var_statement{.name = name, .type = parameter.type});
            body.statements.push_back(function_expression_statement{.expr = benchmark_method("__CLI_PARSER_OBJECT", "PARSE",
                                                                                             {
                                                                                                 {.name = "INPUT", .value = benchmark_method("__BENCHMARK_ARGUMENTS", "VALUE", {{.name = "INDEX", .value = expression_numeric_literal{.value = std::to_string(index)}}})},
                                                                                                 {.name = "OUTPUT", .value = benchmark_local(name)},
                                                                                             },
                                                                                             {{.name = "T", .value = expression_symbol_reference{.symbol = parameter.type}}})});
        }
    }
    else
    {
        result.header.call_parameters = {{
            .name = "__BENCHMARK_STATE",
            .api_name = "STATE",
            .type = ptrref_type{.target = subsymbol{.of = runtime, .name = "BENCHMARK_STATE"}, .ptr_class = pointer_class::ref, .qual = qualifier::mut},
        }};
        if (declaration.cases.empty() && !declaration.parameters.empty())
        {
            throw semantic_compilation_error("Parameterized suite benchmark requires CASE entries: " + to_string(input.benchmark));
        }
        if (!declaration.cases.empty())
        {
            if (input.case_index >= declaration.cases.size())
            {
                throw compiler_bug("Benchmark case index exceeds declaration cases");
            }
            ast2_benchmark_case const& selected = declaration.cases[input.case_index];
            for (ast2_function_parameter const& parameter : declaration.parameters)
            {
                std::vector< expression_arg >::const_iterator argument = std::find_if(selected.arguments.begin(), selected.arguments.end(),
                                                                                      [&](expression_arg const& value)
                                                                                      {
                                                                                          return value.name == parameter.api_name;
                                                                                      });
                if (argument == selected.arguments.end())
                {
                    throw semantic_compilation_error("Missing benchmark case argument: " + *parameter.api_name);
                }
                std::string name = parameter.name.value_or(*parameter.api_name);
                std::string constant = "__BENCHMARK_ARGUMENT_" + std::to_string(body.statements.size());
                body.statements.push_back(function_var_statement{.name = constant, .type = parameter.type, .equals_initializer = argument->value, .static_kind = function_static_kind::constant});
                body.statements.push_back(function_var_statement{.name = name, .type = parameter.type, .equals_initializer = benchmark_local(constant)});
            }
        }
    }
    benchmark_block_lowering lowering(declaration, input.standalone);
    function_block workload = lowering.lower(declaration.body, true);
    body.statements.insert(body.statements.end(), workload.statements.begin(), workload.statements.end());
    if (input.standalone)
    {
        function_try_statement attempt{.body = std::move(body)};
        attempt.handlers.push_back(function_default_catch{.body = function_block{.statements = {function_return_statement{.expr = expression_numeric_literal{.value = "2"}}}}});
        body = function_block{.statements = {std::move(attempt), function_return_statement{.expr = expression_numeric_literal{.value = "0"}}}};
    }
    co_return result;
}
