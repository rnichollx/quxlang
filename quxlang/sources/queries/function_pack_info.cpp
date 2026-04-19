// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/function_pack_info_spec.hpp>

#include <stdexcept>

namespace quxlang
{
    rpnx::querygraph::coroutine< function_pack_info_spec > function_pack_info_impl(instanciation_reference input)
    {
        function_pack_info result;

        auto declaration = co_await rpnx::querygraph::request< function_declaration_query >(input.temploid);
        if (!declaration.has_value())
        {
            co_return result;
        }

        std::uint64_t expanded_positional_index = 0;
        bool saw_pack = false;

        for (auto const& param : declaration->header.call_parameters)
        {
            if (param.api_name.has_value())
            {
                if (param.is_pack)
                {
                    throw std::logic_error("Named variadic packs are not supported");
                }
                continue;
            }

            if (!param.is_pack)
            {
                if (saw_pack)
                {
                    throw std::logic_error("A positional parameter cannot follow a positional variadic pack");
                }
                expanded_positional_index++;
                continue;
            }

            if (saw_pack)
            {
                throw std::logic_error("Only one positional variadic pack is supported");
            }
            saw_pack = true;

            if (input.params.positional.size() < expanded_positional_index)
            {
                throw std::logic_error("Function instantiation has fewer positional arguments than the fixed prefix");
            }

            std::uint64_t const pack_size = static_cast< std::uint64_t >(input.params.positional.size()) - expanded_positional_index;
            if (param.name.has_value())
            {
                function_pack_entry entry;
                entry.fixed_prefix_count = expanded_positional_index;
                entry.size = pack_size;
                for (std::uint64_t pack_index = 0; pack_index < pack_size; pack_index++)
                {
                    std::uint64_t const positional_index = expanded_positional_index + pack_index;
                    entry.positional_indices.push_back(positional_index);
                    entry.types.push_back(parameter_instantiation_type(input.params.positional.at(static_cast< std::vector< type_symbol >::size_type >(positional_index))));
                }
                result.packs[param.name.value()] = std::move(entry);
            }

            expanded_positional_index += pack_size;
        }

        co_return result;
    }
} // namespace quxlang
