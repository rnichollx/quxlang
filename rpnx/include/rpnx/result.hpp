//
// Created by Ryan Nicholl on 6/12/24.
//

#ifndef RPNX_QUXLANG_RESULT_HEADER
#define RPNX_QUXLANG_RESULT_HEADER
#include <exception>
#include <optional>

namespace rpnx
{
    template < typename T >
    class result
    {
        std::optional< T > t;
        std::exception_ptr er;

      public:
        result(T t)
            : t(t)
        {
        }

        result()
        {
        }

        result(std::exception_ptr er)
            : er(er)
        {
        }

        T get() const
        {
            if (er)
                std::rethrow_exception(er);
            return t.value();
        }

        void set_value(T t)
        {
            this->er = nullptr;
            this->t = t;
        }

        void set_error(std::exception_ptr er)
        {
            this->t.reset();
            this->er = er;
        }

        bool has_result() const
        {
            return t.has_value() || er != nullptr;
        }

        bool has_value() const
        {
            return t.has_value();
        }

        bool has_error() const
        {
            return er != nullptr;
        }

        std::exception_ptr get_error() const
        {
            return er;
        }

        inline operator bool() const
        {
            return has_result();
        }
    };

    template <>
    class result< void >
    {
        bool t = false;
        std::exception_ptr er;

      public:
        result()
        {
        }

        result(std::exception_ptr er)
            : er(er)
        {
        }

        void test() const
        {
            if (er)
            {
                std::rethrow_exception(er);
            }
        }

        void get() const
        {
            if (er)
                std::rethrow_exception(er);
            if (!t)
                throw std::logic_error("No value");
            return;
        }

        void set_value()
        {
            this->er = nullptr;
            this->t = true;
        }

        void set_error(std::exception_ptr er)
        {
            this->t = false;
            this->er = er;
        }

        bool has_value() const
        {
            return t;
        }

        bool has_error() const
        {
            return er != nullptr;
        }

        std::exception_ptr get_error() const
        {
            return er;
        }

        inline bool has_result() const
        {
            return has_value() || has_error();
        }

        inline operator bool() const
        {
            return has_result();
        }
    };
} // namespace rpnx

#endif // RPNX_QUXLANG_RESULT_HEADER
