//
// Created by Ryan Nicholl on 7/6/24.
//

#ifndef RPNX_QUXLANG_COMPILER_BINDING_HEADER
#define RPNX_QUXLANG_COMPILER_BINDING_HEADER

#include "compiler.hpp"

// clang-format off

#define QUX_BIND(x) auto x(typename quxlang:: x ## _resolver::input_type const & in) -> decltype(*c->lk_ ## x(std::declval<typename quxlang:: x ## _resolver::input_type const &>())) { return *c->lk_ ## x(in); }
// clang-format on

namespace quxlang
{
    class compiler_binder
    {
      protected:
        compiler* c;

      public:
        template < typename T >
        using co_type = rpnx::general_coroutine< compiler, T >;

        compiler_binder(compiler* c) : c(c)
        {
        }

        QUX_BIND(vm_procedure2);
        QUX_BIND(canonical_symbol_from_contextual_symbol);
        QUX_BIND(instanciation);
        QUX_BIND(functanoid_return_type);
        QUX_BIND(class_layout);
        QUX_BIND(functum_select_function);
        QUX_BIND(function_declaration);
    };
} // namespace quxlang

#endif // RPNX_QUXLANG_COMPILER_BINDING_HEADER
