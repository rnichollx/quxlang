//
// Created by Ryan Nicholl on 2/20/24.
//

#ifndef RPNX_QUXLANG_INPUT_AGGREGATOR_HEADER
#define RPNX_QUXLANG_INPUT_AGGREGATOR_HEADER

#include <filesystem>
#include <set>

namespace quxlang
{
    class input_aggregator
    {
        struct impl;
        impl* p_impl;

      public:
        input_aggregator(std::filesystem::path const& path);
        ~input_aggregator();

        std::set<std::filesystem::path> input_files();
    };

} // namespace quxlang

#endif // RPNX_QUXLANG_INPUT_AGGREGATOR_HEADER
