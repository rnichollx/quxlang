//
// Created by Ryan Nicholl on 2/24/24.
//

#ifndef RPNX_QUXLANG_INTERPRETER_HEADER
#define RPNX_QUXLANG_INTERPRETER_HEADER

#include "rylang/compiler_fwd.hpp"
#include "rylang/data/expression.hpp"
namespace rylang
{

    class interpreter
    {
      public:
        interpreter(compiler* c);
        ~interpreter();

        bool exec_bool(expression const& e);
    };
} // namespace rylang

#endif // RPNX_QUXLANG_INTERPRETER_HEADER
