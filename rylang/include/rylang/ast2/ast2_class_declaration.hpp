#ifndef AST2_CLASS_DECLARATION_HEADER_GUARD
#define AST2_CLASS_DECLARATION_HEADER_GUARD
#include "ast2_class_variable_declaration.hpp"

#include <vector>

namespace rylang
{
    struct ast2_class_declaration
    {
        std::vector<ast2_class_variable_declaration> variables;
        std::vector<ast2_function_declaration> functions;
    };
} // namespace rylang

#endif // AST_2_CLASS_DECLARATION_HEADER_GUARD
