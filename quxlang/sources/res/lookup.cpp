// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/compiler.hpp"

#include "quxlang/manipulators/qmanip.hpp"
#include "quxlang/res/lookup.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(lookup)
{
    type_symbol context = input.context;
    type_symbol const& type = input.type;

    QUXLANG_DEBUG({
        std::cout << "type lookup," << std::endl;
        std::cout << "With Context: " << to_string(context) << std::endl;
        std::cout << "Looking up type: " << to_string(type) << std::endl;
    });

    if (type.template type_is< pointer_type >())
    {
        pointer_type const& ptr = as< pointer_type >(type);

        type_symbol to_type = ptr.target;

        // we need to canonicalize the type_reference

        contextual_type_reference to_type_ref;
        to_type_ref.type = to_type;
        to_type_ref.context = input.context;

        auto canon_ptr_to_type = co_await QUX_CO_DEP(lookup, (to_type_ref));
        if (!canon_ptr_to_type.has_value())
        {
            co_return std::nullopt;
        }

        pointer_type canonical_ptr_type;
        canonical_ptr_type.qual = ptr.qual;
        canonical_ptr_type.ptr_class = ptr.ptr_class;
        canonical_ptr_type.target = canon_ptr_to_type.value();

        co_return canonical_ptr_type;
    }
    else if (type.template type_is< subsymbol >())
    {
        subsymbol const& sub = as< subsymbol >(type);

        type_symbol const& parent = sub.of;

        if (parent.template type_is< context_reference >())
        {
            std::optional< type_symbol > current_context = context;
            assert(current_context.has_value());
            assert(!qualified_is_contextual(current_context.value()));

            while (current_context.has_value())
            {
                std::string name = sub.name;
                if (typeis< instanciation_reference >(*current_context))
                {
                    QUXLANG_DEBUG({ std::cout << "Instanciation:  within " << to_string(*current_context) << " check  " << to_string(type) << std::endl; });

                    // Two possibilities, 1 = this is a template, 2 = this is a function
                    instanciation_reference inst = as< instanciation_reference >(*current_context);

                    auto param_set = co_await QUX_CO_DEP(instanciation_parameter_map, (inst));

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
                }

                subsymbol sub2{current_context.value(), sub.name};

                auto exists = co_await QUX_CO_DEP(exists, (sub2));

                QUXLANG_DEBUG({ std::cout << "Exists? " << to_string(sub2) << ": " << (exists ? "yes" : "no") << std::endl; });

                if (exists)
                {
                    co_return sub2;
                }

                current_context = qualified_parent(current_context.value());
                QUXLANG_DEBUG({ std::cout << "New context: " << quxlang::to_string(current_context.value_or(void_type{})) << std::endl; });
            }

            std::string str = "Could not find '" + sub.name + "'";
            co_return std::nullopt;
        }

        auto parent_canonical = *co_await QUX_CO_DEP(lookup, (contextual_type_reference{.context = context, .type = parent}));

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
            assert(!qualified_is_contextual(current_context.value()));

            while (current_context.has_value())
            {
                submember sub2{current_context.value(), sub.name};

                auto exists = co_await QUX_CO_DEP(exists, (sub2));

                QUXLANG_DEBUG({ std::cout << "Exists? " << to_string(sub2) << ": " << (exists ? "yes" : "no") << std::endl; });

                if (exists)
                {
                    co_return sub2;
                }

                current_context = qualified_parent(current_context.value());
            }

            std::string str = "Could not find '" + sub.name + "'";
            co_return std::nullopt;
        }

        auto parent_canonical = (co_await QUX_CO_DEP(lookup, (contextual_type_reference{.context = context, .type = parent}))).value();

        co_return submember{parent_canonical, sub.name};
    }
    else if (type.template type_is< initialization_reference >())
    {
        initialization_reference const& param_set = as< initialization_reference >(type);

        initialization_reference output;

        auto callee_canonical = co_await QUX_CO_DEP(lookup, ({.type = param_set.initializee, .context = context}));
        if (!callee_canonical.has_value())
        {
            co_return std::nullopt;
        }

        output.initializee = callee_canonical.value();

        for (auto& p : param_set.parameters.positional)
        {
            auto param_canonical = co_await QUX_CO_DEP(lookup, ({p, context}));
            if (!param_canonical.has_value())
            {
                co_return std::nullopt;
            }

            output.parameters.positional.push_back(param_canonical.value());
            // TODO: support named parameters
        }

        co_return output;
    }
    else if (type.template type_is< int_type >())
    {
        co_return type;
    }
    else if (type.template type_is< bool_type >())
    {
        co_return type;
    }
    else if (type.template type_is< module_reference >())
    {
        co_return type;
    }
    else if (typeis< void_type >(type))
    {
        co_return type;
    }
    else if (typeis< template_reference >(type))
    {
        co_return type;
    }
    else if (typeis< array_type >(type))
    {
        auto const& arry = type.template get_as< array_type >();
        constexpr_input ce_input;
        ce_input.context = context;
        ce_input.expr = arry.element_count;
        std::uint64_t element_count = co_await QUX_CO_DEP(constexpr_u64, (ce_input));
        array_type result_type;
        result_type.element_count = expression_numeric_literal{std::to_string(element_count)};
        auto lookup_element_type = co_await QUX_CO_DEP(lookup, ({.type = arry.element_type, .context = context}));
        if (!lookup_element_type.has_value())
        {
            co_return std::nullopt;
        }
        result_type.element_type = lookup_element_type.value();
        co_return result_type;
    }
    else
    {
        std::string str = std::string() + "unimplemented: " + type.type().name();
        QUXLANG_DEBUG({ std::cout << str << std::endl; });
        throw std::logic_error("unreachable/unimplemented");
    }
}