// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR_INTERPRETER_HEADER_GUARD
#define QUXLANG_VMIR_INTERPRETER_HEADER_GUARD

#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/expression.hpp"
#include "quxlang/data/interp_value.hpp"
namespace quxlang
{

    class interpreter
    {
        struct memory_alloc
        {
            std::vector< std::byte > data;
            bool live = false;
        };

        struct pointer_val
        {
            std::size_t alloc_index;
            std::size_t offset;
        };

        std::vector< pointer_val > pointers;
        std::vector< memory_alloc > memory;



      public:
        interpreter(compiler* c);
        ~interpreter();

        rpnx::result<interp_value> exec_call(interp_value callee, std::vector<interp_value> args);
        std::size_t allocate(type_symbol type, std::size_t count);
        rpnx::result<interp_value> pointer_add(interp_value ptr, interp_value offset);

        bool exec_bool(expression const& e);
    };

} // namespace quxlang

#endif // RPNX_QUXLANG_INTERPRETER_HEADER
