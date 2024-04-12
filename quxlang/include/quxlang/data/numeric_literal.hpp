//
// Created by Ryan Nicholl on 11/9/23.
//

#ifndef QUXLANG_NUMERIC_LITERAL_HEADER_GUARD
#define QUXLANG_NUMERIC_LITERAL_HEADER_GUARD

#include <string>

namespace quxlang
{
   struct expression_numeric_literal
    {
       std::string value;

       std::strong_ordering operator<=>(expression_numeric_literal const&) const = default;
   };

}

#endif // QUXLANG_NUMERIC_LITERAL_HEADER_GUARD
