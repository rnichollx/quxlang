//
// Created by Ryan Nicholl on 11/9/23.
//

#ifndef RPNX_RYANSCRIPT1031_NUMERIC_LITERAL_HEADER
#define RPNX_RYANSCRIPT1031_NUMERIC_LITERAL_HEADER

#include <string>

namespace rylang
{
   struct numeric_literal
   {
       std::string value;

       std::strong_ordering operator<=>(numeric_literal const&) const = default;
   };
}

#endif // RPNX_RYANSCRIPT1031_NUMERIC_LITERAL_HEADER
