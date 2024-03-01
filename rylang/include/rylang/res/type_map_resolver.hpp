//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef MEMBER_MAP_RESOLVER_HPP
#define MEMBER_MAP_RESOLVER_HPP
#include <rpnx/resolver_utilities.hpp>
#include <rylang/ast2/ast2_type_map.hpp>
#include <rylang/compiler_fwd.hpp>

namespace rylang
{

    /**
     * \brief The member map resolver is used to map a class declaration's member declarations to a map of names to merged AST entries.
     *
     * For example, if a class defines two funtions of the same name but different argument types,
     * the memeber map resolver merges the two functions into one functum entry.
     */
    class type_map_resolver : public rpnx::co_resolver_base< compiler, ast2_type_map, type_symbol >
    {
      public:
        type_map_resolver(type_symbol functum)
            : co_resolver_base(functum)
        {
        }

        virtual rpnx::resolver_coroutine< compiler, ast2_type_map > co_process(compiler* c, type_symbol input) override;
    };
} // namespace rylang
#endif // MEMBER_MAP_RESOLVER_HPP
