// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/lookup_spec.hpp>

#include <algorithm>
#include <quxlang/cpu_attributes.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/data/constexpr_types.hpp>
#include <quxlang/macros.hpp>
#include <quxlang/manipulators/typeutils.hpp>

namespace quxlang::detail
{
    auto evaluate_u64_type_expression(type_symbol context, expression expr) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::uint64_t >
    {
        constexpr_input input;
        input.context = context;
        input.expr = std::move(expr);
        co_return co_await rpnx::querygraph::request< constexpr_u64_query >(std::move(input));
    }

    auto require_accessible_declaration(contextual_type_reference const& input, type_symbol const& selected_declaration) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< void >
    {
        bool accessible = co_await rpnx::querygraph::request< declaration_is_accessible_query >(declaration_access_request{
            .accessor_context = input.context,
            .selected_declaration = selected_declaration,
        });
        if (!accessible)
        {
            throw semantic_compilation_error("Lookup of " + to_string(selected_declaration) + " is inaccessible in context " + to_string(input.context));
        }
        co_return;
    }

    auto expand_alias(contextual_type_reference const& input, type_symbol const& selected_declaration) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        ast2_symboid const& declaration = co_await rpnx::querygraph::request< symboid_query >(selected_declaration);
        if (!declaration.type_is< ast2_alias_declaration >())
        {
            co_return selected_declaration;
        }

        type_symbol declaration_context = selected_declaration.type_is< instanciation_reference >() ? selected_declaration : type_parent(selected_declaration).value();
        contextual_type_reference target_reference{
            .context = std::move(declaration_context),
            .type = declaration.get_as< ast2_alias_declaration >().target,
        };
        // A self-request would attempt to lock the same dependency node twice.
        if (target_reference == input)
        {
            throw rpnx::querygraph::recursive_dependency_error();
        }
        std::optional< type_symbol > target = co_await rpnx::querygraph::request< lookup_query >(std::move(target_reference));
        // Qualified lookup can return a symbolic path whose final declaration is absent.
        if (target.has_value() && (target->type_is< subsymbol >() || target->type_is< submember >()) && !(co_await rpnx::querygraph::request< exists_query >(*target)))
        {
            co_return std::nullopt;
        }
        co_return target;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, composite_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        composite_type result;
        for (std::pair< std::string const, type_symbol > const& field : type.fields)
        {
            std::optional< type_symbol > resolved = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = input.context, .type = field.second});
            if (!resolved.has_value())
            {
                co_return std::nullopt;
            }
            result.fields.emplace(field.first, std::move(*resolved));
        }
        co_return result;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, composite_field_type_ref const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        std::optional< type_symbol > subject = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = input.context, .type = type.composite});
        if (!subject.has_value())
        {
            co_return std::nullopt;
        }
        type_symbol record = remove_ref(*subject);
        if (!record.type_is< composite_type >())
        {
            throw semantic_compilation_error("COMPOSITE_FIELD_TYPE requires a composite type");
        }
        composite_type const& schema = record.get_as< composite_type >();
        constexpr_result_v3 selection = co_await rpnx::querygraph::request< constexpr_eval_v3_query >(constexpr_input_v3{
            .expr = type.selector, .context = input.context, .expected_result_type = type_symbol(auto_temploidic{}),
        });
        auto selected = selection.values.find(constexpr_primary_result_id);
        if (selected != selection.values.end() && selected->second.type_is< constexpr_string >())
        {
            std::string name;
            for (std::byte byte : selected->second.get_as< constexpr_string >().bytes)
            {
                name.push_back(static_cast< char >(byte));
            }
            auto field = schema.fields.find(name);
            if (field == schema.fields.end())
            {
                throw semantic_compilation_error("Unknown composite field " + name);
            }
            co_return field->second;
        }
        std::uint64_t index = co_await evaluate_u64_type_expression(input.context, type.selector);
        if (index >= schema.fields.size())
        {
            throw semantic_compilation_error("Composite field index out of range");
        }
        auto field = schema.fields.begin();
        std::advance(field, index);
        co_return field->second;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, public_field_type_ref const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        std::optional< type_symbol > subject = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = input.context, .type = type.subject_type});
        if (!subject.has_value())
        {
            co_return std::nullopt;
        }
        type_symbol owner = remove_ref(*subject);
        std::vector< struct_field_declaration > const& fields = co_await rpnx::querygraph::request< public_struct_field_declaration_list_query >(owner);
        constexpr_result_v3 selection = co_await rpnx::querygraph::request< constexpr_eval_v3_query >(constexpr_input_v3{
            .expr = type.selector, .context = input.context, .expected_result_type = type_symbol(auto_temploidic{}),
        });
        type_symbol selector_type = remove_ref(selection.deduced_type.value());
        bool string_selector = selector_type.type_is< string_literal_type >() ||
            (selector_type.type_is< readonly_constant >() && selector_type.get_as< readonly_constant >().kind == constant_kind::string);
        if (!string_selector)
        {
            string_selector = co_await rpnx::querygraph::request< type_is_stringlike_query >(selector_type);
        }
        std::size_t index;
        if (string_selector)
        {
            constexpr_result_v3 selected = co_await rpnx::querygraph::request< constexpr_eval_v3_query >(constexpr_input_v3{
                .expr = type.selector, .context = input.context, .expected_result_type = type_symbol(readonly_constant{.kind = constant_kind::string}),
            });
            std::string name;
            for (std::byte byte : constexpr_value_as_string(selected.values.at(constexpr_primary_result_id)).bytes)
            {
                name.push_back(static_cast< char >(byte));
            }
            std::vector< struct_field_declaration >::const_iterator field = std::find_if(fields.begin(), fields.end(), [&](struct_field_declaration const& candidate) { return candidate.name == name; });
            if (field == fields.end())
            {
                throw semantic_compilation_error("Unknown public field " + name);
            }
            index = static_cast< std::size_t >(field - fields.begin());
        }
        else
        {
            std::uint64_t ordinal = co_await evaluate_u64_type_expression(input.context, type.selector);
            if (ordinal >= fields.size())
            {
                throw semantic_compilation_error("Public field index out of range");
            }
            index = static_cast< std::size_t >(ordinal);
        }
        co_return co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = owner, .type = fields.at(index).type});
    }

    auto lookup_impl_overloads(contextual_type_reference const&, void_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const&, null_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const&, byte_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const&, initguard_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const&, initguard_lock_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const&, constexpr_proxy const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, freebound_identifier const& fb) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        std::optional< type_symbol > current_context = input.context;
        assert(current_context.has_value());
        assert(!type_is_contextual(current_context.value()));

        while (current_context.has_value())
        {
            subsymbol sub2{current_context.value(), fb.name};
            auto exists = co_await rpnx::querygraph::request< exists_query >(sub2);

            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                co_yield rpnx::querygraph::debug_message("Exists? {}: {}", to_string(sub2), exists ? "yes" : "no");
            }

            if (exists)
            {
                if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
                {
                    co_yield rpnx::querygraph::debug_message("Found '{}' in context {}", fb.name, quxlang::to_string(current_context.value()));
                }
                co_await require_accessible_declaration(input, sub2);
                co_return co_await expand_alias(input, sub2);
            }

            subtag_type tag{current_context.value(), fb.name};
            auto tag_binding = co_await rpnx::querygraph::request< subtag_binding_query >(tag);
            if (tag_binding.has_value())
            {
                if (tag_binding->template type_is< parameter_type_instantiation >())
                {
                    co_return tag_binding->template get_as< parameter_type_instantiation >().type;
                }
                co_return tag;
            }

            if (current_context.value().type_is< absolute_module_reference >())
            {
                ast2_module_declaration const& module_ast = co_await rpnx::querygraph::request< module_ast_query >(as< absolute_module_reference >(current_context.value()).module_name);

                auto import_at = module_ast.imports.find(fb.name);

                if (import_at != module_ast.imports.end())
                {
                    co_return absolute_module_reference{import_at->second};
                }
            }

            current_context = type_parent(current_context.value());
            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                co_yield rpnx::querygraph::debug_message("New context: {}", quxlang::to_string(current_context.value_or(void_type{})));
            }
        }

        auto builtin_kind = co_await rpnx::querygraph::request< symbol_type_query >(builtin_symbol{fb.name});
        if (builtin_kind == symbol_kind::templex || builtin_kind == symbol_kind::functum || builtin_kind == symbol_kind::class_ || builtin_kind == symbol_kind::interface_)
        {
            co_return builtin_symbol{fb.name};
        }

        if (fb.name == "MAIN_FUNCTION_ARRAY")
        {
            co_return builtin_symbol{.name = "MAIN_FUNCTION_ARRAY"};
        }
        if (fb.name == "POST_DETECT_FUNCTION_ARRAY")
        {
            co_return builtin_symbol{.name = "POST_DETECT_FUNCTION_ARRAY"};
        }
        if (fb.name == "STEPPING_COUNT" || fb.name == "ACTIVE_STEPPING" || is_cpu_attribute_enabled_name(fb.name))
        {
            co_return builtin_symbol{.name = fb.name};
        }
        if (fb.name == "UNIT_TEST_COUNT" || fb.name == "UNIT_TEST_NAMES" || fb.name == "UNIT_TEST_PROC")
        {
            co_return builtin_symbol{.name = fb.name};
        }

        std::string str = "Could not find '" + fb.name + "'";
        co_return std::nullopt;
    }

    auto lookup_impl_overloads(contextual_type_reference const&, builtin_symbol const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, context_reference const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        std::string str = std::string() + "unimplemented: " + input.type.type().name();
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            co_yield rpnx::querygraph::debug_message("{}", str);
        }
        throw quxlang::semantic_compilation_error(str);
    }

    auto lookup_impl_overloads(contextual_type_reference const&, auto_temploidic const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const&, decay_temploidic const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const&, type_temploidic const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const&, absolute_module_reference const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, subsymbol const& sub) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        type_symbol const& parent = sub.of;

        if (parent.template type_is< context_reference >())
        {
            type_symbol current_module = get_root_module(input.context).value_or(void_type{});
            auto rval = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = input.context, .type = subsymbol{current_module, sub.name}});
            assert(!type_is_contextual(rval.value_or(void_type{})));
            co_return rval;
        }

        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            co_yield rpnx::querygraph::debug_message("Parent: {}", to_string(parent));
        }

        auto parent_canonical_opt = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = input.context, .type = parent});
        if (!parent_canonical_opt.has_value())
        {
            std::string str = "Could not find '" + sub.name + "' in context " + quxlang::to_string(input.context);
            co_return std::nullopt;
        }

        auto parent_canonical = parent_canonical_opt.value();

        auto parent_canonical_str = to_string(parent_canonical);
        assert(!type_is_contextual(parent_canonical));
        type_symbol selected_declaration = subsymbol{parent_canonical, sub.name};
        co_await require_accessible_declaration(input, selected_declaration);
        co_return co_await expand_alias(input, selected_declaration);
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, subtag_type const& sub) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        type_symbol const& parent = sub.of;

        auto parent_canonical_opt = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = input.context, .type = parent});
        if (!parent_canonical_opt.has_value())
        {
            co_return std::nullopt;
        }

        subtag_type canonical{.of = parent_canonical_opt.value(), .name = sub.name};
        auto binding = co_await rpnx::querygraph::request< subtag_binding_query >(canonical);
        if (!binding.has_value())
        {
            co_return std::nullopt;
        }
        if (binding->template type_is< parameter_type_instantiation >())
        {
            co_return binding->template get_as< parameter_type_instantiation >().type;
        }

        assert(!type_is_contextual(canonical.of));
        co_return canonical;
    }

    auto lookup_impl_overloads(contextual_type_reference const&, int_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        assert(!type_is_contextual(type));
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const&, float_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const&, bool_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, initialization_reference const& param_set) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        initialization_reference output;
        output.adaptations = param_set.adaptations;
        output.context = param_set.context.value_or(input.context);
        output.arguments = param_set.arguments;
        output.parameters = param_set.parameters;

        auto callee_canonical = co_await rpnx::querygraph::request< lookup_query >({
            .context = input.context,
            .type = param_set.initializee,
        });
        if (!callee_canonical.has_value())
        {
            co_return std::nullopt;
        }

        output.initializee = callee_canonical.value();

        auto initializee_kind = co_await rpnx::querygraph::request< symbol_type_query >(output.initializee);
        if (initializee_kind == symbol_kind::templex || initializee_kind == symbol_kind::template_)
        {
            auto inst = co_await rpnx::querygraph::request< instanciation_query >(output);
            if (!inst.has_value())
            {
                co_return std::nullopt;
            }
            co_return co_await expand_alias(input, *inst);
        }

        assert(!type_is_contextual(output));

        co_return output;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, instanciation_reference const& inst) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        if (type_is_contextual(inst.temploid.templexoid))
        {
            throw compiler_bug("lookup received an instanciation_reference with an unresolved templexoid");
        }
        co_return co_await expand_alias(input, input.type);
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, temploid_reference const& selection) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        temploid_reference output = selection;

        auto templexoid_canonical = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
            .context = input.context,
            .type = selection.templexoid,
        });
        if (!templexoid_canonical.has_value())
        {
            co_return std::nullopt;
        }
        output.templexoid = *templexoid_canonical;

        assert(!type_is_contextual(output));
        co_await require_accessible_declaration(input, output);
        co_return output;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, value_expression_reference const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        std::string str = std::string() + "unimplemented: " + input.type.type().name();
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            co_yield rpnx::querygraph::debug_message("{}", str);
        }
        throw quxlang::semantic_compilation_error(str);
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, submember const& sub) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        type_symbol const& parent = sub.of;

        if (parent.template type_is< context_reference >())
        {
            std::optional< type_symbol > current_context = input.context;
            assert(current_context.has_value());
            assert(!type_is_contextual(current_context.value()));

            while (current_context.has_value())
            {
                submember sub2{current_context.value(), sub.name};

                auto kind = co_await rpnx::querygraph::request< symbol_type_query >(sub2);
                if (kind == symbol_kind::class_)
                {
                    break;
                }
                current_context = type_parent(current_context.value());
            }

            if (current_context.has_value())
            {
                bool exists = co_await rpnx::querygraph::request< exists_query >(submember{current_context.value(), sub.name});
                if (exists)
                {
                    type_symbol selected_declaration = submember{current_context.value(), sub.name};
                    co_await require_accessible_declaration(input, selected_declaration);
                    co_return selected_declaration;
                }
                else
                {
                    std::string str = "Could not find '" + sub.name + "' in context " + quxlang::to_string(current_context.value());
                    co_return std::nullopt;
                }
            }

            std::string str = "Could not find '" + sub.name + "'";
            co_return std::nullopt;
        }

        std::optional< type_symbol > parent_canonical = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = input.context, .type = parent});
        if (!parent_canonical.has_value())
        {
            co_return std::nullopt;
        }

        assert(!type_is_contextual(*parent_canonical));
        type_symbol selected_declaration = submember{*parent_canonical, sub.name};
        co_await require_accessible_declaration(input, selected_declaration);
        co_return selected_declaration;
    }

    auto lookup_impl_overloads(contextual_type_reference const&, thistype const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, procedure_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        procedure_type canonical_proc = type;

        for (auto& [name, arg_type] : canonical_proc.signature.params.named)
        {
            auto canonical_arg = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = input.context, .type = arg_type});
            if (!canonical_arg.has_value())
            {
                co_return std::nullopt;
            }
            arg_type = canonical_arg.value();
        }

        for (auto& arg_type : canonical_proc.signature.params.positional)
        {
            auto canonical_arg = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = input.context, .type = arg_type});
            if (!canonical_arg.has_value())
            {
                co_return std::nullopt;
            }
            arg_type = canonical_arg.value();
        }

        if (canonical_proc.signature.return_type.has_value())
        {
            auto canonical_ret = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = input.context, .type = *canonical_proc.signature.return_type});
            if (!canonical_ret.has_value())
            {
                co_return std::nullopt;
            }
            canonical_proc.signature.return_type = canonical_ret.value();
        }

        co_return canonical_proc;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, ptrref_type const& ptr) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        type_symbol to_type = ptr.target;

        // we need to canonicalize the type_reference

        contextual_type_reference to_type_ref;
        to_type_ref.type = to_type;
        to_type_ref.context = input.context;

        auto canon_ptr_to_type = co_await rpnx::querygraph::request< lookup_query >(to_type_ref);
        if (!canon_ptr_to_type.has_value())
        {
            co_return std::nullopt;
        }

        ptrref_type canonical_ptr_type;
        canonical_ptr_type.qual = ptr.qual;
        canonical_ptr_type.ptr_class = ptr.ptr_class;
        canonical_ptr_type.target = canon_ptr_to_type.value();

        co_return canonical_ptr_type;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, attached_type_reference const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        attached_type_reference attached = type;
        auto carrying_type = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = input.context, .type = std::move(attached.carrying_type)});
        if (!carrying_type.has_value())
        {
            co_return std::nullopt;
        }
        auto attached_symbol = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = input.context, .type = std::move(attached.attached_symbol)});
        if (!attached_symbol.has_value())
        {
            co_return std::nullopt;
        }
        attached.carrying_type = std::move(*carrying_type);
        attached.attached_symbol = std::move(*attached_symbol);
        co_return attached;
    }

    auto lookup_impl_overloads(contextual_type_reference const&, numeric_literal_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, numeric_literal_any_temploidic const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        std::string str = std::string() + "unimplemented: " + input.type.type().name();
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            co_yield rpnx::querygraph::debug_message("{}", str);
        }
        throw quxlang::semantic_compilation_error(str);
    }

    auto lookup_impl_overloads(contextual_type_reference const&, string_literal_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, string_literal_any_temploidic const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        std::string str = std::string() + "unimplemented: " + input.type.type().name();
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            co_yield rpnx::querygraph::debug_message("{}", str);
        }
        throw quxlang::semantic_compilation_error(str);
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, nvalue_slot const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        nvalue_slot canonical_slot = type;
        auto canonical_target = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
            .context = input.context,
            .type = canonical_slot.target,
        });
        if (!canonical_target.has_value())
        {
            co_return std::nullopt;
        }
        canonical_slot.target = *canonical_target;
        co_return canonical_slot;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, dvalue_slot const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        dvalue_slot canonical_slot = type;
        auto canonical_target = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
            .context = input.context,
            .type = canonical_slot.target,
        });
        if (!canonical_target.has_value())
        {
            co_return std::nullopt;
        }
        canonical_slot.target = *canonical_target;
        co_return canonical_slot;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, array_type const& arry) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        // TODO: support non-64bit platforms
        std::uint64_t element_count = co_await evaluate_u64_type_expression(input.context, arry.element_count);
        array_type result_type;
        result_type.element_count = expression_numeric_literal{std::to_string(element_count)};
        auto lookup_element_type = co_await rpnx::querygraph::request< lookup_query >({.context = input.context, .type = arry.element_type});
        if (!lookup_element_type.has_value())
        {
            co_return std::nullopt;
        }
        result_type.element_type = strip_source_locations(lookup_element_type.value());
        co_return result_type;
    }

    auto lookup_impl_overloads(contextual_type_reference const&, size_type const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        machine_target_info machine_info = co_await rpnx::querygraph::request< machine_info_query >(std::monostate{});
        std::uint64_t size_bits = cpu_is_layoutless(machine_info.cpu_type) ? 64 : machine_info.pointer_size_bytes() * 8;
        co_return int_type{.bits = size_bits, .has_sign = false};
    }

    auto lookup_impl_overloads(contextual_type_reference const&, address_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const&, type_index_type const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const&, readonly_constant const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, storage const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        storage result_type;
        for (type_symbol const& stored_type : type.storable_types)
        {
            auto lookup_stored_type = co_await rpnx::querygraph::request< lookup_query >({.context = input.context, .type = stored_type});
            if (!lookup_stored_type.has_value())
            {
                co_return std::nullopt;
            }
            result_type.storable_types.insert(strip_source_locations(lookup_stored_type.value()));
        }
        co_return result_type;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, aligned_storage const& storage_type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        machine_target_info machine_info = co_await rpnx::querygraph::request< machine_info_query >(std::monostate{});
        if (cpu_is_layoutless(machine_info.cpu_type))
        {
            throw semantic_compilation_error("ALIGNED_STORAGE is unavailable for a layoutless target");
        }
        std::uint64_t size_value = co_await evaluate_u64_type_expression(input.context, storage_type.size);
        std::uint64_t align_value = co_await evaluate_u64_type_expression(input.context, storage_type.align);

        co_return aligned_storage{
            .size = expression_numeric_literal{std::to_string(size_value)},
            .align = expression_numeric_literal{std::to_string(align_value)},
        };
    }

    auto lookup_impl_overloads(contextual_type_reference const&, virtual_storage const& type) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        co_return type;
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, array_initializer_type const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        std::string str = std::string() + "unimplemented: " + input.type.type().name();
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            co_yield rpnx::querygraph::debug_message("{}", str);
        }
        throw quxlang::semantic_compilation_error(str);
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, static_local_ref const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        std::string str = std::string() + "unimplemented: " + input.type.type().name();
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            co_yield rpnx::querygraph::debug_message("{}", str);
        }
        throw quxlang::semantic_compilation_error(str);
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, static_snapshot_ref const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        std::string str = std::string() + "unimplemented: " + input.type.type().name();
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            co_yield rpnx::querygraph::debug_message("{}", str);
        }
        throw quxlang::semantic_compilation_error(str);
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, pack_arg_type_ref const& ref) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        if (!input.context.type_is< instanciation_reference >())
        {
            throw quxlang::semantic_compilation_error("PACK_ARG_TYPE requires an instantiated function context");
        }

        std::uint64_t pack_index = co_await evaluate_u64_type_expression(input.context, ref.index);

        function_pack_info pack_info = co_await rpnx::querygraph::request< function_pack_info_query >(as< instanciation_reference >(input.context));
        auto pack_it = pack_info.packs.find(ref.pack_name);
        if (pack_it == pack_info.packs.end())
        {
            throw semantic_compilation_error("Unknown positional pack '" + ref.pack_name + "'");
        }
        if (pack_index >= pack_it->second.size)
        {
            throw semantic_compilation_error("PACK_ARG_TYPE index is out of range for positional pack '" + ref.pack_name + "'");
        }

        co_return pack_it->second.types.at(static_cast< std::vector< type_symbol >::size_type >(pack_index));
    }

    auto lookup_impl_overloads(contextual_type_reference const& input, decltype_type_ref const& ref) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        /** @brief Finds a declared parameter type in the enclosing instantiation. */
        auto declared_parameter_type_from_context = [](type_symbol context, std::string const& name) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
        {
            std::optional< type_symbol > current_context = std::move(context);
            while (current_context.has_value())
            {
                if (typeis< instanciation_reference >(*current_context))
                {
                    instanciation_reference const& inst = as< instanciation_reference >(*current_context);
                    auto declaration = co_await rpnx::querygraph::request< function_declaration_query >(inst.temploid);
                    if (declaration.has_value())
                    {
                        std::size_t positional_index = 0;
                        for (ast2_function_parameter const& param : declaration->header.call_parameters)
                        {
                            if (param.is_named_rest)
                            {
                                if (param.name == name)
                                {
                                    std::set< std::string > fixed_names{"THIS", "RETURN"};
                                    for (ast2_function_parameter const& fixed : declaration->header.call_parameters)
                                    {
                                        if (fixed.api_name.has_value())
                                        {
                                            fixed_names.insert(*fixed.api_name);
                                        }
                                    }
                                    composite_type type;
                                    for (std::pair< std::string const, parameter_instantiation > const& field : inst.params.named)
                                    {
                                        if (!fixed_names.contains(field.first))
                                        {
                                            type.fields.emplace(field.first, parameter_instantiation_type(field.second));
                                        }
                                    }
                                    co_return type;
                                }
                                continue;
                            }
                            if (param.api_name.has_value())
                            {
                                if (param.api_name.value() == name || (param.name.has_value() && param.name.value() == name))
                                {
                                    auto it = inst.params.named.find(param.api_name.value());
                                    if (it == inst.params.named.end())
                                    {
                                        co_return std::nullopt;
                                    }
                                    co_return parameter_instantiation_type(it->second);
                                }
                                continue;
                            }

                            if (param.is_pack)
                            {
                                if (param.name.has_value() && param.name.value() == name)
                                {
                                    throw quxlang::semantic_compilation_error("DECLTYPE cannot name a positional pack; use PACK_ARG_TYPE for pack elements.");
                                }
                                positional_index = inst.params.positional.size();
                                continue;
                            }

                            if (param.name.has_value() && param.name.value() == name)
                            {
                                if (positional_index >= inst.params.positional.size())
                                {
                                    co_return std::nullopt;
                                }
                                co_return parameter_instantiation_type(inst.params.positional.at(positional_index));
                            }
                            positional_index++;
                        }
                    }
                }

                current_context = type_parent(*current_context);
            }

            co_return std::nullopt;
        };

        if (ref.symbol.template type_is< freebound_identifier >())
        {
            std::string const& name = as< freebound_identifier >(ref.symbol).name;
            auto parameter_type = co_await declared_parameter_type_from_context(input.context, name);
            if (parameter_type.has_value())
            {
                co_return *parameter_type;
            }
        }

        auto canonical_symbol = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = input.context, .type = ref.symbol});
        if (!canonical_symbol.has_value())
        {
            co_return std::nullopt;
        }

        auto kind = co_await rpnx::querygraph::request< symbol_type_query >(*canonical_symbol);
        if (kind != symbol_kind::global_variable)
        {
            throw quxlang::semantic_compilation_error("DECLTYPE requires a value symbol");
        }

        if (canonical_symbol->template type_is< subtag_type >())
        {
            auto binding = co_await rpnx::querygraph::request< subtag_binding_query >(canonical_symbol->template get_as< subtag_type >());
            if (binding.has_value() && binding->template type_is< parameter_value_instantiation >())
            {
                co_return binding->template get_as< parameter_value_instantiation >().type;
            }
            throw quxlang::semantic_compilation_error("DECLTYPE subtag target is not a value");
        }

        auto declaration = co_await rpnx::querygraph::request< symboid_query >(*canonical_symbol);
        if (!declaration.template type_is< ast2_variable_declaration >())
        {
            throw quxlang::semantic_compilation_error("DECLTYPE target variable is not declared as a variable");
        }

        auto declared_type = as< ast2_variable_declaration >(declaration).type;
        auto resolved = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = *canonical_symbol, .type = declared_type});
        if (!resolved.has_value())
        {
            co_return std::nullopt;
        }
        co_return *resolved;
    }

    auto lookup_impl_overloads(contextual_type_reference const&, typeof_type_ref const&) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > >
    {
        throw quxlang::semantic_compilation_error("TYPEOF requires a function generation context for expression type resolution");
    }
} // namespace quxlang::detail

rpnx::querygraph::coroutine< quxlang::lookup_spec > quxlang::lookup_impl(contextual_type_reference input)
{
    if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
    {
        co_yield rpnx::querygraph::debug_message("type lookup,");
        co_yield rpnx::querygraph::debug_message("With Context: {}", to_string(input.context));
        co_yield rpnx::querygraph::debug_message("Looking up type: {}", to_string(input.type));
    }

    co_return co_await rpnx::apply_visitor< rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< std::optional< type_symbol > > >(input.type, [&input]< typename Type >(Type const& type)
    {
        return detail::lookup_impl_overloads(input, type);
    });
}
