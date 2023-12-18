//
// Created by Ryan Nicholl on 12/17/23.
//
#include <rylang/ast2/ast2_entity.hpp>

namespace rylang
{
    struct ast2_entity_stringifier : boost::static_visitor< std::string >
    {
        std::string operator()(std::monostate) const
        {
            return "error/monostate";
        }

        std::string operator()(ast2_namespace_declaration const& ref) const
        {
            std::string out = "namespace {\n";
            for (auto const& [name, decl] : ref.globals)
            {
                out += "    " + name + " = " + to_string(decl) + "\n";
            }
            out += "}";
            return out;
        }

        
    };
} // namespace rylang
std::string rylang::to_string(ast2_declarable const& ref)
{
}