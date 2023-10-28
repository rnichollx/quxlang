//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_CANONICAL_POINTER_TYPE_REFERENCE_HEADER
#define RPNX_RYANSCRIPT1031_CANONICAL_POINTER_TYPE_REFERENCE_HEADER

#include "canonical_type_reference.hpp"
namespace rylang
{
   struct canonical_pointer_type_reference
   {
       canonical_type_reference to;

       inline bool operator<(canonical_pointer_type_reference const& other) const
       {
           return to < other.to;
       }

       inline bool operator == (canonical_pointer_type_reference const& other) const
       {
           return to == other.to;
       }

       inline bool operator != (canonical_pointer_type_reference const& other) const
       {
           return to != other.to;
       }
   };
}

#endif // RPNX_RYANSCRIPT1031_CANONICAL_POINTER_TYPE_REFERENCE_HEADER
