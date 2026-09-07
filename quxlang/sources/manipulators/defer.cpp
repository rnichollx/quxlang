// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#include <quxlang/manipulators/defer.hpp>
#include <quxlang/data/compilation_result.hpp>

#include <algorithm>
#include <type_traits>

auto quxlang::normalize_defer_body(function_block body, std::optional< std::string > label_name) -> function_block
{
    std::vector< std::string > inner_labels;
    std::size_t runtime_loop_depth = 0;
    auto normalize_block = [&](auto&& self, function_block& block) -> void
    {
        for (function_statement& statement : block.statements)
        {
            bool exits_defer = false;
            rpnx::apply_visitor<void>(statement, [&](auto& node)
            {
                using statement_type = std::decay_t< decltype(node) >;
                if constexpr (std::is_same_v< statement_type, function_return_statement > || std::is_same_v< statement_type, function_return_unequal_statement >)
                {
                    compilation_error error = semantic_compilation_error("RETURN is not allowed in a DEFER body; use BREAK to exit the deferred action");
                    error.traceback.push_back(trace_frame{.trace_context = "deferred action", .location = node.location});
                    throw error;
                }
                else if constexpr (std::is_same_v< statement_type, function_break_statement >)
                {
                    if (node.label_name.has_value())
                    {
                        exits_defer = node.label_name == label_name && std::find(inner_labels.begin(), inner_labels.end(), *node.label_name) == inner_labels.end();
                    }
                    else
                    {
                        exits_defer = runtime_loop_depth == 0;
                    }
                }
                else if constexpr (std::is_same_v< statement_type, function_block >)
                {
                    self(self, node);
                }
                else if constexpr (std::is_same_v< statement_type, function_if_statement > || std::is_same_v< statement_type, function_static_if_statement > || std::is_same_v< statement_type, function_runtime_statement >)
                {
                    self(self, node.then_block);
                    if (node.else_block.has_value())
                    {
                        self(self, *node.else_block);
                    }
                }
                else if constexpr (std::is_same_v< statement_type, function_while_statement > || std::is_same_v< statement_type, function_loop_statement >)
                {
                    if constexpr (std::is_same_v< statement_type, function_loop_statement >)
                    {
                        for (std::optional< function_block >* clause : {&node.init_block, &node.eval_block, &node.step_block})
                        {
                            if (clause->has_value())
                            {
                                self(self, **clause);
                            }
                        }
                    }
                    ++runtime_loop_depth;
                    if (node.label_name.has_value())
                    {
                        inner_labels.push_back(*node.label_name);
                    }
                    self(self, node.loop_block);
                    if (node.label_name.has_value())
                    {
                        inner_labels.pop_back();
                    }
                    --runtime_loop_depth;
                }
                else if constexpr (std::is_same_v< statement_type, function_static_while_statement >)
                {
                    self(self, node.loop_block);
                }
                else if constexpr (std::is_same_v< statement_type, function_label_block_statement >)
                {
                    inner_labels.push_back(node.name);
                    self(self, node.block);
                    inner_labels.pop_back();
                }
                else if constexpr (std::is_same_v< statement_type, function_match_statement >)
                {
                    for (function_match_arm& arm : node.arms)
                    {
                        self(self, arm.block);
                    }
                    if (node.default_clause.has_value() && node.default_clause->block.has_value())
                    {
                        self(self, *node.default_clause->block);
                    }
                }
                else if constexpr (std::is_same_v< statement_type, function_visit_statement >)
                {
                    self(self, node.body);
                }
                else if constexpr (std::is_same_v< statement_type, function_try_statement >)
                {
                    self(self, node.body);
                    for (auto& handler : node.handlers)
                    {
                        rpnx::apply_visitor< void >(handler, [&](auto& clause) { self(self, clause.body); });
                    }
                }
                // Expressions and nested DEFER actions own independent callable boundaries.
            });
            if (exits_defer)
            {
                function_return_statement exit;
                exit.location = get_location(statement);
                statement = std::move(exit);
            }
        }
    };
    normalize_block(normalize_block, body);
    return body;
}

auto quxlang::defer_callable_expression(function_defer_statement const& statement) -> expression
{
    return rpnx::apply_visitor< expression >(statement.action, [&](const auto& action) -> expression
    {
        using action_type = std::decay_t< decltype(action) >;
        if constexpr (std::is_same_v< action_type, defer_call_action >)
        {
            return action.expr;
        }
        else
        {
            expression_lambda callable;
            callable.return_type = void_type{};
            callable.is_noexcept = true;
            callable.location = statement.location;
            if constexpr (std::is_same_v< action_type, defer_expression_action >)
            {
                function_expression_statement invocation;
                invocation.expr = action.expr;
                invocation.location = action.location;
                callable.body.statements.push_back(std::move(invocation));
                callable.body.location = action.location;
            }
            else
            {
                callable.body = normalize_defer_body(action.body, action.label_name);
            }
            return callable;
        }
    });
}
