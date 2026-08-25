// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/specs/pseudotype_match_spec.hpp>

namespace quxlang
{
    rpnx::querygraph::coroutine< pseudotype_match_spec > pseudotype_match_impl(pseudotype_match_input input)
    {
        if (!is_canonical(input.type))
        {
            throw compiler_bug("pseudotype_match_query requires an already-canonical candidate type");
        }

        pseudotype_match_result result;

        auto bind_pseudotype = [&](std::string const& name, type_symbol const& type) -> bool
        {
            if (name.empty())
            {
                return true;
            }
            std::map< std::string, type_symbol >::iterator existing = result.matches.find(name);
            if (existing != result.matches.end())
            {
                return existing->second == type;
            }
            result.matches.emplace(name, type);
            return true;
        };

        auto match = [&](auto&& self, type_symbol const& pseudotype_symbol, type_symbol const& candidate_symbol) -> bool
        {
            if (typeis< auto_temploidic >(pseudotype_symbol))
            {
                return bind_pseudotype(as< auto_temploidic >(pseudotype_symbol).name, candidate_symbol);
            }
            if (typeis< decay_temploidic >(pseudotype_symbol))
            {
                type_symbol const* deduced_type = &candidate_symbol;
                if (typeis< ptrref_type >(candidate_symbol))
                {
                    ptrref_type const& reference = as< ptrref_type >(candidate_symbol);
                    if (reference.ptr_class == pointer_class::ref && reference.qual == qualifier::temp)
                    {
                        deduced_type = &reference.target;
                    }
                }
                return bind_pseudotype(as< decay_temploidic >(pseudotype_symbol).name, *deduced_type);
            }
            if (typeis< type_temploidic >(pseudotype_symbol))
            {
                return bind_pseudotype(as< type_temploidic >(pseudotype_symbol).name, candidate_symbol);
            }
            if (typeis< numeric_literal_any_temploidic >(pseudotype_symbol))
            {
                if (!typeis< numeric_literal_type >(candidate_symbol))
                {
                    return false;
                }
                return bind_pseudotype(as< numeric_literal_any_temploidic >(pseudotype_symbol).name, candidate_symbol);
            }
            if (typeis< string_literal_any_temploidic >(pseudotype_symbol))
            {
                if (!typeis< string_literal_type >(candidate_symbol))
                {
                    return false;
                }
                return bind_pseudotype(as< string_literal_any_temploidic >(pseudotype_symbol).name, candidate_symbol);
            }
            if (typeis< thistype >(pseudotype_symbol))
            {
                return true;
            }
            if (pseudotype_symbol.type() != candidate_symbol.type())
            {
                return false;
            }

            if (typeis< ptrref_type >(pseudotype_symbol))
            {
                ptrref_type const& pseudotype = as< ptrref_type >(pseudotype_symbol);
                ptrref_type const& candidate = as< ptrref_type >(candidate_symbol);
                if (pseudotype.ptr_class != candidate.ptr_class)
                {
                    return false;
                }
                bool qualifier_matches = pseudotype.qual == candidate.qual;
                if (pseudotype.qual == qualifier::auto_ || pseudotype.qual == qualifier::input || pseudotype.qual == qualifier::output)
                {
                    qualifier_matches = qualifier_template_match(pseudotype.qual, candidate.qual).has_value();
                }
                if (!qualifier_matches)
                {
                    return false;
                }
                return self(self, pseudotype.target, candidate.target);
            }
            if (typeis< nvalue_slot >(pseudotype_symbol))
            {
                return self(self, as< nvalue_slot >(pseudotype_symbol).target, as< nvalue_slot >(candidate_symbol).target);
            }
            if (typeis< dvalue_slot >(pseudotype_symbol))
            {
                return self(self, as< dvalue_slot >(pseudotype_symbol).target, as< dvalue_slot >(candidate_symbol).target);
            }
            if (typeis< subsymbol >(pseudotype_symbol))
            {
                subsymbol const& pseudotype = as< subsymbol >(pseudotype_symbol);
                subsymbol const& candidate = as< subsymbol >(candidate_symbol);
                if (pseudotype.name != candidate.name)
                {
                    return false;
                }
                return self(self, pseudotype.of, candidate.of);
            }
            if (typeis< subtag_type >(pseudotype_symbol))
            {
                subtag_type const& pseudotype = as< subtag_type >(pseudotype_symbol);
                subtag_type const& candidate = as< subtag_type >(candidate_symbol);
                if (pseudotype.name != candidate.name)
                {
                    return false;
                }
                return self(self, pseudotype.of, candidate.of);
            }
            if (typeis< submember >(pseudotype_symbol))
            {
                submember const& pseudotype = as< submember >(pseudotype_symbol);
                submember const& candidate = as< submember >(candidate_symbol);
                if (pseudotype.name != candidate.name)
                {
                    return false;
                }
                return self(self, pseudotype.of, candidate.of);
            }
            if (typeis< storage >(pseudotype_symbol))
            {
                storage const& pseudotype = as< storage >(pseudotype_symbol);
                storage const& candidate = as< storage >(candidate_symbol);
                if (pseudotype.storable_types.size() != candidate.storable_types.size())
                {
                    return false;
                }
                std::set< type_symbol >::const_iterator pseudotype_element = pseudotype.storable_types.begin();
                std::set< type_symbol >::const_iterator candidate_element = candidate.storable_types.begin();
                for (; pseudotype_element != pseudotype.storable_types.end(); ++pseudotype_element, ++candidate_element)
                {
                    if (!self(self, *pseudotype_element, *candidate_element))
                    {
                        return false;
                    }
                }
                return true;
            }
            if (typeis< array_type >(pseudotype_symbol))
            {
                array_type const& pseudotype = as< array_type >(pseudotype_symbol);
                array_type const& candidate = as< array_type >(candidate_symbol);
                if (pseudotype.element_count != candidate.element_count)
                {
                    return false;
                }
                return self(self, pseudotype.element_type, candidate.element_type);
            }
            if (typeis< array_initializer_type >(pseudotype_symbol))
            {
                array_initializer_type const& pseudotype = as< array_initializer_type >(pseudotype_symbol);
                array_initializer_type const& candidate = as< array_initializer_type >(candidate_symbol);
                if (pseudotype.count != candidate.count)
                {
                    return false;
                }
                return self(self, pseudotype.element_type, candidate.element_type);
            }
            if (typeis< temploid_reference >(pseudotype_symbol))
            {
                temploid_reference const& pseudotype = as< temploid_reference >(pseudotype_symbol);
                temploid_reference const& candidate = as< temploid_reference >(candidate_symbol);
                if (pseudotype.overload_id != candidate.overload_id)
                {
                    return false;
                }
                return self(self, pseudotype.templexoid, candidate.templexoid);
            }

            auto match_parameter = [&](parameter_instantiation const& pseudotype, parameter_instantiation const& candidate) -> bool
            {
                if (pseudotype.type() != candidate.type())
                {
                    return false;
                }
                if (pseudotype.template type_is< parameter_value_instantiation >()
                    && pseudotype.template get_as< parameter_value_instantiation >().value != candidate.template get_as< parameter_value_instantiation >().value)
                {
                    return false;
                }
                return self(self, parameter_instantiation_type(pseudotype), parameter_instantiation_type(candidate));
            };
            auto match_parameters = [&](instatype const& pseudotype, instatype const& candidate) -> bool
            {
                if (pseudotype.named.size() != candidate.named.size() || pseudotype.positional.size() != candidate.positional.size())
                {
                    return false;
                }
                for (auto const& [name, pseudotype_parameter] : pseudotype.named)
                {
                    std::map< std::string, parameter_instantiation >::const_iterator candidate_parameter = candidate.named.find(name);
                    if (candidate_parameter == candidate.named.end())
                    {
                        return false;
                    }
                    if (!match_parameter(pseudotype_parameter, candidate_parameter->second))
                    {
                        return false;
                    }
                }
                for (std::size_t index = 0; index < pseudotype.positional.size(); ++index)
                {
                    if (!match_parameter(pseudotype.positional[index], candidate.positional[index]))
                    {
                        return false;
                    }
                }
                return true;
            };

            if (typeis< instanciation_reference >(pseudotype_symbol))
            {
                instanciation_reference const& pseudotype = as< instanciation_reference >(pseudotype_symbol);
                instanciation_reference const& candidate = as< instanciation_reference >(candidate_symbol);
                if (!self(self, pseudotype.temploid, candidate.temploid))
                {
                    return false;
                }
                if (!match_parameters(pseudotype.params, candidate.params))
                {
                    return false;
                }
                return true;
            }
            if (typeis< initialization_reference >(pseudotype_symbol))
            {
                initialization_reference const& pseudotype = as< initialization_reference >(pseudotype_symbol);
                initialization_reference const& candidate = as< initialization_reference >(candidate_symbol);
                if (pseudotype.arguments != candidate.arguments || pseudotype.adaptations != candidate.adaptations
                    || pseudotype.context.has_value() != candidate.context.has_value())
                {
                    return false;
                }
                if (!self(self, pseudotype.initializee, candidate.initializee))
                {
                    return false;
                }
                if (pseudotype.context.has_value())
                {
                    if (!self(self, *pseudotype.context, *candidate.context))
                    {
                        return false;
                    }
                }
                if (!match_parameters(pseudotype.parameters, candidate.parameters))
                {
                    return false;
                }
                return true;
            }
            if (typeis< attached_type_reference >(pseudotype_symbol))
            {
                attached_type_reference const& pseudotype = as< attached_type_reference >(pseudotype_symbol);
                attached_type_reference const& candidate = as< attached_type_reference >(candidate_symbol);
                if (!self(self, pseudotype.carrying_type, candidate.carrying_type))
                {
                    return false;
                }
                if (!self(self, pseudotype.attached_symbol, candidate.attached_symbol))
                {
                    return false;
                }
                return true;
            }

            auto match_invotype = [&](invotype const& pseudotype, invotype const& candidate) -> bool
            {
                if (pseudotype.named.size() != candidate.named.size() || pseudotype.positional.size() != candidate.positional.size())
                {
                    return false;
                }
                for (auto const& [name, pseudotype_parameter] : pseudotype.named)
                {
                    std::map< std::string, type_symbol >::const_iterator candidate_parameter = candidate.named.find(name);
                    if (candidate_parameter == candidate.named.end())
                    {
                        return false;
                    }
                    if (!self(self, pseudotype_parameter, candidate_parameter->second))
                    {
                        return false;
                    }
                }
                for (std::size_t index = 0; index < pseudotype.positional.size(); ++index)
                {
                    if (!self(self, pseudotype.positional[index], candidate.positional[index]))
                    {
                        return false;
                    }
                }
                return true;
            };

            if (typeis< procedure_type >(pseudotype_symbol))
            {
                procedure_type const& pseudotype = as< procedure_type >(pseudotype_symbol);
                procedure_type const& candidate = as< procedure_type >(candidate_symbol);
                if (pseudotype.calling_convention != candidate.calling_convention || pseudotype.is_noexcept != candidate.is_noexcept
                    || pseudotype.signature.return_type.has_value() != candidate.signature.return_type.has_value())
                {
                    return false;
                }
                if (!match_invotype(pseudotype.signature.params, candidate.signature.params))
                {
                    return false;
                }
                if (pseudotype.signature.return_type.has_value() && candidate.signature.return_type.has_value())
                {
                    if (!self(self, *pseudotype.signature.return_type, *candidate.signature.return_type))
                    {
                        return false;
                    }
                }
                return true;
            }
            if (typeis< static_local_ref >(pseudotype_symbol))
            {
                static_local_ref const& pseudotype = as< static_local_ref >(pseudotype_symbol);
                static_local_ref const& candidate = as< static_local_ref >(candidate_symbol);
                if (pseudotype.name != candidate.name || pseudotype.generation != candidate.generation)
                {
                    return false;
                }
                return self(self, pseudotype.functanoid, candidate.functanoid);
            }
            if (typeis< static_snapshot_ref >(pseudotype_symbol))
            {
                static_snapshot_ref const& pseudotype = as< static_snapshot_ref >(pseudotype_symbol);
                static_snapshot_ref const& candidate = as< static_snapshot_ref >(candidate_symbol);
                if (pseudotype.name != candidate.name || pseudotype.generation != candidate.generation || pseudotype.snapshot_id != candidate.snapshot_id)
                {
                    return false;
                }
                return self(self, pseudotype.functanoid, candidate.functanoid);
            }
            if (typeis< decltype_type_ref >(pseudotype_symbol))
            {
                return self(self, as< decltype_type_ref >(pseudotype_symbol).symbol, as< decltype_type_ref >(candidate_symbol).symbol);
            }
            return pseudotype_symbol == candidate_symbol;
        };

        if (!match(match, input.pseudotype, input.type))
        {
            co_return std::nullopt;
        }
        co_return result;
    }
} // namespace quxlang
