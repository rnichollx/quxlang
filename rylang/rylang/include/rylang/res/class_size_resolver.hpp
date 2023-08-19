//
// Created by Ryan Nicholl on 8/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_SIZE_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_SIZE_RESOLVER_HEADER

#include "rylang/compiler.hpp"
#include "rylang/data/symbol_id.hpp"

namespace rylang
{
    /** This resolver takes a symbol_id as input and returns the size of an instanciaion of that class */
    class class_size_resolver
    : public rpnx::output_base<compiler, std::size_t>
    {
        class_size_resolver(symbol_id id);

        virtual void process(compiler* c);

      private:
        symbol_id m_id;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CLASS_SIZE_RESOLVER_HEADER
