//
// Created by Ryan Nicholl on 11/9/23.
//

#ifndef RYLANG_NUMERIC_LITERAL_HEADER_GUARD
#define RYLANG_NUMERIC_LITERAL_HEADER_GUARD

#include <string>

namespace rylang
{
   struct numeric_literal
   {
       std::string value;

       std::strong_ordering operator<=>(numeric_literal const&) const = default;
   };

}

#endif // RYLANG_NUMERIC_LITERAL_HEADER_GUARD
