// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/struct_virtual_slots_spec.hpp>

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/data/contextual_type_reference.hpp>
#include <quxlang/keywords.hpp>

#include <algorithm>
#include <functional>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>

rpnx::querygraph::coroutine< quxlang::struct_virtual_slots_spec > quxlang::struct_virtual_slots_impl(type_symbol input)
{
    struct normalized_virtual_declaration
    {
        type_symbol owner;
        type_symbol declaration_symbol;
        struct_virtual_signature signature;
        type_symbol return_type;
        std::optional< ast2_virtual_specifier > specifier;
        bool is_nonvirtual = false;
        bool is_destructor = false;
        bool has_forbidden_virtual_features = false;
        std::optional< source_location > location;
    };

    struct_inheritance_info inheritance = co_await rpnx::querygraph::request< struct_inheritance_info_query >(input);
    std::set< type_symbol > hierarchy_types;
    for (struct_subobject_record const& subobject : inheritance.subobjects)
    {
        hierarchy_types.insert(subobject.type);
    }

    std::map< type_symbol, std::vector< struct_base_declaration > > direct_bases;
    std::map< type_symbol, struct_polymorphism_kind > polymorphism;
    std::map< type_symbol, std::vector< normalized_virtual_declaration > > declarations;
    std::map< type_symbol, bool > has_destructor_declaration;

    for (type_symbol const& hierarchy_type : hierarchy_types)
    {
        direct_bases.emplace(hierarchy_type, co_await rpnx::querygraph::request< struct_direct_bases_query >(hierarchy_type));
        std::set< std::string > tags = co_await rpnx::querygraph::request< struct_tags_query >(hierarchy_type);
        if (tags.contains(keywords::virtual_polymorphic))
        {
            polymorphism.emplace(hierarchy_type, struct_polymorphism_kind::virtual_polymorphic);
        }
        else if (tags.contains(keywords::polymorphic))
        {
            polymorphism.emplace(hierarchy_type, struct_polymorphism_kind::polymorphic);
        }
        else
        {
            polymorphism.emplace(hierarchy_type, struct_polymorphism_kind::none);
        }

        std::vector< subdeclaroid > active_declarations = co_await rpnx::querygraph::request< active_symboid_subdeclaroids_query >(hierarchy_type);
        std::map< std::string, std::size_t > overload_ordinals;
        bool has_destructor = false;
        for (subdeclaroid const& declaration_entry : active_declarations)
        {
            if (!declaration_entry.type_is< member_subdeclaroid >())
            {
                continue;
            }
            member_subdeclaroid const& member = declaration_entry.get_as< member_subdeclaroid >();
            if (member.decl.type_is< ast2_template_declaration >())
            {
                ast2_template_declaration const& function_template = member.decl.get_as< ast2_template_declaration >();
                if (function_template.m_declaroid.type_is< ast2_function_declaration >())
                {
                    ast2_function_declaration const& function = function_template.m_declaroid.get_as< ast2_function_declaration >();
                    if (function.header.virtual_specifier.has_value() || function.header.is_nonvirtual)
                    {
                        throw semantic_compilation_error("Virtual functions and NONVIRTUAL destructors cannot be templates: " + to_string(submember{hierarchy_type, member.name}) + source_location_suffix(member.location));
                    }
                }
                continue;
            }
            if (!member.decl.type_is< ast2_function_declaration >())
            {
                continue;
            }

            ast2_function_declaration const& function = member.decl.get_as< ast2_function_declaration >();
            std::size_t overload_ordinal = overload_ordinals[member.name]++;
            bool is_destructor = member.name == "DESTRUCTOR";
            bool is_constructor = member.name == "CONSTRUCTOR" || member.name == "FULLOBJECT_CONSTRUCTOR" || member.name == "SUBOBJECT_CONSTRUCTOR";
            if (is_constructor && !function.header.virtual_specifier.has_value() && !function.header.is_nonvirtual)
            {
                continue;
            }

            bool participates_in_slot_analysis = function.header.virtual_specifier.has_value() || function.header.is_nonvirtual || (is_destructor && polymorphism.at(hierarchy_type) != struct_polymorphism_kind::none) || polymorphism.at(hierarchy_type) != struct_polymorphism_kind::none;
            if (!participates_in_slot_analysis)
            {
                continue;
            }

            struct_virtual_signature signature;
            signature.name = member.name;
            signature.this_parameter = is_destructor ? type_symbol(dvalue_slot{.target = thistype{}}) : type_symbol(ptrref_type{.target = thistype{}, .ptr_class = pointer_class::ref, .qual = qualifier::auto_});
            bool has_forbidden_virtual_features = function.header.priority.has_value();
            instatype enable_if_parameters;
            for (ast2_function_parameter const& parameter : function.header.call_parameters)
            {
                std::optional< type_symbol > canonical_parameter = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                    .context = hierarchy_type,
                    .type = parameter.type,
                });
                if (!canonical_parameter.has_value())
                {
                    throw semantic_compilation_error("Virtual function parameter type could not be resolved: " + to_string(parameter.type) + source_location_suffix(parameter.location));
                }
                if (parameter.is_pack || parameter.default_expr.has_value())
                {
                    has_forbidden_virtual_features = true;
                }
                type_symbol enable_if_parameter_type = *canonical_parameter;
                if (parameter.api_name == std::optional< std::string >{"THIS"} && typeis< ptrref_type >(enable_if_parameter_type) && typeis< thistype >(as< ptrref_type >(enable_if_parameter_type).target))
                {
                    as< ptrref_type >(enable_if_parameter_type).target = hierarchy_type;
                }
                if (parameter.api_name == std::optional< std::string >{"THIS"})
                {
                    signature.this_parameter = std::move(*canonical_parameter);
                }
                else if (parameter.api_name.has_value())
                {
                    signature.named_parameters.emplace(*parameter.api_name, std::move(*canonical_parameter));
                }
                else
                {
                    signature.positional_parameters.push_back(std::move(*canonical_parameter));
                }
                if (parameter.api_name.has_value())
                {
                    enable_if_parameters.named.emplace(*parameter.api_name, make_type_instantiation(std::move(enable_if_parameter_type)));
                }
                else
                {
                    enable_if_parameters.positional.push_back(make_type_instantiation(std::move(enable_if_parameter_type)));
                }
            }

            temploid_reference declaration_symbol{
                .templexoid = submember{hierarchy_type, member.name},
                .overload_id = overload_ordinal,
            };
            if (function.header.enable_if.has_value())
            {
                constexpr_input enable_if_input{
                    .expr = *function.header.enable_if,
                    .context = instanciation_reference{
                        .temploid = declaration_symbol,
                        .params = std::move(enable_if_parameters),
                    },
                };
                if (!(co_await rpnx::querygraph::request< constexpr_bool_query >(std::move(enable_if_input))))
                {
                    continue;
                }
            }
            if (function.header.is_nonvirtual && !is_destructor)
            {
                throw semantic_compilation_error("NONVIRTUAL is permitted only on a destructor: " + to_string(submember{hierarchy_type, member.name}) + source_location_suffix(function.location));
            }
            if (function.header.is_nonvirtual && polymorphism.at(hierarchy_type) == struct_polymorphism_kind::none)
            {
                throw semantic_compilation_error("NONVIRTUAL requires a polymorphic struct: " + to_string(hierarchy_type) + source_location_suffix(function.location));
            }
            if (function.header.virtual_specifier.has_value() && is_constructor)
            {
                throw semantic_compilation_error("Constructors cannot be virtual: " + to_string(submember{hierarchy_type, member.name}) + source_location_suffix(function.location));
            }
            if (is_constructor)
            {
                continue;
            }
            if (function.header.virtual_specifier.has_value() && polymorphism.at(hierarchy_type) == struct_polymorphism_kind::none)
            {
                throw semantic_compilation_error("VIRTUAL requires POLYMORPHIC or VIRTUAL_POLYMORPHIC on " + to_string(hierarchy_type) + source_location_suffix(function.location));
            }
            if (is_destructor && function.header.virtual_specifier.has_value() && function.header.virtual_specifier->is_pure)
            {
                throw semantic_compilation_error("A destructor cannot be VIRTUAL(PURE): " + to_string(hierarchy_type) + source_location_suffix(function.location));
            }
            if (function.header.virtual_specifier.has_value() && has_forbidden_virtual_features)
            {
                throw semantic_compilation_error("Virtual functions cannot use packs, default arguments, or overload priority: " + to_string(submember{hierarchy_type, member.name}) + source_location_suffix(function.location));
            }
            if (function.header.virtual_specifier.has_value() && typeis< ptrref_type >(signature.this_parameter) && as< ptrref_type >(signature.this_parameter).ptr_class == pointer_class::ref)
            {
                qualifier const this_qualifier = as< ptrref_type >(signature.this_parameter).qual;
                if (this_qualifier == qualifier::auto_ || this_qualifier == qualifier::input || this_qualifier == qualifier::output)
                {
                    throw semantic_compilation_error("Virtual functions require an explicit concrete THIS qualifier: " + to_string(submember{hierarchy_type, member.name}) + source_location_suffix(function.location));
                }
            }

            type_symbol declared_return = function.definition.return_type.value_or(type_symbol(void_type{}));
            std::optional< type_symbol > canonical_return = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                .context = hierarchy_type,
                .type = std::move(declared_return),
            });
            if (!canonical_return.has_value())
            {
                throw semantic_compilation_error("Virtual function return type could not be resolved: " + to_string(submember{hierarchy_type, member.name}) + source_location_suffix(function.location));
            }

            declarations[hierarchy_type].push_back(normalized_virtual_declaration{
                .owner = hierarchy_type,
                .declaration_symbol = std::move(declaration_symbol),
                .signature = std::move(signature),
                .return_type = std::move(*canonical_return),
                .specifier = function.header.virtual_specifier,
                .is_nonvirtual = function.header.is_nonvirtual,
                .is_destructor = is_destructor,
                .has_forbidden_virtual_features = has_forbidden_virtual_features,
                .location = function.location,
            });
            if (is_destructor)
            {
                if (has_destructor)
                {
                    throw semantic_compilation_error("A STRUCT cannot declare more than one destructor: " + to_string(hierarchy_type));
                }
                has_destructor = true;
            }
        }
        has_destructor_declaration.emplace(hierarchy_type, has_destructor);
    }

    std::map< type_symbol, struct_virtual_slots > slots_by_type;
    std::set< type_symbol > processing;
    std::function< void(type_symbol const&) > normalize_type;
    normalize_type = [&](type_symbol const& current_type)
    {
        if (slots_by_type.contains(current_type))
        {
            return;
        }
        if (!processing.insert(current_type).second)
        {
            throw compiler_bug("Virtual slot normalization encountered a validated inheritance cycle");
        }

        struct_virtual_slots current_output;
        for (struct_base_declaration const& base : direct_bases.at(current_type))
        {
            normalize_type(base.base_type);
            struct_virtual_slots const& base_slots = slots_by_type.at(base.base_type);
            for (struct_virtual_slot const& base_slot : base_slots.slots)
            {
                std::vector< struct_virtual_slot >::iterator current_slot = std::find_if(current_output.slots.begin(), current_output.slots.end(), [&](struct_virtual_slot const& candidate)
                {
                    return candidate.key == base_slot.key;
                });
                if (current_slot == current_output.slots.end())
                {
                    current_output.slots.push_back(struct_virtual_slot{
                        .key = base_slot.key,
                        .return_type = base_slot.return_type,
                        .overriders = {},
                    });
                    current_slot = std::prev(current_output.slots.end());
                }

                for (struct_virtual_overrider const& base_overrider : base_slot.overriders)
                {
                    struct_virtual_overrider shifted = base_overrider;
                    if (!base_overrider.source_subobject.virtual_root.has_value())
                    {
                        if (base.kind == inheritance_kind::virtual_)
                        {
                            shifted.source_subobject.virtual_root = base.base_type;
                        }
                        shifted.source_subobject.nonvirtual_path.insert(shifted.source_subobject.nonvirtual_path.begin(), base.declaration_ordinal);
                        if (base.kind == inheritance_kind::virtual_)
                        {
                            shifted.source_subobject.nonvirtual_path.erase(shifted.source_subobject.nonvirtual_path.begin());
                        }
                    }
                    for (struct_subobject_path& source_path : shifted.source_paths)
                    {
                        source_path.steps.insert(source_path.steps.begin(), struct_subobject_path_step{
                            .direct_base_ordinal = base.declaration_ordinal,
                            .kind = base.kind,
                            .base_type = base.base_type,
                        });
                    }

                    std::vector< struct_virtual_overrider >::iterator existing = std::find_if(current_slot->overriders.begin(), current_slot->overriders.end(), [&](struct_virtual_overrider const& candidate)
                    {
                        return candidate.source_subobject == shifted.source_subobject && candidate.final_overrider == shifted.final_overrider;
                    });
                    if (existing == current_slot->overriders.end())
                    {
                        current_slot->overriders.push_back(std::move(shifted));
                    }
                    else
                    {
                        for (struct_subobject_path& path : shifted.source_paths)
                        {
                            if (std::find(existing->source_paths.begin(), existing->source_paths.end(), path) == existing->source_paths.end())
                            {
                                existing->source_paths.push_back(std::move(path));
                            }
                        }
                    }
                }
            }
        }

        std::vector< normalized_virtual_declaration > direct_declarations = declarations[current_type];
        bool has_user_destructor = has_destructor_declaration.at(current_type);
        if (polymorphism.at(current_type) != struct_polymorphism_kind::none && !has_user_destructor)
        {
            direct_declarations.push_back(normalized_virtual_declaration{
                .owner = current_type,
                .declaration_symbol = submember{current_type, "DESTRUCTOR"},
                .signature = struct_virtual_signature{
                    .name = "DESTRUCTOR",
                    .this_parameter = dvalue_slot{.target = thistype{}},
                },
                .return_type = void_type{},
                .is_destructor = true,
            });
        }

        for (normalized_virtual_declaration const& declaration : direct_declarations)
        {
            std::vector< struct_virtual_slot* > matching_slots;
            for (struct_virtual_slot& slot : current_output.slots)
            {
                if ((declaration.is_destructor && slot.key.signature.name == "DESTRUCTOR") || (!declaration.is_destructor && slot.key.signature == declaration.signature))
                {
                    matching_slots.push_back(&slot);
                }
            }

            if (declaration.is_nonvirtual)
            {
                current_output.destructor_policy = struct_destructor_policy::nonvirtual;
                if (!matching_slots.empty())
                {
                    throw semantic_compilation_error("NONVIRTUAL destructor would override an inherited virtual destructor: " + to_string(current_type) + source_location_suffix(declaration.location));
                }
                continue;
            }

            if (declaration.has_forbidden_virtual_features && !matching_slots.empty())
            {
                throw semantic_compilation_error("A declaration matching an inherited virtual slot cannot use packs, default arguments, or overload priority: " + to_string(declaration.declaration_symbol) + source_location_suffix(declaration.location));
            }

            bool declares_virtual = declaration.specifier.has_value() || (declaration.is_destructor && polymorphism.at(current_type) != struct_polymorphism_kind::none);
            if (!declares_virtual)
            {
                if (!matching_slots.empty())
                {
                    throw semantic_compilation_error("A declaration matching an inherited virtual slot must use VIRTUAL(OVERRIDE): " + to_string(declaration.declaration_symbol) + source_location_suffix(declaration.location));
                }
                continue;
            }

            bool explicit_override = declaration.specifier.has_value() && declaration.specifier->is_override;
            bool implicit_destructor_override = declaration.is_destructor && !declaration.specifier.has_value();
            if (!implicit_destructor_override)
            {
                if (explicit_override && matching_slots.empty())
                {
                    throw semantic_compilation_error("VIRTUAL(OVERRIDE) does not match an inherited slot: " + to_string(declaration.declaration_symbol) + source_location_suffix(declaration.location));
                }
                if (!explicit_override && !matching_slots.empty())
                {
                    throw semantic_compilation_error("An inherited virtual slot must be overridden with VIRTUAL(OVERRIDE): " + to_string(declaration.declaration_symbol) + source_location_suffix(declaration.location));
                }
            }

            bool is_final = declaration.specifier.has_value() && declaration.specifier->is_final;
            bool is_pure = declaration.specifier.has_value() && declaration.specifier->is_pure;
            if (matching_slots.empty())
            {
                current_output.slots.push_back(struct_virtual_slot{
                    .key = struct_virtual_slot_key{
                        .introducing_declaration = declaration.declaration_symbol,
                        .signature = declaration.signature,
                    },
                    .return_type = declaration.return_type,
                    .overriders = {struct_virtual_overrider{
                        .source_subobject = {},
                        .source_paths = {struct_subobject_path{}},
                        .final_overrider = declaration.declaration_symbol,
                        .is_final = is_final,
                        .is_pure = is_pure,
                    }},
                });
                continue;
            }

            for (struct_virtual_slot* matching_slot : matching_slots)
            {
                if (matching_slot->return_type != declaration.return_type)
                {
                    throw semantic_compilation_error("Virtual override return type must match exactly: " + to_string(declaration.declaration_symbol) + source_location_suffix(declaration.location));
                }
                for (struct_virtual_overrider const& inherited_overrider : matching_slot->overriders)
                {
                    if (inherited_overrider.is_final)
                    {
                        throw semantic_compilation_error("Cannot override a VIRTUAL(FINAL) slot with " + to_string(declaration.declaration_symbol) + source_location_suffix(declaration.location));
                    }
                }

                std::map< struct_subobject_id, std::vector< struct_subobject_path > > source_paths;
                for (struct_virtual_overrider const& inherited_overrider : matching_slot->overriders)
                {
                    std::vector< struct_subobject_path >& paths = source_paths[inherited_overrider.source_subobject];
                    for (struct_subobject_path const& path : inherited_overrider.source_paths)
                    {
                        if (std::find(paths.begin(), paths.end(), path) == paths.end())
                        {
                            paths.push_back(path);
                        }
                    }
                }
                matching_slot->overriders.clear();
                for (std::pair< struct_subobject_id const, std::vector< struct_subobject_path > >& source : source_paths)
                {
                    matching_slot->overriders.push_back(struct_virtual_overrider{
                        .source_subobject = source.first,
                        .source_paths = std::move(source.second),
                        .final_overrider = declaration.declaration_symbol,
                        .is_final = is_final,
                        .is_pure = is_pure,
                    });
                }
            }
        }

        for (struct_virtual_slot& slot : current_output.slots)
        {
            std::map< struct_subobject_id, type_symbol > selected_overriders;
            for (struct_virtual_overrider const& overrider : slot.overriders)
            {
                std::map< struct_subobject_id, type_symbol >::iterator selected = selected_overriders.find(overrider.source_subobject);
                if (selected != selected_overriders.end() && selected->second != overrider.final_overrider)
                {
                    throw semantic_compilation_error("Inheritance produces competing final overriders for " + to_string(slot.key.introducing_declaration) + " in " + to_string(current_type));
                }
                selected_overriders[overrider.source_subobject] = overrider.final_overrider;
                current_output.is_abstract = current_output.is_abstract || overrider.is_pure;
            }
        }
        for (std::size_t ordinal = 0; ordinal < current_output.slots.size(); ++ordinal)
        {
            current_output.slots.at(ordinal).slot_ordinal = ordinal;
        }

        processing.erase(current_type);
        slots_by_type.emplace(current_type, std::move(current_output));
    };
    normalize_type(input);
    co_return slots_by_type.at(input);
}
