// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/interface_slot_list_spec.hpp>

#include <quxlang/manipulators/typeutils.hpp>

#include <set>

rpnx::querygraph::coroutine< quxlang::interface_slot_list_spec > quxlang::interface_slot_list_impl(type_symbol input)
{
    auto strip_locations = [](invotype value) -> invotype {
        for (type_symbol& param : value.positional)
        {
            param = strip_source_locations(std::move(param));
        }
        for (std::pair< std::string const, type_symbol >& entry : value.named)
        {
            entry.second = strip_source_locations(std::move(entry.second));
        }
        return value;
    };

    auto sym = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_interface_declaration >(sym))
    {
        throw quxlang::compiler_bug("Cannot list interface slots of non-interface: " + quxlang::to_string(input));
    }

    ast2_interface_declaration const& interface_decl = as< ast2_interface_declaration >(sym);
    std::vector< interface_slot > output;
    std::set< interface_slot_key > seen_slots;

    for (ast2_interface_function_declaration const& function : interface_decl.functions)
    {
        if (function.header.enable_if.has_value() || function.header.priority.has_value())
        {
            throw quxlang::semantic_compilation_error("Interface functions cannot use overload priority or ENABLE_IF");
        }
        if (!function.definition.delegates.empty())
        {
            throw quxlang::semantic_compilation_error("Interface functions cannot use constructor delegates");
        }

        interface_slot slot;
        slot.declaration = function;
        slot.key.name = function.name;

        for (ast2_function_parameter const& param : function.header.call_parameters)
        {
            if (param.api_name == std::optional< std::string >{"THIS"} || param.name == std::optional< std::string >{"THIS"})
            {
                throw quxlang::semantic_compilation_error("Interface functions cannot declare THIS");
            }
            if (param.is_pack)
            {
                throw quxlang::semantic_compilation_error("Interface functions cannot use variadic packs");
            }
            if (is_template(param.type))
            {
                throw quxlang::semantic_compilation_error("Interface function parameter types must be concrete");
            }

            auto resolved = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = input, .type = param.type});
            if (!resolved.has_value())
            {
                throw quxlang::semantic_compilation_error("Interface function parameter type could not be resolved");
            }
            if (is_template(*resolved))
            {
                throw quxlang::semantic_compilation_error("Interface function parameter types must be concrete");
            }

            if (param.api_name.has_value())
            {
                if (slot.key.concrete_params.named.contains(*param.api_name))
                {
                    throw quxlang::semantic_compilation_error("Duplicate interface function parameter name");
                }
                slot.key.concrete_params.named[*param.api_name] = *resolved;
            }
            else
            {
                slot.key.concrete_params.positional.push_back(*resolved);
            }
        }

        type_symbol declared_return_type = function.definition.return_type.value_or(type_symbol(void_type{}));
        if (is_template(declared_return_type))
        {
            throw quxlang::semantic_compilation_error("Interface function return types must be concrete");
        }
        auto resolved_return_type = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{.context = input, .type = std::move(declared_return_type)});
        if (!resolved_return_type.has_value())
        {
            throw quxlang::semantic_compilation_error("Interface function return type could not be resolved");
        }
        if (is_template(*resolved_return_type))
        {
            throw quxlang::semantic_compilation_error("Interface function return types must be concrete");
        }
        if (!typeis< void_type >(*resolved_return_type))
        {
            slot.key.concrete_return_type = strip_source_locations(*resolved_return_type);
        }

        slot.key.concrete_params = strip_locations(std::move(slot.key.concrete_params));
        if (seen_slots.contains(slot.key))
        {
            throw quxlang::semantic_compilation_error("Duplicate interface slot: " + slot.key.name);
        }
        seen_slots.insert(slot.key);
        output.push_back(std::move(slot));
    }

    co_return output;
}
