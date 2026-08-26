// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/struct_constructor_forms_spec.hpp>

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/data/contextual_type_reference.hpp>
#include <quxlang/manipulators/typeutils.hpp>

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <utility>

rpnx::querygraph::coroutine< quxlang::struct_constructor_forms_spec > quxlang::struct_constructor_forms_impl(type_symbol input)
{
    struct parsed_constructor
    {
        temploid_ensig signature;
        ast2_function_declaration declaration;
        std::optional< source_location > location;
    };

    struct_inheritance_info inheritance = co_await rpnx::querygraph::request< struct_inheritance_info_query >(input);
    std::vector< parsed_constructor > constructor_templates;
    std::map< temploid_ensig, parsed_constructor > full_declarations;
    std::map< temploid_ensig, parsed_constructor > subobject_declarations;
    std::vector< subdeclaroid > declarations = co_await rpnx::querygraph::request< active_symboid_subdeclaroids_query >(input);

    for (subdeclaroid const& declaration_entry : declarations)
    {
        if (!declaration_entry.type_is< member_subdeclaroid >())
        {
            continue;
        }
        member_subdeclaroid const& member = declaration_entry.get_as< member_subdeclaroid >();
        bool is_template = member.name == "CONSTRUCTOR";
        bool is_full = member.name == "FULLOBJECT_CONSTRUCTOR";
        bool is_subobject = member.name == "SUBOBJECT_CONSTRUCTOR";
        if ((!is_template && !is_full && !is_subobject) || !member.decl.type_is< ast2_function_declaration >())
        {
            continue;
        }

        ast2_function_declaration const& declaration = member.decl.get_as< ast2_function_declaration >();
        temploid_ensig signature;
        signature.priority = declaration.header.priority;
        signature.enable_if = declaration.header.enable_if;
        bool has_this = false;
        for (ast2_function_parameter const& parameter : declaration.header.call_parameters)
        {
            std::optional< type_symbol > parameter_type = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                .context = input,
                .type = parameter.type,
            });
            if (!parameter_type.has_value())
            {
                throw semantic_compilation_error("Constructor parameter type could not be resolved: " + to_string(parameter.type) + source_location_suffix(parameter.location));
            }
            argif parameter_interface{
                .type = std::move(*parameter_type),
                .is_defaulted = parameter.default_expr.has_value(),
                .is_pack = parameter.is_pack,
            };
            if (parameter.api_name.has_value())
            {
                if (!signature.interface.named.emplace(*parameter.api_name, std::move(parameter_interface)).second)
                {
                    throw semantic_compilation_error("Duplicate constructor parameter name '" + *parameter.api_name + "'" + source_location_suffix(parameter.location));
                }
                has_this = has_this || *parameter.api_name == "THIS";
            }
            else
            {
                signature.interface.positional.push_back(std::move(parameter_interface));
            }
        }
        if (!has_this)
        {
            signature.interface.named.emplace("THIS", argif{.type = nvalue_slot{.target = thistype{}}});
        }
        signature = strip_source_locations(std::move(signature));

        parsed_constructor parsed{
            .signature = signature,
            .declaration = declaration,
            .location = member.location,
        };
        if (is_template)
        {
            constructor_templates.push_back(std::move(parsed));
        }
        else if (is_full)
        {
            if (!full_declarations.emplace(signature, std::move(parsed)).second)
            {
                throw semantic_compilation_error("Duplicate FULLOBJECT_CONSTRUCTOR signature in " + to_string(input) + source_location_suffix(member.location));
            }
        }
        else if (!subobject_declarations.emplace(signature, std::move(parsed)).second)
        {
            throw semantic_compilation_error("Duplicate SUBOBJECT_CONSTRUCTOR signature in " + to_string(input) + source_location_suffix(member.location));
        }
    }

    struct_constructor_forms output;
    output.uses_split_abi = inheritance.polymorphism == struct_polymorphism_kind::virtual_polymorphic;
    if (!output.uses_split_abi)
    {
        if (!full_declarations.empty() || !subobject_declarations.empty())
        {
            throw semantic_compilation_error("FULLOBJECT_CONSTRUCTOR and SUBOBJECT_CONSTRUCTOR require VIRTUAL_POLYMORPHIC: " + to_string(input));
        }
        co_return output;
    }

    std::set< temploid_ensig > claimed_signatures;
    for (parsed_constructor const& constructor_template : constructor_templates)
    {
        if (!claimed_signatures.insert(constructor_template.signature).second || full_declarations.contains(constructor_template.signature) || subobject_declarations.contains(constructor_template.signature))
        {
            throw semantic_compilation_error("CONSTRUCTOR template collides with another constructor form in " + to_string(input) + source_location_suffix(constructor_template.location));
        }
        ast2_function_declaration subobject_declaration = constructor_template.declaration;
        std::erase_if(subobject_declaration.definition.delegates, [](ast2_function_delegate const& delegate)
        {
            return delegate.kind == function_delegate_kind::virtual_base;
        });
        output.forms.push_back(struct_constructor_form{
            .normalized_signature = constructor_template.signature,
            .origin = constructor_form_origin::constructor_template,
            .full_declaration = constructor_template.declaration,
            .subobject_declaration = std::move(subobject_declaration),
        });
    }

    std::set< temploid_ensig > explicit_signatures;
    for (std::pair< temploid_ensig const, parsed_constructor > const& full : full_declarations)
    {
        explicit_signatures.insert(full.first);
    }
    for (std::pair< temploid_ensig const, parsed_constructor > const& subobject : subobject_declarations)
    {
        explicit_signatures.insert(subobject.first);
    }
    for (temploid_ensig const& signature : explicit_signatures)
    {
        std::map< temploid_ensig, parsed_constructor >::const_iterator full = full_declarations.find(signature);
        std::map< temploid_ensig, parsed_constructor >::const_iterator subobject = subobject_declarations.find(signature);
        if (full == full_declarations.end() || subobject == subobject_declarations.end())
        {
            throw semantic_compilation_error("Every explicit VIRTUAL_POLYMORPHIC constructor signature requires both FULLOBJECT_CONSTRUCTOR and SUBOBJECT_CONSTRUCTOR in " + to_string(input));
        }
        for (ast2_function_delegate const& delegate : subobject->second.declaration.definition.delegates)
        {
            if (delegate.kind == function_delegate_kind::virtual_base)
            {
                throw semantic_compilation_error("SUBOBJECT_CONSTRUCTOR cannot initialize a virtual base in " + to_string(input) + source_location_suffix(delegate.location));
            }
        }
        output.forms.push_back(struct_constructor_form{
            .normalized_signature = signature,
            .origin = constructor_form_origin::explicit_pair,
            .full_declaration = full->second.declaration,
            .subobject_declaration = subobject->second.declaration,
        });
    }

    if (output.forms.empty())
    {
        ast2_function_declaration generated_declaration;
        temploid_ensig generated_signature;
        generated_signature.interface.named.emplace("THIS", argif{.type = nvalue_slot{.target = thistype{}}});
        output.forms.push_back(struct_constructor_form{
            .normalized_signature = std::move(generated_signature),
            .origin = constructor_form_origin::compiler_generated,
            .full_declaration = generated_declaration,
            .subobject_declaration = std::move(generated_declaration),
        });
    }

    for (std::size_t ordinal = 0; ordinal < output.forms.size(); ++ordinal)
    {
        output.forms.at(ordinal).full_entry = temploid_reference{
            .templexoid = submember{input, "FULLOBJECT_CONSTRUCTOR"},
            .overload_id = ordinal,
        };
        output.forms.at(ordinal).subobject_entry = temploid_reference{
            .templexoid = submember{input, "SUBOBJECT_CONSTRUCTOR"},
            .overload_id = ordinal,
        };
    }

    co_return output;
}
