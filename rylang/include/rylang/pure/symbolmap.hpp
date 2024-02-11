//
// Created by Ryan Nicholl on 2024-02-10.
//

#ifndef RYLANG_SYMBOLMAP_HPP
#define RYLANG_SYMBOLMAP_HPP
#include <string>

namespace rylang
{

    struct symbol_map_info_input
    {
        std::size_t m_position;
        std::string m_name;

        std::size_t position() const
        {
            return m_position;
        }

        std::string name() const
        {
            return m_name;
        }
    };

    struct symbol_map_info_output
    {
        std::string name;
        std::size_t position;
        std::size_t position_end;
    };

    template <typename It, typename F >
    inline void calc_symbol_positions(It begin, It end, std::size_t size, F output)
    {
        std::multimap<std::size_t, std::string> symbols;


        for (auto i = begin; i != end; i++)
        {
            symbol_map_info_input input;
            input.m_position = i->position();
            input.m_name = i->name();

            symbols.insert(std::make_pair<std::size_t, std::string>(input.m_position, std::move(input.m_name)));
        }

        for (auto it = symbols.begin(); it != symbols.end(); it++)
        {
            auto pos = it->first;

            auto rang_end = std::upper_bound( symbols.begin(), symbols.end(), pos);

            symbol_map_info_output output_object;

            if (rang_end != symbols.end())
            {
                output_object.position_end = rang_end->first;
            }
            else
            {
                output_object.position_end = size;
            }

            output(output_object);
        }
    }
}

#endif //RYLANG_SYMBOLMAP_HPP
