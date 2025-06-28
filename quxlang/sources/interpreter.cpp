// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/vmir/interpreter.hpp"

#include "rpnx/value.hpp"
rpnx::result< quxlang::interp_value > quxlang::interpreter::exec_call(type_symbol callee, interp_callargs args)
{
    vmir2::functanoid_routine2& func = functanoids.at(callee);

    return call(func, args);
}
quxlang::interp_value quxlang::interpreter::call(vmir2::functanoid_routine2 const& func, interp_callargs args)
{

    rpnx::result< quxlang::interp_value > result;
    std::size_t frame_id = current_frame++;

    live_frames.push_back(std::make_shared< vm_frame >());
    frame_ids_map[frame_id] = live_frames.back();

    std::size_t arg_index = 0;

    auto current_frame = live_frames.back();

    auto named_args = args.named;

    current_frame->slots.resize(func.local_types.size());

    for (std::size_t i = 0; i < func.local_types.size(); i++)
    {
        auto const & slot = func.local_types[i];

        if (slot.kind == vmir2::slot_kind::positional_arg)
        {
            auto argval = args.positional.at(arg_index++);

            if (argval.type != slot.type)
            {
                throw std::logic_error("Argument type mismatch");
            }

            if (argval.type.type_is<nvalue_slot>())
            {
                throw std::logic_error("Constexpr VM cannot initialize nvalue slot in global context");
            }

            current_frame->slots[i] = argval;
        }



    }

    throw rpnx::unimplemented();
}
