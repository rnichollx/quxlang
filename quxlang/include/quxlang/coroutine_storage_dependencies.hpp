// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_COROUTINE_STORAGE_DEPENDENCIES_HEADER_GUARD
#define QUXLANG_COROUTINE_STORAGE_DEPENDENCIES_HEADER_GUARD

#include <quxlang/queries/specs/coroutine_frame_spec.hpp>

#include <set>
#include <vector>

namespace quxlang
{
    /** Traverses structural storage dependencies before requesting coroutine body or payload layout. */
    struct coroutine_storage_dependencies
    {
        type_symbol frame;
        std::set< type_symbol > visited;
        std::vector< type_symbol > pending;

        /** Adds runtime captures whose values contribute storage to the nominal frame. */
        auto co_add_captures(instanciation_reference const& coroutine) -> rpnx::querygraph::coroutine< coroutine_frame_spec >::cosubroutine< void >;

        /** Rejects storage edges returning to the nominal frame and retains completed traversal results. */
        auto co_validate() -> rpnx::querygraph::coroutine< coroutine_frame_spec >::cosubroutine< void >;
    };
}

#endif // QUXLANG_COROUTINE_STORAGE_DEPENDENCIES_HEADER_GUARD
