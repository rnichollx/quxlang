//
// Created by Ryan Nicholl on 7/20/23.
//

#include "rylang/ast/type_ref_ast.hpp"
#include "rylang/ast/pointer_ref_ast.hpp"

std::string rylang::type_ref_ast::to_string() const
{
    return "type_ref_ast{" +
           std::visit(
               [](auto const& arg) -> std::string
               {
                   return arg.to_string();
               },
               val.get()) +
           "}";
}

bool rylang::type_ref_ast::operator<(type_ref_ast const& other) const
{
    return val < other.val;
}

rylang::type_ref_ast::operator type_reference() const
{
    if (holds_alternative< symbol_ref_ast >(val.get()))
    {
        // TODO: This conversion is messy and incomplete.
        symbol_ref_ast const& sym = std::get< symbol_ref_ast >(val.get());
        lookup_chain chain;
        lookup_singular lk;
        lk.type = lookup_type::scope;
        lk.identifier = sym.name;
        chain.chain.push_back(lk);

        return chain;
    }

    throw std::runtime_error("unimplemented");
}
