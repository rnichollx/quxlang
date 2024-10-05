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
    else if (type.type() == boost::typeindex::type_id< subsymbol >())
    {
        subsymbol const& sub = as< subsymbol >(type);

        type_symbol const& parent = sub.of;

        if (parent.type() == boost::typeindex::type_id< context_reference >())
        {
            std::optional< type_symbol > current_context = context;
            assert(current_context.has_value());
            assert(!qualified_is_contextual(current_context.value()));

            while (current_context.has_value())
            {
                std::string name = sub.name;
                if (typeis< instantiation_type >(*current_context))
                {
                    QUXLANG_DEBUG({
                        std::cout << "Instanciation:  within " << to_string(*current_context) << " check  " << to_string(type) << std::endl;

                    });

                    // Two possibilities, 1 = this is a template, 2 = this is a function
                    instantiation_type inst = as< instantiation_type >(*current_context);

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
                    subsymbol sub2{current_context.value(), sub.name};

                    auto exists = co_await QUX_CO_DEP(entity_canonical_chain_exists, (sub2));

                    QUXLANG_DEBUG({ std::cout << "Exists? " << to_string(sub2) << ": " << (exists ? "yes" : "no") << std::endl; });

                    if (exists)
                    {
                        co_return sub2;
                    }
                }

                current_context = qualified_parent(current_context.value());
            }

            std::string str = "Could not find '" + sub.name + "'";
            throw std::logic_error(str.c_str());
        }

        auto parent_canonical = co_await QUX_CO_DEP(canonical_symbol_from_contextual_symbol, (parent, context));

        co_return subsymbol{parent_canonical, sub.name};
    }
    else if (type.type() == boost::typeindex::type_id< submember >())
    {
        submember const& sub = as< submember >(type);

        type_symbol const& parent = sub.of;

        if (parent.type() == boost::typeindex::type_id< context_reference >())
        {
            std::optional< type_symbol > current_context = context;
            assert(current_context.has_value());
            assert(!qualified_is_contextual(current_context.value()));

            while (current_context.has_value())
            {
                submember sub2{current_context.value(), sub.name};

                auto exists = co_await QUX_CO_DEP(entity_canonical_chain_exists, (sub2));

                QUXLANG_DEBUG({ std::cout << "Exists? " << to_string(sub2) << ": " << (exists ? "yes" : "no") << std::endl; });

                if (exists)
                {
                    co_return sub2;
                }

                current_context = qualified_parent(current_context.value());
            }

            std::string str = "Could not find '" + sub.name + "'";
            throw std::logic_error(str.c_str());
        }

        auto parent_canonical = co_await QUX_CO_DEP(canonical_symbol_from_contextual_symbol, (parent, context));

        co_return submember{parent_canonical, sub.name};
    }
    else if (type.type() == boost::typeindex::type_id< instantiation_type >())
    {
        instantiation_type const& param_set = as< instantiation_type >(type);

        instantiation_type output;

        auto callee_canonical = co_await QUX_CO_DEP(canonical_symbol_from_contextual_symbol, (param_set.callee, context));

        output.callee = callee_canonical;

        for (auto& p : param_set.parameters.positional)
        {
            auto param_canonical = co_await QUX_CO_DEP(canonical_symbol_from_contextual_symbol, (p, context));

            output.parameters.positional.push_back(param_canonical);
            // TODO: support named parameters
        }

        co_return output;
    }
    else if (type.type() == boost::typeindex::type_id< int_type >())
    {
        co_return type;
    }
    else if (type.type() == boost::typeindex::type_id< bool_type >())
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