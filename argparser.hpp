//
// Created by Ryan Nicholl on 3/8/23.
//

#ifndef RPNX_RYANSCRIPT1031_ARGPARSER_HEADER
#define RPNX_RYANSCRIPT1031_ARGPARSER_HEADER
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>
namespace rs1031
{
    class argparser
    {
        std::vector< std::string > arglist;

      public:
        argparser(int argc, char** argv)
        {
            for (int i = 0; i < argc; i++)
            {
                arglist.push_back(argv[i]);
            }
        }

        std::string get_string(int index)
        {
            return arglist.at(index);
        }

        std::filesystem::path get_path(std::size_t index)
        {
            return arglist.at(index);
        }

        std::size_t size() const
        {
            return arglist.size();
        }
    };
} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_ARGPARSER_HEADER
