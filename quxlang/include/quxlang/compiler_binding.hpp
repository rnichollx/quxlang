// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_COMPILER_BINDING_HEADER_GUARD
#define QUXLANG_COMPILER_BINDING_HEADER_GUARD

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
        QUX_BIND(lookup);
        QUX_BIND(instanciation);
        QUX_BIND(functanoid_return_type);
        QUX_BIND(class_layout);
        QUX_BIND(class_field_list);
        QUX_BIND(functum_select_function);
        QUX_BIND(functanoid_param_names);
        QUX_BIND(functanoid_sigtype);
        QUX_BIND(function_declaration);
        QUX_BIND(implicitly_convertible_to);
        QUX_BIND(symbol_type);
        QUX_BIND(variable_type);
        QUX_BIND(nontrivial_default_dtor);


        auto& implicitly_convertible_to(type_symbol from, type_symbol to)
        {
            implicitly_convertible_to_query q;
            q.from = from;
            q.to = to;
            return this->implicitly_convertible_to(q);
        }

        auto output_info()
        {
          return c->m_output_info;
        }
    };
} // namespace quxlang

#endif // RPNX_QUXLANG_COMPILER_BINDING_HEADER
