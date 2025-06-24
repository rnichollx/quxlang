// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef RPNX_COW_HEADER_GUARD
#define RPNX_COW_HEADER_GUARD

#include <memory>

namespace rpnx
{
    /* Copy on write pointer */
    template < typename T >
    class cow
    {
        // Shared pointer to manage the memory of the object
        std::shared_ptr< T > ptr;

      public:
        // Default constructor that initializes the shared pointer with a new object of type T
        cow()
            : ptr(std::make_shared< T >())
        {
        }

        // Constructor that initializes the shared pointer with nullptr
        cow(std::nullptr_t n)
            : ptr(n)
        {
        }

        // Constructor that initializes the shared pointer with an object of type T
        cow(T t)
            : ptr(std::make_shared< T >(std::move(t)))
        {
        }

        // Variadic template constructor to allow construction with an arbitrary number of arguments
        template < typename... Ts >
        cow(Ts&&... ts)
            : ptr(std::make_shared< T >(std::forward< Ts >(ts)...))
        {
        }

        // Copy constructors
        cow(cow< T > const& other)
            : ptr(other.ptr)
        {
        }

        cow(cow< T >& other)
            : ptr(other.ptr)
        {
        }

        // Move constructors
        cow(cow< T > const&& other)
            : ptr(other.ptr)
        {
        }

        cow(cow< T >&& other)
            : ptr(std::move(other.ptr))
        {
        }

        // Comparison operators
        bool operator==(cow< T > const& other) const
        {
            return get() == other.get();
        }

        bool operator<(cow< T > const& other) const
        {
            return get() < other.get();
        }

        // Copy assignment operators
        cow< T >& operator=(cow< T > const& other)
        {
            ptr = other.ptr;
            return *this;
        }

        // Move assignment operators
        cow< T >& operator=(cow< T >&& other)
        {
            ptr = std::move(other.ptr);
            return *this;
        }

        // Assignment operators for objects of type T
        cow< T >& operator=(T&& other)
        {
            if (ptr.use_count() == 1)
            {
                *ptr = std::move(other);
            }
            else
            {
                ptr = std::make_shared< T >(std::move(other));
            }
            return *this;
        }

        cow< T >& operator=(T const& other)
        {
            if (ptr.use_count() == 1)
            {
                *ptr = other;
            }
            else
            {
                ptr = std::make_shared< T >(other);
            }
            return *this;
        }

        // Getter for the object of type T
        T const& get() const
        {
            return *ptr;
        }

        T const & read() const
        {
            return *ptr;
        }

        // Method to modify the object of type T
        T& edit()
        {
            if (ptr.use_count() > 1)
            {
                ptr = std::make_shared< T >(*ptr);
            }
            return *ptr;
        }

        // Method to set a new object of type T
        void set(T t)
        {
            ptr = std::make_shared< T >(std::move(t));
        }

        // Dereference operators
        T const& operator*() const
        {
            return *ptr;
        }

        T const* operator->() const
        {
            return ptr.get();
        }

        // Method to swap the shared pointers of two cow objects
        void swap(cow< T >& other)
        {
            ptr.swap(other.ptr);
        }

        auto serialize_interface() const
        {
            return std::tie(read());
        }

        auto deserialize_interface()
        {
            return std::tie(edit());
        }

        static constexpr auto strings()
        {
            return std::vector< std::string >{"cow_value"};
        }

        auto tie() const
        {
            return std::tie(read());
        }

        auto tie()
        {
            return std::tie(edit());
        }

        bool has_value() const
        {
            return ptr != nullptr;
        }

    };
} // namespace quxlang

#endif // QUXLANG_COW_HEADER_GUARD
