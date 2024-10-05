// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef RPNX_QUXLANG_RESULT_HEADER
#define RPNX_QUXLANG_RESULT_HEADER
#include <exception>
#include <optional>

namespace rpnx
{
    /**
     * @brief A generic result class that can hold either a value or an exception.
     * @tparam T The type of the value stored in the result.
     */
    template < typename T >
    class result
    {
        std::optional< T > t;
        std::exception_ptr er;

      public:
        /**
         * @brief Constructs a result object with a value.
         * @param t The value to store in the result.
         */
        result(T t) : t(t)
        {
        }

        /**
         * @brief Constructs an empty result object.
         */
        result()
        {
        }

        /**
         * @brief Constructs a result object with an exception.
         * @param er The exception to store in the result.
         */
        result(std::exception_ptr er) : er(er)
        {
        }

        /**
         * @brief Tests if the result contains an exception and rethrows it if present.
         * @throws The stored exception if present.
         */
        void test() const
        {
            if (er)
            {
                std::rethrow_exception(er);
            }
        }

        /**
         * @brief Gets the stored value.
         * @return The stored value.
         * @throws The stored exception if present.
         */
        T get() const
        {
            if (er)
            {
                std::rethrow_exception(er);
            }

            return t.value();
        }

        /**
         * @brief Sets the value of the result.
         * @param t The value to set.
         */
        void set_value(T t)
        {
            this->er = nullptr;
            this->t = t;
        }

        /**
         * @brief Sets the exception of the result.
         * @param er The exception to set.
         */
        void set_error(std::exception_ptr er)
        {
            this->t.reset();
            this->er = er;
        }

        /**
         * @brief Checks if the result has a value or an exception.
         * @return True if the result has a value or an exception, false otherwise.
         */
        bool has_result() const
        {
            return t.has_value() || er != nullptr;
        }

        /**
         * @brief Checks if the result has a value.
         * @return True if the result has a value, false otherwise.
         */
        bool has_value() const
        {
            return t.has_value();
        }

        /**
         * @brief Checks if the result has an exception.
         * @return True if the result has an exception, false otherwise.
         */
        bool has_error() const
        {
            return er != nullptr;
        }

        /**
         * @brief Gets the stored exception.
         * @return The stored exception.
         */
        std::exception_ptr get_error() const
        {
            return er;
        }

        /**
         * @brief Implicit conversion to bool.
         * @return True if the result has a value or an exception, false otherwise.
         */
        inline operator bool() const
        {
            return has_result();
        }
    };

    /**
     * @brief Specialization of the result class for void type.
     */
    template <>
    class result< void >
    {
        bool t = false;
        std::exception_ptr er;

      public:
        /**
         * @brief Constructs an empty result object.
         */
        result()
        {
        }

        /**
         * @brief Constructs a result object with an exception.
         * @param er The exception to store in the result.
         */
        result(std::exception_ptr er) : er(er)
        {
        }

        /**
         * @brief Tests if the result contains an exception and rethrows it if present.
         * @throws The stored exception if present.
         */
        void test() const
        {
            if (er)
            {
                std::rethrow_exception(er);
            }
        }

        /**
         * @brief Gets the stored value (void).
         * @throws The stored exception if present.
         * @throws std::logic_error if no value is stored.
         */
        void get() const
        {
            if (er)
                std::rethrow_exception(er);
            if (!t)
                throw std::logic_error("No value");
            return;
        }

        /**
         * @brief Sets the value of the result (void).
         */
        void set_value()
        {
            this->er = nullptr;
            this->t = true;
        }

        /**
         * @brief Sets the exception of the result.
         * @param er The exception to set.
         */
        void set_error(std::exception_ptr er)
        {
            this->t = false;
            this->er = er;
        }

        /**
         * @brief Checks if the result has a value (void).
         * @return True if the result has a value, false otherwise.
         */
        bool has_value() const
        {
            return t;
        }

        /**
         * @brief Checks if the result has an exception.
         * @return True if the result has an exception, false otherwise.
         */
        bool has_error() const
        {
            return er != nullptr;
        }

        /**
         * @brief Gets the stored exception.
         * @return The stored exception.
         */
        std::exception_ptr get_error() const
        {
            return er;
        }

        /**
         * @brief Checks if the result has a value (void) or an exception.
         * @return True if the result has a value or an exception, false otherwise.
         */
        inline bool has_result() const
        {
            return has_value() || has_error();
        }

        /**
         * @brief Implicit conversion to bool.
         * @return True if the result has a value or an exception, false otherwise.
         */
        inline operator bool() const
        {
            return has_result();
        }
    };
} // namespace rpnx

#endif // RPNX_QUXLANG_RESULT_HEADER