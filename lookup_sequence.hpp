//
// Created by Ryan Nicholl on 3/8/23.
//

#ifndef RPNX_RYANSCRIPT1031_LOOKUP_SEQUENCE_HEADER
#define RPNX_RYANSCRIPT1031_LOOKUP_SEQUENCE_HEADER
#include <deque>
#include <string>
#include <vector>

namespace rs1031
{

    struct lir_lookup_item
    {
    };

    // TODO: replace with other types of lookup
    struct static_lookup_sequence : std::vector< std::string >
    {
        template < typename... Args >
        static_lookup_sequence(Args&&... args)
            : std::vector< std::string >{std::forward< Args >(args)...}
        {
        }

        static_lookup_sequence(const static_lookup_sequence&) = default;

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

#endif // RPNX_RYANSCRIPT1031_LOOKUP_SEQUENCE_HEADER
