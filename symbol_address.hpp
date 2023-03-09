//
// Created by Ryan Nicholl on 3/8/23.
//

#ifndef RPNX_RYANSCRIPT1031_SYMBOL_ADDRESS_HEADER
#define RPNX_RYANSCRIPT1031_SYMBOL_ADDRESS_HEADER

#include <vector>

namespace rs1031
{

    struct symbol_address : std::vector< std::string >
    {
        template < typename... Args >
        symbol_address(Args&&... args)
            : std::vector< std::string >{std::forward< Args >(args)...}
        {
        }

        inline std::string prettystring() const
        {
            std::string str;
            bool first = true;
            for (auto const& s : *this)
            {
                if (first)
                {
                    first = false;
                }
                else
                {
                    str += "::";
                }
                str += s;
            }
            return str;
        }
    };
} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_SYMBOL_ADDRESS_HEADER
