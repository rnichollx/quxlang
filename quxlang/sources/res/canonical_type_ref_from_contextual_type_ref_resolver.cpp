// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/compiler.hpp"

#include "quxlang/manipulators/qmanip.hpp"
#include "quxlang/res/canonical_symbol_from_contextual_symbol_resolver.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(canonical_symbol_from_contextual_symbol)
{
    type_symbol context = input.context;
    type_symbol const& type = input.type;

    QUXLANG_DEBUG({
        std::cout << "type lookup," << std::endl;
        std::cout << "With Context: " << to_string(context) << std::endl;
        std::cout << "Looking up type: " << to_string(type) << std::endl;
    });

    if (type.type() == boost::typeindex::type_id< instance_pointer_type >())
    {
        instance_pointer_type const& ptr = as< instance_pointer_type >(type);

        type_symbol to_type = ptr.target;

        // we need to canonicalize the type_reference

        contextual_type_reference to_type_ref;
        to_type_ref.type = to_type;
        to_type_ref.context = input.context;

        auto canon_ptr_to_type = co_await QUX_CO_DEP(canonical_symbol_from_contextual_symbol, (to_type_ref));

        instance_pointer_type canonical_ptr_type;
        canonical_ptr_type.target = canon_ptr_to_type;

        co_return canonical_ptr_type;
    }
    else if (type.type() == boost::typeindex::type_id< subentity_reference >())
    {
        subentity_reference const& sub = as< subentity_reference >(type);

        type_symbol const& parent = sub.parent;

        if (parent.type() == boost::typeindex::type_id< context_reference >())
        {
            std::optional< type_symbol > current_context = context;
            assert(current_context.has_value());
            assert(!qualified_is_contextual(current_context.value()));

            while (current_context.has_value())
            {
                std::string name = sub.subentity_name;
                if (typeis< instanciation_reference >(*current_context))
                {
                    QUXLANG_DEBUG({
                        std::cout << "Instanciation:  within " << to_string(*current_context) << " check  " << to_string(type) << std::endl;

                    });

                    // Two possibilities, 1 = this is a template, 2 = this is a function
                    instanciation_reference inst = as< instanciation_reference >(*current_context);

                    auto param_set = co_await QUX_CO_DEP(temploid_instanciation_parameter_set, (inst));

                    QUXLANG_DEBUG({
                        std::cout << "Param set, name=" << name << std::endl;
                        std::cout << "Param map " << to_string(inst) << " / " << to_string(*current_context) << " " << param_set.parameter_map.size() << std::endl;
                        for (auto it = param_set.parameter_map.begin(); it != param_set.parameter_map.end(); it++)
                        {
                            std::cout << "Param map: k=" << it->first << " v=" << to_string(it->second) << std::endl;
                        }
                    });
                    auto it = param_set.parameter_map.find(name);
                    if (it != param_set.parameter_map.end())
                    {
                        co_return it->second;
                    }
                    current_context = qualified_parent(current_context.value());
                }
                else
                {
                    subentity_reference sub2{current_context.value(), sub.subentity_name};

                    auto exists = co_await QUX_CO_DEP(entity_canonical_chain_exists, (sub2));

                    QUXLANG_DEBUG({ std::cout << "Exists? " << to_string(sub2) << ": " << (exists ? "yes" : "no") << std::endl; });

                    if (exists)
                    {
                        co_return sub2;
                    }
                }

                current_context = qualified_parent(current_context.value());
            }

            std::string str = "Could not find '" + sub.subentity_name + "'";
            throw std::logic_error(str.c_str());
        }

        auto parent_canonical = co_await QUX_CO_DEP(canonical_symbol_from_contextual_symbol, (parent, context));

        co_return subentity_reference{parent_canonical, sub.subentity_name};
    }
    else if (type.type() == boost::typeindex::type_id< subdotentity_reference >())
    {
        subdotentity_reference const& sub = as< subdotentity_reference >(type);

        type_symbol const& parent = sub.parent;

        if (parent.type() == boost::typeindex::type_id< context_reference >())
        {
            std::optional< type_symbol > current_context = context;
            assert(current_context.has_value());
            assert(!qualified_is_contextual(current_context.value()));

            while (current_context.has_value())
            {
                subdotentity_reference sub2{current_context.value(), sub.subdotentity_name};

                auto exists = co_await QUX_CO_DEP(entity_canonical_chain_exists, (sub2));

                QUXLANG_DEBUG({ std::cout << "Exists? " << to_string(sub2) << ": " << (exists ? "yes" : "no") << std::endl; });

                if (exists)
                {
                    co_return sub2;
                }

                current_context = qualified_parent(current_context.value());
            }

            std::string str = "Could not find '" + sub.subdotentity_name + "'";
            throw std::logic_error(str.c_str());
        }

        auto parent_canonical = co_await QUX_CO_DEP(canonical_symbol_from_contextual_symbol, (parent, context));

        co_return subdotentity_reference{parent_canonical, sub.subdotentity_name};
    }
    else if (type.type() == boost::typeindex::type_id< instanciation_reference >())
    {
        instanciation_reference const& param_set = as< instanciation_reference >(type);

        instanciation_reference output;

        auto callee_canonical = co_await QUX_CO_DEP(canonical_symbol_from_contextual_symbol, (param_set.callee, context));

        output.callee = callee_canonical;

        for (auto& p : param_set.parameters.positional_parameters)
        {
            auto param_canonical = co_await QUX_CO_DEP(canonical_symbol_from_contextual_symbol, (p, context));

            output.parameters.positional_parameters.push_back(param_canonical);
            // TODO: support named parameters
        }

        co_return output;
    }
    else if (type.type() == boost::typeindex::type_id< primitive_type_integer_reference >())
    {
        co_return type;
    }
    else if (type.type() == boost::typeindex::type_id< primitive_type_bool_reference >())
    {
        co_return type;
    }
    else if (type.type() == boost::typeindex::type_id< module_reference >())
    {
        co_return type;
    }
    else if (type.type() == boost::typeindex::type_id< mvalue_reference >())
    {
        auto target_type = as< mvalue_reference >(type).target;
        auto target_canonical = co_await QUX_CO_DEP(canonical_symbol_from_contextual_symbol, (target_type, context));
        co_return mvalue_reference{target_canonical};
    }
    else if (typeis< void_type >(type))
    {
        co_return type;
    }
    else if (typeis< template_reference >(type))
    {
        co_return type;
    }
    else
    {
        std::string str = std::string() + "unimplemented: " + type.type().name();
        QUXLANG_DEBUG({ std::cout << str << std::endl; });
        throw std::logic_error("unreachable/unimplemented");
    }
}