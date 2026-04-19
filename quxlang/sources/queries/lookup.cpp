// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/lookup_spec.hpp>
#include <quxlang/macros.hpp>

#include "quxlang/data/constexpr_types.hpp"
#include "quxlang/manipulators/typeutils.hpp"

#include "quxlang/manipulators/typeutils.hpp"


namespace quxlang
{
    auto apply_context_instantiation_scopes(type_symbol context, constexpr_input& input) -> rpnx::querygraph::coroutine< lookup_spec >::cosubroutine< void >
    {
        std::optional< type_symbol > current_context = std::move(context);
        while (current_context.has_value())
        {
            if (typeis< instanciation_reference >(*current_context))
            {
                auto const& inst = as< instanciation_reference >(*current_context);
                auto inst_kind = co_await rpnx::querygraph::request< symbol_type_query >(inst.temploid);
                if (inst_kind == symbol_kind::template_)
                {
                    merge_instantiation_scope_bindings(instantiation_scope_for(inst), input);
                }
            }
            current_context = type_parent(*current_context);
        }
        co_return;
    }
}

rpnx::querygraph::coroutine< quxlang::lookup_spec > quxlang::lookup_impl(contextual_type_reference input)
{
    type_symbol context = input.context;
    type_symbol const& type = input.type;
    output_info const machine_info = co_await rpnx::querygraph::request< machine_info_query >(std::monostate{});



    if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
    {
        co_yield rpnx::querygraph::debug_message("type lookup,");
        co_yield rpnx::querygraph::debug_message("With Context: {}", to_string(context));
        co_yield rpnx::querygraph::debug_message("Looking up type: {}", to_string(type));
    }

    auto current_module = get_root_module(context).value_or(void_type{});

    if (type.type_is< byte_type >())
    {
        co_return type;
    }
    if (type.type_is< absolute_module_reference >())
    {
        co_return type;
    }
    else if (type.type_is< readonly_constant >())
    {
        co_return type;
    }

    else if (type.type_is< size_type >())
    {
        co_return int_type{.bits = machine_info.pointer_size_bytes() * 8, .has_sign = false};
    }
    else if (type.template type_is< ptrref_type >())
    {
        ptrref_type const& ptr = as< ptrref_type >(type);

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
    else if (type.template type_is< procedure_type >())
    {
        procedure_type canonical_proc = as< procedure_type >(type);

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
    else if (type.template type_is< pack_arg_type_ref >())
    {
        if (!context.type_is< instanciation_reference >())
        {
            throw std::logic_error("PACK_ARG_TYPE requires an instantiated function context");
        }

        auto const& ref = as< pack_arg_type_ref >(type);
        constexpr_input index_input;
        index_input.context = context;
        index_input.expr = ref.index;
        std::uint64_t const pack_index = co_await rpnx::querygraph::request< constexpr_u64_query >(index_input);

        auto const pack_info = co_await rpnx::querygraph::request< function_pack_info_query >(as< instanciation_reference >(context));
        auto const pack_it = pack_info.packs.find(ref.pack_name);
        if (pack_it == pack_info.packs.end())
        {
            throw std::logic_error("Unknown positional pack '" + ref.pack_name + "'");
        }
        if (pack_index >= pack_it->second.size)
        {
            throw std::logic_error("PACK_ARG_TYPE index is out of range for positional pack '" + ref.pack_name + "'");
        }

        co_return pack_it->second.types.at(static_cast< std::vector< type_symbol >::size_type >(pack_index));
    }
    else if (type.template type_is< freebound_identifier >())
    {
        std::optional< type_symbol > current_context = context;
        assert(current_context.has_value());
        assert(!type_is_contextual(current_context.value()));

        auto fb = as< freebound_identifier >(type);

        while (current_context.has_value())
        {
            std::string name = fb.name;
            if (typeis< instanciation_reference >(*current_context))
            {
                if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
                {
                    co_yield rpnx::querygraph::debug_message("Instanciation:  within {} check  {}", to_string(*current_context), to_string(type));
                }

                // Two possibilities, 1 = this is a template, 2 = this is a function
                instanciation_reference inst = as< instanciation_reference >(*current_context);

                auto param_set = co_await rpnx::querygraph::request< instanciation_tempar_map_query >(inst);

                if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
                {
                    co_yield rpnx::querygraph::debug_message("Param set, name={}", name);
                    co_yield rpnx::querygraph::debug_message("Param map {} / {} {}", to_string(inst), to_string(*current_context), param_set.parameter_map.size());
                    for (auto it = param_set.parameter_map.begin(); it != param_set.parameter_map.end(); it++)
                    {
                        co_yield rpnx::querygraph::debug_message("Param map: k={} v={}", it->first, to_string(it->second));
                    }
                }
                auto it = param_set.parameter_map.find(name);
                if (it != param_set.parameter_map.end())
                {
                    co_return it->second;
                }
            }

            subsymbol sub2{current_context.value(), fb.name};

            if (current_context.value().type_is< absolute_module_reference >())
            {
                ast2_module_declaration const & module_ast = co_await rpnx::querygraph::request< module_ast_query >(as< absolute_module_reference >(current_context.value()).module_name);

                auto import_at = module_ast.imports.find(fb.name);

                if (import_at != module_ast.imports.end())
                {
                    co_return absolute_module_reference{import_at->second};
                }
            }

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
                co_return sub2;
            }

            current_context = type_parent(current_context.value());
            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                co_yield rpnx::querygraph::debug_message("New context: {}", quxlang::to_string(current_context.value_or(void_type{})));
            }
        }

        std::string str = "Could not find '" + fb.name + "'";
        co_return std::nullopt;
    }
    else if (type.template type_is< subsymbol >())
    {
        subsymbol const& sub = as< subsymbol >(type);

        type_symbol const& parent = sub.of;

        if (parent.template type_is< context_reference >())
        {
            auto rval = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = context, .type = subsymbol{current_module, sub.name}});
            assert(!type_is_contextual(rval.value_or(void_type{})));
            co_return rval;
        }

        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            co_yield rpnx::querygraph::debug_message("Parent: {}", to_string(parent));
        }

        auto parent_canonical_opt = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = context, .type = parent});
        if (!parent_canonical_opt.has_value())
        {
            std::string str = "Could not find '" + sub.name + "' in context " + quxlang::to_string(context);
            co_return std::nullopt;
        }

        auto parent_canonical = parent_canonical_opt.value();

        auto parent_canonical_str = to_string(parent_canonical);
        assert(!type_is_contextual(parent_canonical));
        co_return subsymbol{parent_canonical, sub.name};
    }
    else if (type.template type_is< submember >())
    {
        submember const& sub = as< submember >(type);

        type_symbol const& parent = sub.of;

        if (parent.template type_is< context_reference >())
        {
            std::optional< type_symbol > current_context = context;
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
                    co_return submember{current_context.value(), sub.name};
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

        auto parent_canonical = (co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = context, .type = parent})).value();

        assert(!type_is_contextual(parent_canonical));
        co_return submember{parent_canonical, sub.name};
    }
    else if (type.template type_is< initialization_reference >())
    {
        initialization_reference const& param_set = as< initialization_reference >(type);

        initialization_reference output;
        output.adaptations = param_set.adaptations;
        output.context = param_set.context.value_or(context);
        output.arguments = param_set.arguments;

        auto callee_canonical = co_await rpnx::querygraph::request< lookup_query >({
                                                                .context = context,
                                                                .type = param_set.initializee,
                                                            });
        if (!callee_canonical.has_value())
        {
            co_return std::nullopt;
        }

        output.initializee = callee_canonical.value();

        for (auto& p : param_set.parameters.positional)
        {
            auto param_canonical = co_await rpnx::querygraph::request< lookup_query >({.context = context, .type = parameter_instantiation_type(p)});
            if (!param_canonical.has_value())
            {
                co_return std::nullopt;
            }

            if (p.template type_is< parameter_value_instantiation >())
            {
                auto value = p.template get_as< parameter_value_instantiation >();
                value.type = param_canonical.value();
                output.parameters.positional.push_back(std::move(value));
            }
            else
            {
                output.parameters.positional.push_back(make_type_instantiation(param_canonical.value()));
            }
        }

        for (auto const& [name, p] : param_set.parameters.named)
        {
            auto param_canonical = co_await rpnx::querygraph::request< lookup_query >({.context = context, .type = parameter_instantiation_type(p)});
            if (!param_canonical.has_value())
            {
                co_return std::nullopt;
            }

            if (p.template type_is< parameter_value_instantiation >())
            {
                auto value = p.template get_as< parameter_value_instantiation >();
                value.type = param_canonical.value();
                output.parameters.named[name] = std::move(value);
            }
            else
            {
                output.parameters.named[name] = make_type_instantiation(param_canonical.value());
            }
        }

        auto initializee_kind = co_await rpnx::querygraph::request< symbol_type_query >(output.initializee);
        if (initializee_kind == symbol_kind::templex || initializee_kind == symbol_kind::template_)
        {
            auto inst = co_await rpnx::querygraph::request< instanciation_query >(output);
            if (!inst.has_value())
            {
                co_return std::nullopt;
            }
            co_return *inst;
        }

        assert(!type_is_contextual(output));

        co_return output;
    }
    else if (type.template type_is< instanciation_reference >())
    {
        auto const& inst = as< instanciation_reference >(type);
        instanciation_reference output = inst;

        auto templexoid_canonical = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
            .context = context,
            .type = inst.temploid.templexoid,
        });
        if (!templexoid_canonical.has_value())
        {
            co_return std::nullopt;
        }
        output.temploid.templexoid = *templexoid_canonical;

        for (auto& param : output.params.positional)
        {
            auto param_canonical = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = context, .type = parameter_instantiation_type(param)});
            if (!param_canonical.has_value())
            {
                co_return std::nullopt;
            }
            if (param.template type_is< parameter_value_instantiation >())
            {
                param.template get_as< parameter_value_instantiation >().type = *param_canonical;
            }
            else
            {
                param = make_type_instantiation(*param_canonical);
            }
        }

        for (auto& [_, param] : output.params.named)
        {
            auto param_canonical = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = context, .type = parameter_instantiation_type(param)});
            if (!param_canonical.has_value())
            {
                co_return std::nullopt;
            }
            if (param.template type_is< parameter_value_instantiation >())
            {
                param.template get_as< parameter_value_instantiation >().type = *param_canonical;
            }
            else
            {
                param = make_type_instantiation(*param_canonical);
            }
        }

        assert(!type_is_contextual(output));
        co_return output;
    }
    else if (type.template type_is< int_type >())
    {
        assert(!type_is_contextual(type));
        co_return type;
    }
    else if (type.template type_is< bool_type >())
    {
        co_return type;
    }
    else if (typeis< void_type >(type))
    {
        co_return type;
    }
    else if (typeis< auto_temploidic >(type))
    {
        co_return type;
    }
    else if (typeis< type_temploidic >(type))
    {
        co_return type;
    }
    else if (typeis< array_type >(type))
    {
        auto const& arry = type.template get_as< array_type >();
        constexpr_input ce_input;
        ce_input.context = context;
        ce_input.expr = arry.element_count;
        co_await apply_context_instantiation_scopes(context, ce_input);
        // TODO: support non-64bit platforms
        std::uint64_t element_count = co_await rpnx::querygraph::request< constexpr_u64_query >(ce_input);
        array_type result_type;
        result_type.element_count = expression_numeric_literal{std::to_string(element_count)};
        auto lookup_element_type = co_await rpnx::querygraph::request< lookup_query >({.type = arry.element_type, .context = context});
        if (!lookup_element_type.has_value())
        {
            co_return std::nullopt;
        }
        result_type.element_type = strip_source_locations(lookup_element_type.value());
        co_return result_type;
    }
    else if (typeis< storage >(type))
    {
        storage result_type;
        for (auto const& stored_type : as< storage >(type).storable_types)
        {
            auto lookup_stored_type = co_await rpnx::querygraph::request< lookup_query >({.type = stored_type, .context = context});
            if (!lookup_stored_type.has_value())
            {
                co_return std::nullopt;
            }
            result_type.storable_types.insert(strip_source_locations(lookup_stored_type.value()));
        }
        co_return result_type;
    }
    else if (typeis< aligned_storage >(type))
    {
        auto const& storage_type = as< aligned_storage >(type);
        constexpr_input size_input{
            .context = context,
            .expr = storage_type.size,
        };
        constexpr_input align_input{
            .context = context,
            .expr = storage_type.align,
        };

        auto size_value = co_await rpnx::querygraph::request< constexpr_u64_query >(size_input);
        auto align_value = co_await rpnx::querygraph::request< constexpr_u64_query >(align_input);

        co_return aligned_storage{
            .size = expression_numeric_literal{std::to_string(size_value)},
            .align = expression_numeric_literal{std::to_string(align_value)},
        };
    }
    else
    {
        std::string str = std::string() + "unimplemented: " + type.type().name();
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            co_yield rpnx::querygraph::debug_message("{}", str);
        }
        throw std::logic_error(str);
    }

    throw std::logic_error("unreachable code reached in lookup resolver");
}
