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

        inline void build(std::string target)
        {
           std::filesystem::path buildfile_path = path / "build.yml"
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
    void input_aggregator::build()
    {
        p_impl->build();
    }

} // namespace rylang