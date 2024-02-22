//
// Created by Ryan Nicholl on 2/20/24.
//

#include "rylang/input_aggregator.hpp"

namespace rylang
{
    struct input_aggregator::impl
    {

        std::filesystem::path path;

        inline impl(std::filesystem::path const& path)
        {
            this->path = path;
        }

        inline std::set< std::filesystem::path > input_files()
        {
            std::set< std::filesystem::path > ret;

            for (auto& p : std::filesystem::recursive_directory_iterator(path))
            {
            // If file ends in .qx, add the file to the list of input files
                if (p.is_regular_file() && p.path().extension() == ".qx")
                {
                    ret.insert(p);
                }
            }

            return ret;
        }
    };

    input_aggregator::input_aggregator(std::filesystem::path const& path)
    {
        p_impl = new impl(path);
    }

    input_aggregator::~input_aggregator()
    {
        delete p_impl;
    }

    std::set<std::filesystem::path> input_aggregator::input_files()
    {
        return p_impl->input_files();
    }

} // namespace rylang