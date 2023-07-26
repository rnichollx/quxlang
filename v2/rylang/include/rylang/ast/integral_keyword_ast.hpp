//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_INTEGRAL_KEYWORD_AST_HEADER
#define RPNX_RYANSCRIPT1031_INTEGRAL_KEYWORD_AST_HEADER

namespace rylang
{
    struct integral_keyword_ast
    {
        bool is_sized;
        int size;

        inline std::string to_string() const
        {
            return "ast_integral_keyword{ signedness: " + std::to_string(is_sized) + ", size: " + std::to_string(size) + " }";
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_INTEGRAL_KEYWORD_AST_HEADER
