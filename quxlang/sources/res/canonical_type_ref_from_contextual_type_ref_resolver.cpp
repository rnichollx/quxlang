//
// Created by Ryan Nicholl on 10/20/23.
//

#include "quxlang/compiler.hpp"

#include "quxlang/manipulators/qmanip.hpp"
#include "quxlang/res/canonical_symbol_from_contextual_symbol_resolver.hpp"

void quxlang::canonical_symbol_from_contextual_symbol_resolver::process(quxlang::compiler* c)
{

    type_symbol context = m_ref.context;
    type_symbol const& type = m_ref.type;

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
        to_type_ref.context = m_ref.context;

        auto canonical_to_type_dep = get_dependency(
            [&]
            {
                return c->lk_canonical_symbol_from_contextual_symbol(to_type_ref);
            });

        if (!ready())
            return;

        type_symbol canon_ptr_to_type = canonical_to_type_dep->get();

        instance_pointer_type canonical_ptr_type;
        canonical_ptr_type.target = canon_ptr_to_type;

        set_value(canonical_ptr_type);
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
                    QUXLANG_DEBUG({std::cout << "Instanciation:  within "<< to_string(*current_context) << " check  " << to_string(type) << std::endl;});

                    // Two possibilities, 1 = this is a template, 2 = this is a function
                    instanciation_reference inst = as< instanciation_reference >(*current_context);

                    QUXLANG_DEBUG({std::cout << "current inst: " << to_string(inst) << std::endl;});
                    auto param_set_dp = get_dependency(
                        [&]
                        {
                            return c->lk_temploid_instanciation_parameter_set(inst);
                        });
                    if (!ready())
                    {
                        return;
                    }
                    temploid_instanciation_parameter_set param_set = param_set_dp->get();
                    QUXLANG_DEBUG({
                        std::cout << "dep str=" << param_set_dp->question() << " answer: " << param_set_dp->answer() << std::endl;
                        std::cout << "Param set, name=" << name << std::endl;

                        std::cout << "Param map " << to_string(inst) << " / " << to_string(*current_context) << " " << param_set.parameter_map.size() << std::endl;
                        });
                    for (auto it = param_set.parameter_map.begin(); it != param_set.parameter_map.end(); it++)
                    {
                        std::cout << "Param map: k=" << it->first << " v=" << to_string(it->second) << std::endl;
                    }
                    auto it = param_set.parameter_map.find(name);
                    if (it != param_set.parameter_map.end())
                    {
                        set_value(it->second);
                        return;
                    }
                    current_context = qualified_parent(current_context.value());
                }
                else
                {
                    subentity_reference sub2{current_context.value(), sub.subentity_name};

                    auto exists_dp = get_dependency(
                        [&]
                        {
                            return c->lk_entity_canonical_chain_exists(sub2);
                        });
                    if (!ready())
                        return;

                    bool exists = exists_dp->get();
                    QUXLANG_DEBUG({std::cout << "Exists? " << to_string(sub2) << ": " << (exists ? "yes" : "no") << std::endl;});

                    if (exists)
                    {
                        set_value(sub2);
                        return;
                    }
                }

                current_context = qualified_parent(current_context.value());
            }

            std::string str = "Could not find '" + sub.subentity_name + "'";
            throw std::logic_error(str.c_str());
        }

        auto parent_dp = get_dependency(
            [&]
            {
                return c->lk_canonical_symbol_from_contextual_symbol(parent, context);
            });

        if (!ready())
            return;

        set_value(subentity_reference{parent_dp->get(), sub.subentity_name});

        return;
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

                auto exists_dp = get_dependency(
                    [&]
                    {
                        return c->lk_entity_canonical_chain_exists(sub2);
                    });
                if (!ready())
                    return;

                bool exists = exists_dp->get();

                std::cout << "Exists? " << to_string(sub2) << ": " << (exists ? "yes" : "no") << std::endl;

                if (exists)
                {
                    set_value(sub2);
                    return;
                }

                current_context = qualified_parent(current_context.value());
            }

            std::string str = "Could not find '" + sub.subdotentity_name + "'";
            throw std::logic_error(str.c_str());
        }

        auto parent_dp = get_dependency(
            [&]
            {
                return c->lk_canonical_symbol_from_contextual_symbol(parent, context);
            });

        if (!ready())
            return;

        set_value(subdotentity_reference{parent_dp->get(), sub.subdotentity_name});

        return;
    }
    else if (type.type() == boost::typeindex::type_id< instanciation_reference >())
    {
        instanciation_reference const& param_set = as< instanciation_reference >(type);

        instanciation_reference output;

        auto callee_dp = get_dependency(
            [&]
            {
                return c->lk_canonical_symbol_from_contextual_symbol(param_set.callee, context);
            });

        if (!ready())
            return;

        output.callee = callee_dp->get();

        for (auto& p : param_set.parameters)
        {
            auto param_dp = get_dependency(
                [&]
                {
                    return c->lk_canonical_symbol_from_contextual_symbol(p, context);
                });

            if (!ready())
                return;

            output.parameters.push_back(param_dp->get());
        }

        set_value(output);
    }
    else if (type.type() == boost::typeindex::type_id< primitive_type_integer_reference >())
    {
        set_value(type);
    }
    else if (type.type() == boost::typeindex::type_id< primitive_type_bool_reference >())
    {
        set_value(type);
    }
    else if (type.type() == boost::typeindex::type_id< module_reference >())
    {
        set_value(type);
    }
    else if (type.type() == boost::typeindex::type_id< mvalue_reference >())
    {
        auto target_type = as< mvalue_reference >(type).target;
        auto target_can_dp = get_dependency(
            [&]
            {
                return c->lk_canonical_symbol_from_contextual_symbol(target_type, context);
            });
        if (!ready())
        {
            return;
        }
        set_value(mvalue_reference{target_can_dp->get()});
    }
    else if (typeis< void_type >(type))
    {
        set_value(type);
    }
    else if (typeis< template_reference >(type))
    {
        set_value(type);
    }
    else
    {
        std::string str = std::string() + "unimplemented: " + type.type().name();
        std::cout << str << std::endl;
        throw std::logic_error("unreachable/unimplemented");
    }
}