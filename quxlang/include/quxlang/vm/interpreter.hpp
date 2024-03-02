//
// Created by Ryan Nicholl on 2/24/24.
//

#ifndef RPNX_QUXLANG_INTERPRETER_HEADER
#define RPNX_QUXLANG_INTERPRETER_HEADER

#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/expression.hpp"
namespace quxlang
{

    class interpreter
    {
      public:
        interpreter(compiler* c);
        ~interpreter();

        bool exec_bool(expression const& e);
    };
} // namespace quxlang

#endif // RPNX_QUXLANG_INTERPRETER_HEADER
