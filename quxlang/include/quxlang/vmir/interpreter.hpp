// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR_INTERPRETER_HEADER_GUARD
#define QUXLANG_VMIR_INTERPRETER_HEADER_GUARD

#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/expression.hpp"
#include "quxlang/data/interp_value.hpp"

#include <quxlang/vmir2/vmir2.hpp>
namespace quxlang
{
    struct class_layout;

    class interpreter
    {
        struct memory_alloc
        {
            std::vector< std::byte > data;
            bool live = false;
        };

        struct pointer_val
        {
            std::size_t frame_index;
            std::size_t slot;
        };

        std::vector< pointer_val > pointers;
        std::vector< memory_alloc > memory;


        struct vm_frame
        {
            std::vector< interp_value > slots;
            std::vector< bool > slot_live;

        };

        std::map< type_symbol, vmir2::functanoid_routine2 > functanoids;

        std::size_t current_frame = 0;

        std::vector<std::shared_ptr<vm_frame>> live_frames;
        std::map< std::size_t, std::weak_ptr<vm_frame> > frame_ids_map;



      public:
        interpreter()= default;
        ~interpreter() = default;


        void add_functanoid(type_symbol addr, vmir2::functanoid_routine2 func)
        {
            functanoids.emplace(std::move(addr), std::move(func));
        }

        void add_class_info(type_symbol cls, class_layout layout);

        rpnx::result<interp_value> exec_call(type_symbol callee, interp_callargs args);

        interp_value call(vmir2::functanoid_routine2 const & func, interp_callargs args);

    };

} // namespace quxlang

#endif // RPNX_QUXLANG_INTERPRETER_HEADER
