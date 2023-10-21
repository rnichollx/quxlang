//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_TYPE_REF_AST2_HEADER
#define RPNX_RYANSCRIPT1031_TYPE_REF_AST2_HEADER

#include "rylang/data/lookup_chain.hpp"
#include <boost/variant.hpp>
namespace rylang
{
   struct pointer_type_ast;
   using type_reference_ast = boost::variant< boost::recursive_wrapper<pointer_type_ast>, lookup_chain >;
}

#endif // RPNX_RYANSCRIPT1031_TYPE_REF_AST2_HEADER
