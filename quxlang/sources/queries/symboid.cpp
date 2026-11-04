// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/merge_entity.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include <quxlang/data/builtin_array_ranges.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/data/coroutine_types.hpp>
#include <quxlang/queries/specs/symboid_spec.hpp>

#include <optional>
#include <string>
#include <variant>

rpnx::querygraph::coroutine< quxlang::symboid_spec > quxlang::symboid_impl(type_symbol input)
{
    if (input.type_is< subsymbol >())
    {
        subsymbol const& symbol = input.get_as< subsymbol >();
        if (symbol.name == "__COROUTINE_DESTRUCTOR" || symbol.name == "__COROUTINE_SUBOBJECT_DESTRUCTOR")
        {
            ast2_function_declaration declaration;
            declaration.header.execution_kind = function_execution_kind::coroutine;
            declaration.header.controller_parameter = ast2_function_parameter{.api_name = "CONTROLLER", .type = auto_temploidic{}};
            declaration.header.call_parameters.push_back(ast2_function_parameter{.api_name = "THIS", .type = make_mref(symbol.of)});
            declaration.definition.return_type = void_type{};
            co_return functum{.functions = {std::move(declaration)}};
        }
    }
    if (std::optional< instanciation_reference > coroutine = coroutine_frame_specialization(input))
    {
        std::optional< ast2_function_declaration > source = co_await rpnx::querygraph::request< function_declaration_query >(coroutine->temploid);
        if (!source.has_value() || source->header.execution_kind != function_execution_kind::coroutine)
        {
            throw semantic_compilation_error("FRAME_TYPE requires a coroutine specialization");
        }
        instatype parameters = co_await rpnx::querygraph::request< instanciation_concrete_params_query >(*coroutine);
        ast2_struct_declaration frame;
        frame.struct_keywords.insert(keywords::rooted);
        ast2_function_declaration initialize;
        initialize.definition.return_type = void_type{};
        initialize.header.call_parameters.push_back(ast2_function_parameter{
            .name = "THIS", .api_name = "THIS", .type = make_mref(input),
        });
        auto capture_type = [](parameter_instantiation const& parameter) -> type_symbol
        {
            type_symbol type = parameter_instantiation_type(parameter);
            if (type.type_is< nvalue_slot >()) return make_mref(type.get_as< nvalue_slot >().target);
            if (type.type_is< dvalue_slot >()) return make_mref(type.get_as< dvalue_slot >().target);
            return type;
        };
        std::map< std::string, expression > named_defaults;
        std::vector< std::optional< expression > > positional_defaults;
        std::optional< type_symbol > lifecycle_storage;
        for (ast2_function_parameter const& parameter : source->header.call_parameters)
        {
            if (parameter.is_named_rest) continue;
            if (parameter.api_name.has_value())
            {
                if (parameter.default_expr.has_value()) named_defaults.emplace(*parameter.api_name, *parameter.default_expr);
            }
            else positional_defaults.push_back(parameter.default_expr);
        }
        for (std::size_t index = 0; index < parameters.positional.size(); ++index)
        {
            initialize.header.call_parameters.push_back(ast2_function_parameter{
                .name = "__ARG" + std::to_string(index),
                .type = capture_type(parameters.positional.at(index)),
                .default_expr = index < positional_defaults.size() ? positional_defaults[index] : std::nullopt,
            });
        }
        for (std::pair< std::string const, parameter_instantiation > const& parameter : parameters.named)
        {
            if (parameter.first == "CONTROLLER") continue;
            if (parameter.first == "THIS")
            {
                type_symbol const& receiver = parameter_instantiation_type(parameter.second);
                if (receiver.type_is< nvalue_slot >()) lifecycle_storage = receiver.get_as< nvalue_slot >().target;
                else if (receiver.type_is< dvalue_slot >()) lifecycle_storage = receiver.get_as< dvalue_slot >().target;
                else if (coroutine->temploid.templexoid.type_is< submember >() && coroutine->temploid.templexoid.get_as< submember >().name == "DESTRUCTOR")
                    lifecycle_storage = remove_ref(receiver);
                else if (coroutine->temploid.templexoid.type_is< subsymbol >())
                {
                    std::string const& name = coroutine->temploid.templexoid.get_as< subsymbol >().name;
                    if (name == "__COROUTINE_DESTRUCTOR" || name == "__COROUTINE_SUBOBJECT_DESTRUCTOR") lifecycle_storage = remove_ref(receiver);
                }
            }
            initialize.header.call_parameters.push_back(ast2_function_parameter{
                .name = parameter.first == "THIS" ? "__RECEIVER" : parameter.first,
                .api_name = parameter.first == "THIS" ? "RECEIVER" : parameter.first,
                .type = capture_type(parameter.second),
                .default_expr = named_defaults.contains(parameter.first) ? std::optional< expression >(named_defaults.at(parameter.first)) : std::nullopt,
            });
        }
        if (lifecycle_storage.has_value())
        {
            ast2_function_declaration storage_initialize = initialize;
            for (ast2_function_parameter& parameter : storage_initialize.header.call_parameters)
            {
                if (parameter.api_name == "RECEIVER") parameter.type = make_mref(storage{.storable_types = {*lifecycle_storage}});
            }
            frame.declarations.push_back(member_subdeclaration{.decl = std::move(storage_initialize), .name = "INITIALIZE"});
        }
        frame.declarations.push_back(member_subdeclaration{.decl = std::move(initialize), .name = "INITIALIZE"});
        type_symbol controller = remove_ref(parameter_instantiation_type(parameters.named.at("CONTROLLER")));
        bool concrete_controller = !is_template(controller);
        if (!concrete_controller && (controller.type_is< instanciation_reference >() || controller.type_is< subsymbol >()))
        {
            concrete_controller = (co_await rpnx::querygraph::request< symbol_type_query >(controller)) == symbol_kind::class_;
        }
        if (!concrete_controller)
        {
            throw semantic_compilation_error("FRAME_TYPE requires a concrete CONTROLLER specialization");
        }
        for (std::string const& name : {std::string("RESUME"), std::string("TRY_CANCEL")})
        {
            if (name == "TRY_CANCEL" && source->header.cancellation_mode == coroutine_cancellation_mode::none) continue;
            ast2_function_declaration driver;
            driver.header.call_parameters.push_back(ast2_function_parameter{
                .name = "THIS", .api_name = "THIS", .type = make_mref(input),
            });
            driver.header.call_parameters.push_back(ast2_function_parameter{
                .name = "CONTROLLER", .api_name = "CONTROLLER", .type = make_mref(controller),
            });
            driver.definition.return_type = bool_type{};
            frame.declarations.push_back(member_subdeclaration{.decl = std::move(driver), .name = name});
        }
        co_return frame;
    }
    if (input.type_is< array_type >())
    {
        co_return builtin_array_index_declaration(input.get_as< array_type >());
    }

    if (std::optional< std::uint64_t > number = snapshot_number(input))
    {
        snapshot_value snapshot = co_await rpnx::querygraph::subquery_request< snapshot_value_subquery >(
            as< instanciation_reference >(as< submember >(input).of), *number);
        ast2_variable_declaration declaration;
        declaration.type = snapshot.type;
        declaration.keyword_tags.insert("STATIC");
        co_return declaration;
    }

    if (input == type_symbol(builtin_symbol{.name = "COROUTINE_CANCELLATION_MODE"}))
    {
        ast2_enum_declaration declaration;
        declaration.entries = {
            ast2_enum_value_declaration{.name = "NONE", .is_default = true},
            ast2_enum_value_declaration{.name = "COOPERATIVE"},
            ast2_enum_value_declaration{.name = "INSTANT"},
        };
        co_return declaration;
    }
    if (input == type_symbol(builtin_symbol{.name = "COROUTINE_CANCELLED"}))
    {
        co_return ast2_struct_declaration{};
    }

    // The canonical polymorphic view has a compiler-owned identity and no source declaration.
    // Expose its operations through ordinary member and polymorphic-layout queries.
    if (input == type_symbol(builtin_symbol{.name = "POLYMORPHIC_BASE"}))
    {
        ast2_struct_declaration declaration;
        declaration.struct_keywords.insert(keywords::polymorphic);
        co_return declaration;
    }

    if (input.type_is< subsymbol >())
    {
        subsymbol const& symbol = input.get_as< subsymbol >();
        if (symbol.name == "__BENCHMARK_STANDALONE" || symbol.name.starts_with("__BENCHMARK_CASE_"))
        {
            benchmark_function_input specialization{.benchmark = symbol.of, .standalone = symbol.name == "__BENCHMARK_STANDALONE"};
            if (!specialization.standalone)
            {
                specialization.case_index = std::stoull(symbol.name.substr(std::string("__BENCHMARK_CASE_").size()));
            }
            ast2_function_declaration declaration = co_await rpnx::querygraph::request< benchmark_function_query >(specialization);
            co_return functum{.functions = {std::move(declaration)}};
        }
    }

    // Test bodies use ordinary function identities so their closures share the function query path.
    if (input.type_is< subsymbol >() && input.get_as< subsymbol >().name == "__TEST_BODY")
    {
        ast2_symboid parent = co_await rpnx::querygraph::request< symboid_query >(input.get_as< subsymbol >().of);
        if (parent.type_is< ast2_test >())
        {
            ast2_function_declaration declaration;
            declaration.definition = parent.get_as< ast2_test >().definition;
            declaration.definition.return_type = void_type{};
            declaration.location = parent.get_as< ast2_test >().location;
            co_return functum{.functions = {std::move(declaration)}};
        }
    }
    if (input.type_is< composite_type >())
    {
        ast2_struct_declaration declaration;
        for (std::pair< std::string const, type_symbol > const& field : input.get_as< composite_type >().fields)
        {
            declaration.declarations.push_back(member_subdeclaration{
                .decl = ast2_variable_declaration{.type = field.second},
                .name = field.first,
            });
        }
        co_return declaration;
    }

    auto asm_procedure_matches_target = [](ast2_asm_procedure_declaration const& declaration, machine_target_info const& machine) -> bool
    {
        if (declaration.architecture == "X86")
        {
            return machine.cpu_type == cpu::x86_32;
        }
        if (declaration.architecture == "X64")
        {
            return machine.cpu_type == cpu::x86_64;
        }
        if (declaration.architecture == "ARM32")
        {
            return machine.cpu_type == cpu::arm_32;
        }
        if (declaration.architecture == "ARM64")
        {
            return machine.cpu_type == cpu::arm_64;
        }
        if (declaration.architecture == "Z_ARCH")
        {
            return machine.cpu_type == cpu::z_arch;
        }

        throw compiler_bug("Unsupported asm procedure architecture during merge selection: " + declaration.architecture);
    };

    auto selected_template_decl_to_symboid = [](declaration const& decl) -> ast2_symboid
    {
        if (typeis< ast2_alias_declaration >(decl))
        {
            return as< ast2_alias_declaration >(decl);
        }
        if (typeis< ast2_struct_declaration >(decl))
        {
            return as< ast2_struct_declaration >(decl);
        }
        else if (typeis< ast2_union_declaration >(decl))
        {
            return as< ast2_union_declaration >(decl);
        }
        else if (typeis< ast2_variant_declaration >(decl))
        {
            return as< ast2_variant_declaration >(decl);
        }
        else if (typeis< ast2_interface_declaration >(decl))
        {
            return as< ast2_interface_declaration >(decl);
        }
        else if (typeis< ast2_generic_declaration >(decl))
        {
            return as< ast2_generic_declaration >(decl);
        }
        else if (typeis< ast2_implementation_declaration >(decl))
        {
            return as< ast2_implementation_declaration >(decl);
        }
        else if (typeis< ast2_enum_declaration >(decl))
        {
            return as< ast2_enum_declaration >(decl);
        }
        else if (typeis< ast2_flagset_declaration >(decl))
        {
            return as< ast2_flagset_declaration >(decl);
        }
        else if (typeis< ast2_function_declaration >(decl))
        {
            functum output;
            output.functions.push_back(as< ast2_function_declaration >(decl));
            return output;
        }
        else if (typeis< ast2_variable_declaration >(decl))
        {
            return as< ast2_variable_declaration >(decl);
        }
        else if (typeis< ast2_namespace_declaration >(decl))
        {
            return as< ast2_namespace_declaration >(decl);
        }
        else if (typeis< ast2_extern_procedure >(decl))
        {
            return as< ast2_extern_procedure >(decl);
        }
        else if (typeis< ast2_extern_type >(decl))
        {
            return as< ast2_extern_type >(decl);
        }
        else if (typeis< ast2_asm_procedure_declaration >(decl))
        {
            return as< ast2_asm_procedure_declaration >(decl);
        }
        else if (typeis< ast2_test >(decl))
        {
            return as< ast2_test >(decl);
        }
        else
        {
            throw compiler_bug("Unsupported selected template declaration");
        }
    };

    if (typeis< initialization_reference >(input))
    {
        auto const& init = as< initialization_reference >(input);
        auto inst = co_await rpnx::querygraph::request< instanciation_query >(init);
        if (!inst.has_value())
        {
            throw quxlang::compiler_bug("symboid resolver received a non-canonical initialization_reference that could not be canonicalized: " + to_string(input));
        }
        co_return co_await rpnx::querygraph::request< symboid_query >(*inst);
    }

    if (typeis< instanciation_reference >(input))
    {
        auto const& inst = as< instanciation_reference >(input);
        auto temploid_kind = co_await rpnx::querygraph::request< symbol_type_query >(inst.temploid);

        if (temploid_kind == symbol_kind::template_)
        {
            if (co_await rpnx::querygraph::request< template_builtin_query >(inst.temploid))
            {
                if (inst.temploid.templexoid == type_symbol(builtin_symbol{"__DEFERRED"}))
                {
                    if (!inst.params.positional.empty() || inst.params.named.size() != 1 || !inst.params.named.contains("T") || !inst.params.named.at("T").type_is< parameter_type_instantiation >())
                    {
                        throw compiler_bug("Deferred guard requires one canonical type argument T");
                    }
                    type_symbol callable_type = inst.params.named.at("T").get_as< parameter_type_instantiation >().type;
                    ast2_struct_declaration guard;
                    guard.struct_keywords.insert(keywords::rooted);
                    guard.declarations.push_back(member_subdeclaration{
                        .decl = ast2_variable_declaration{.type = callable_type},
                        .name = "__CALLABLE",
                    });
                    for (qualifier source_qualifier : {qualifier::constant, qualifier::temp})
                    {
                        ast2_function_declaration constructor;
                        constructor.header.call_parameters.push_back(ast2_function_parameter{
                            .name = "OTHER", .api_name = "OTHER",
                            .type = ptrref_type{.target = callable_type, .ptr_class = pointer_class::ref, .qual = source_qualifier},
                        });
                        constructor.definition.return_type = void_type{};
                        constructor.definition.delegates.push_back(ast2_function_delegate{
                            .target = submember{.of = freebound_identifier{"THIS"}, .name = "__CALLABLE"},
                            .args = {expression_arg{.name = "OTHER", .value = expression_forward{.symbol = freebound_identifier{"OTHER"}}}},
                        });
                        guard.declarations.push_back(member_subdeclaration{.decl = std::move(constructor), .name = "CONSTRUCTOR"});
                    }
                    ast2_function_declaration destructor;
                    destructor.definition.return_type = void_type{};
                    destructor.definition.body.statements.push_back(function_expression_statement{
                        .expr = expression_call{.callee = expression_thisdot_reference{.field_name = "__CALLABLE"}},
                    });
                    guard.declarations.push_back(member_subdeclaration{.decl = std::move(destructor), .name = "DESTRUCTOR"});
                    co_return guard;
                }
                if (auto atomic_value_type = atomic_type_argument(input); atomic_value_type.has_value())
                {
                    if (!is_valid_atomic_storage_type(*atomic_value_type))
                    {
                        co_return std::monostate{};
                    }

                    ast2_struct_declaration builtin_atomic_struct;
                    builtin_atomic_struct.struct_keywords.insert(keywords::no_implicit_assignment);
                    builtin_atomic_struct.struct_keywords.insert(keywords::not_copyable);
                    builtin_atomic_struct.struct_keywords.insert(keywords::no_default_swap);
                    co_return builtin_atomic_struct;
                }

                functum builtin_functum;
                co_return builtin_functum;
            }

            auto const& sym = co_await rpnx::querygraph::request< symboid_query >(inst.temploid.templexoid);
            if (!typeis< ast2_templex >(sym))
            {
                throw compiler_bug("template instanciation parent did not resolve to ast2_templex");
            }

            auto const& templex = as< ast2_templex >(sym);
            std::uint64_t template_index = inst.temploid.overload_id.value_or(0);
            if (!inst.temploid.overload_id.has_value() && templex.templates.size() != 1)
            {
                throw quxlang::compiler_bug("Template instanciation did not resolve to a unique template declaration");
            }

            if (template_index >= templex.templates.size())
            {
                throw quxlang::compiler_bug("Template overload id is out of range for instanciation symboid lookup");
            }

            co_return selected_template_decl_to_symboid(templex.templates.at(static_cast< std::vector< ast2_template_declaration >::size_type >(template_index)).m_declaration);
        }
    }


    if (typeis< absolute_module_reference >(input))
    {
        auto const & module_ref = as< absolute_module_reference >(input);
        co_return co_await rpnx::querygraph::request< module_ast_query >(as< absolute_module_reference >(input).module_name);
    }

    if (typeis< builtin_symbol >(input) && is_builtin_generic_interface_name(as< builtin_symbol >(input).name))
    {
        ast2_interface_declaration interface_declaration;
        auto append_function = [&](std::string name, std::vector< ast2_function_parameter > parameters, type_symbol return_type)
        {
            ast2_interface_function_declaration function;
            function.name = std::move(name);
            function.header.call_parameters = std::move(parameters);
            function.definition.return_type = std::move(return_type);
            interface_declaration.functions.push_back(std::move(function));
        };
        type_symbol mutable_pointer = ptrref_type{.target = void_type{}, .ptr_class = pointer_class::instance, .qual = qualifier::mut};
        type_symbol constant_pointer = ptrref_type{.target = void_type{}, .ptr_class = pointer_class::instance, .qual = qualifier::constant};
        std::string const& interface_name = as< builtin_symbol >(input).name;
        if (interface_name == "COPYABLE_INTERFACE")
        {
            append_function("COPY_CONSTRUCT", {ast2_function_parameter{.name = "GENERIC_OTHER", .api_name = "GENERIC_OTHER", .type = constant_pointer}}, mutable_pointer);
            append_function("MOVE_CONSTRUCT", {ast2_function_parameter{.name = "GENERIC_OTHER", .api_name = "GENERIC_OTHER", .type = ptrref_type{.target = void_type{}, .ptr_class = pointer_class::instance, .qual = qualifier::temp}}}, mutable_pointer);
        }
        else if (interface_name == "COMPARABLE_INTERFACE")
        {
            append_function("COMPARE", {ast2_function_parameter{.name = "RHS", .api_name = "RHS", .type = constant_pointer}, ast2_function_parameter{.name = "LHS", .api_name = "LHS", .type = constant_pointer}}, builtin_symbol{"ORDER"});
            append_function("COMPARE_EQ", {ast2_function_parameter{.name = "RHS", .api_name = "RHS", .type = constant_pointer}, ast2_function_parameter{.name = "LHS", .api_name = "LHS", .type = constant_pointer}}, bool_type{});
        }
        else
        {
            append_function("CURRENT_TYPE", {}, type_index_type{});
            append_function("DELETE", {ast2_function_parameter{.name = "GENERIC_THIS", .api_name = "GENERIC_THIS", .type = mutable_pointer}}, void_type{});
        }
        co_return interface_declaration;
    }

    if (input.type_is< subsymbol >() || input.type_is< submember >())
    {
        type_symbol parent = *type_parent(input);
        ast2_symboid const& parent_declaration = co_await rpnx::querygraph::request< symboid_query >(parent);
        if (parent_declaration.type_is< ast2_generic_declaration >())
        {
            if (input.type_is< subsymbol >() && input.get_as< subsymbol >().name == "__INTERFACE")
            {
                co_return co_await rpnx::querygraph::request< generic_interface_query >(parent);
            }
            if (input.type_is< submember >())
            {
                std::vector< struct_field_declaration > const& fields = co_await rpnx::querygraph::request< struct_field_declaration_list_query >(parent);
                for (struct_field_declaration const& field : fields)
                {
                    if (field.name == input.get_as< submember >().name)
                    {
                        co_return ast2_variable_declaration{.type = field.type};
                    }
                }
            }
        }
    }

    auto declarations = co_await rpnx::querygraph::request< declarations_query >(input);
    machine_target_info const machine_info = co_await rpnx::querygraph::request< machine_info_query >(std::monostate{});

    ast2_symboid output;

    for (auto& decl : declarations)
    {
        if (decl.type_is< ast2_asm_procedure_declaration >() && !asm_procedure_matches_target(decl.get_as< ast2_asm_procedure_declaration >(), machine_info))
        {
            continue;
        }
        merge_entity(output, decl);
    }

    co_return output;
}
