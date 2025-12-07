// Copyright (c) 2025 Ryan P. Nicholl $USER_EMAIL

#ifndef QUXLANG_COMPILATION_RESULT_HPP
#define QUXLANG_COMPILATION_RESULT_HPP

#include <rpnx/variant.hpp>

namespace quxlang
{
    struct syntax_error;
    struct semantic_error;


    struct trace_frame
    {
        std::string trace_context;
        source_location location;

        RPNX_MEMBER_METADATA(trace_frame, trace_context, location);
    };

    struct compilation_error
    {
        std::vector< trace_frame > traceback;

        rpnx::variant< syntax_error, semantic_error > structured_error;

        RPNX_MEMBER_METADATA(compilation_error, traceback, structured_error);
    };

    struct syntax_error
    {
        std::string message;

        RPNX_MEMBER_METADATA(syntax_error, message)
    };
    struct semantic_error
    {
        std::string message;

        RPNX_MEMBER_METADATA(semantic_error, message);
    };


    template < typename T >
    class compilation_result
    {
        rpnx::variant< T, compilation_error > m_data;

      public:
        compilation_result(compilation_error e) : m_data(std::move(e))
        {
        }

        compilation_result(T t) : m_data(std::move(t))
        {
        }
    };
} // namespace quxlang

#endif // QUXLANG_COMPILATION_RESULT_HPP
