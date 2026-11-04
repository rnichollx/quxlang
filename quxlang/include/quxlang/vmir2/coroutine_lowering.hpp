// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_COROUTINE_LOWERING_HEADER_GUARD
#define QUXLANG_VMIR2_COROUTINE_LOWERING_HEADER_GUARD

#include <quxlang/data/coroutine_frame_info.hpp>
#include <quxlang/vmir2/vmir2.hpp>

namespace quxlang::vmir2
{
    /** Implements driver generation while retaining logical object identities. */
    class coroutine_resume_lowering;

    /** Lowers one logical coroutine into an ordinary RESUME procedure using its canonical frame. */
    [[nodiscard]] auto lower_coroutine_resume(functanoid_routine3 const& logical, coroutine_frame_info const& frame) -> functanoid_routine3;

    /** Lowers a cancellable coroutine into a TRY_CANCEL procedure that claims only cancellation continuations. */
    [[nodiscard]] auto lower_coroutine_try_cancel(functanoid_routine3 const& logical, coroutine_frame_info const& frame) -> functanoid_routine3;

    /** Lowers explicit destruction into direct synchronous cleanup from an initialized or unshielded suspended state. */
    [[nodiscard]] auto lower_coroutine_destroy(functanoid_routine3 const& logical, coroutine_frame_info const& frame) -> functanoid_routine3;
}

#endif // QUXLANG_VMIR2_COROUTINE_LOWERING_HEADER_GUARD
