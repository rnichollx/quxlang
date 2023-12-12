//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef AST2_ENTITY_HEADER_GUARD
#define AST2_ENTITY_HEADER_GUARD

#include <boost/variant.hpp>

namespace rylang
{

    struct ast2_namespace;
    struct ast2_class_variable_declaration;
    struct ast2_class_declaration;
    struct ast2_function_declaration;
    struct ast2_template_class_declaration;

    using ast2_entity = boost::variant< boost::recursive_wrapper< ast2_namespace >, boost::recursive_wrapper< ast2_class_variable_declaration >, boost::recursive_wrapper< ast2_template_class_declaration >, boost::recursive_wrapper< ast2_class_declaration >, boost::recursive_wrapper< ast2_function_declaration > >;

} // namespace rylang

#endif // AST2_ENTITY_HEADER_GUARD
