// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/implementation_function_map_spec.hpp>

#include <quxlang/manipulators/typeutils.hpp>

rpnx::querygraph::coroutine< quxlang::implementation_function_map_spec > quxlang::implementation_function_map_impl(type_symbol input)
{
    auto slot_return_type = [](interface_slot_key const& key) -> type_symbol {
        return key.concrete_return_type.value_or(type_symbol(void_type{}));
    };

    type_symbol interface_type = co_await rpnx::querygraph::request< implementation_interface_type_query >(input);
    std::vector< interface_slot > slots = co_await rpnx::querygraph::request< interface_slot_list_query >(interface_type);

    std::map< interface_slot_key, type_symbol > output;
    for (interface_slot const& slot : slots)
    {
        type_symbol function_symbol = subsymbol{.of = input, .name = slot.key.name};
        auto function_kind = co_await rpnx::querygraph::request< symbol_type_query >(function_symbol);
        if (function_kind != symbol_kind::functum)
        {
            throw std::logic_error("Implementation is missing interface function: " + slot.key.name);
        }

        initialization_reference init{
            .initializee = function_symbol,
            .parameters = instatype_from_invotype(slot.key.concrete_params),
            .adaptations = allowed_adaptations::destination_rebinding,
        };

        auto inst = co_await rpnx::querygraph::request< instanciation_query >(init);
        if (!inst.has_value())
        {
            throw std::logic_error("Implementation function does not match interface slot: " + slot.key.name);
        }

        auto return_type = co_await rpnx::querygraph::request< functanoid_return_type_query >(*inst);
        if (return_type != slot_return_type(slot.key))
        {
            throw std::logic_error("Implementation function return type does not match interface slot: " + slot.key.name);
        }

        output[slot.key] = *inst;
    }

    co_return output;
}
