//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RPNX_RYANSCRIPT1031_COW_HEADER
#define RPNX_RYANSCRIPT1031_COW_HEADER

#include <memory>

namespace rylang
{
    /* Copy on write pointer */
    template < typename T >
    class cow
    {
        std::shared_ptr< T > ptr;

      public:
        cow(T t)
            : ptr(std::make_shared< T >(std::move(t)))
        {
        }

        template <typename ... Ts>
        cow(Ts&& ... ts)
            : ptr(std::make_shared< T >(std::forward<Ts>(ts)...))
        {
        }

        cow(cow< T > const& other)
            : ptr(other.ptr)
        {
        }

        cow(cow< T > & other)
            : ptr(other.ptr)
        {
        }

        cow(cow< T > const && other)
            : ptr(other.ptr)
        {
        }

        cow(cow< T >&& other)
            : ptr(std::move(other.ptr))
        {
        }

        bool operator==(cow< T > const& other) const
        {
            return get() == other.get();
        }

        bool operator<(cow< T > const& other) const
        {
            return get() < other.get();
        }

        cow< T >& operator=(cow< T > const& other)
        {
            ptr = other.ptr;
            return *this;
        }

        cow< T >& operator=(cow< T >&& other)
        {
            ptr = std::move(other.ptr);
            return *this;
        }

        T const& get() const
        {
            return *ptr;
        }

        T& edit()
        {
            if (ptr.use_count() > 1)
            {
                ptr = std::make_shared< T >(*ptr);
            }
            return *ptr;
        }

        void set(T t)
        {
            ptr = std::make_shared< T >(std::move(t));
        }

        T const& operator*() const
        {
            return *ptr;
        }

        T const* operator->() const
        {
            return ptr.get();
        }

        void swap(cow< T > & other)
        {
            ptr.swap(other.ptr);
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_COW_HEADER
