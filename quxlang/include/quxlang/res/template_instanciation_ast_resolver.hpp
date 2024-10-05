// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_TEMPLATE_INSTANCIATION_AST_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_TEMPLATE_INSTANCIATION_AST_RESOLVER_HEADER_GUARD

#include <rpnx/resolver_utilities.hpp>
#include <quxlang/ast2/ast2_entity.hpp>

namespace quxlang
{
    class compiler;
}
namespace quxlang
{
    class template_instanciation_ast_resolver
        : public rpnx::co_resolver_base<compiler, ast2_template_declaration, instantiation_type>
    {
    public:
        template_instanciation_ast_resolver(input_type input)
            : co_resolver_base(input)
        {
        }

        virtual std::string question() const override;

        virtual auto co_process(compiler * c, input_type input) -> co_type override;

    };
}

#endif //TEMPLATE_INSTANCIATION_RESOLVER_HPP
