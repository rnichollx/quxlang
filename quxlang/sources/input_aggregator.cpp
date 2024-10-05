// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/input_aggregator.hpp"

namespace quxlang
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

} // namespace quxlang