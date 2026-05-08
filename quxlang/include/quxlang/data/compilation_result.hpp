// Copyright 2025 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2025 Ryan P. Nicholl $USER_EMAIL

#ifndef QUXLANG_COMPILATION_RESULT_HPP
#define QUXLANG_COMPILATION_RESULT_HPP

#include <quxlang/ast2/source_location.hpp>
#include <rpnx/variant.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace quxlang
{
    struct trace_frame
    {
        std::string trace_context;
        std::optional< source_location > location;

        RPNX_MEMBER_METADATA(trace_frame, trace_context, location);
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

    /**
     * Compiler-facing diagnostic that can be cached and rethrown as a
     * canonical querygraph error.
     */
    struct compilation_error : public std::logic_error
    {
        std::string message = "compilation error";
        std::vector< trace_frame > traceback;

        rpnx::variant< syntax_error, semantic_error > structured_error;

        compilation_error() : std::logic_error("compilation error")
        {
        }

        explicit compilation_error(std::string message_arg) : std::logic_error(message_arg), message(std::move(message_arg))
        {
        }

        auto what() const noexcept -> char const* override
        {
            return message.c_str();
        }

        RPNX_MEMBER_METADATA(compilation_error, message, traceback, structured_error);
    };

    /**
     * Builds a compilation_error for a semantic diagnostic message.
     */
    inline auto semantic_compilation_error(std::string message) -> compilation_error
    {
        compilation_error error(message);
        error.structured_error = semantic_error{std::move(message)};
        return error;
    }


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
