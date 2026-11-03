// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_COROUTINE_DESTRUCTOR_HEADER_GUARD
#define QUXLANG_QUERIES_COROUTINE_DESTRUCTOR_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <optional>

namespace quxlang
{
    /** Identifies the object and external controller types used for coroutine-context destruction. */
    struct coroutine_destructor_request
    {
        type_symbol object_type;
        type_symbol controller_type;

        RPNX_MEMBER_METADATA(coroutine_destructor_request, object_type, controller_type);
    };

    /** Describes a selected destructor body and the cleanup still owned by its caller. */
    struct coroutine_destructor_selection
    {
        /** Selected user body, or a complete synchronous operation when subobject cleanup is false. */
        std::optional< instanciation_reference > operation;
        /** True when operation denotes a coroutine that requires controller adaptation. */
        bool is_coroutine = false;
        /** True when the caller must destroy owned subobjects after the optional body. */
        bool requires_subobject_cleanup = false;

        RPNX_MEMBER_METADATA(coroutine_destructor_selection, operation, is_coroutine, requires_subobject_cleanup);
    };

    /**
     * Selects destruction for the adapted child controller, including owned subobjects.
     * Disabled or controller-incompatible coroutine overloads permit an ordinary
     * alternative. Synchronous selected cleanup returns the complete ordinary
     * operation. The enclosing CO_CALL_DTOR operation determines child-controller
     * adaptation before this query resolves the selected body.
     */
    struct coroutine_destructor_query
    {
        static constexpr auto query_id = "coroutine_destructor";
        using input_type = coroutine_destructor_request;
        using output_type = coroutine_destructor_selection;
    };
}

#endif
