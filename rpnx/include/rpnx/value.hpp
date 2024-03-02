//
// Created by Ryan Nicholl on 2/28/23.
//

#ifndef QUXLANG__VALUE_HEADER
#define QUXLANG__VALUE_HEADER

#include <memory>
#include <type_traits>
#include <utility>

namespace rpnx
{

    template < typename T >
    concept less_than_comparable = requires(T a, T b) {
        {
            a < b
        } -> std::convertible_to< bool >; // Requires the expression 'a < b' to be valid and convertible to bool
    };
    // The class "value" implements a pointer-indirected value
    // It behaves like the underlying type, e.g. it can be copied, moved, etc.
    // However, the pointer indirection allows for values to refer to themselves
    // in variants.
    // For example, a type_reference is  a value of e.g. variant<symbol, pointer, array>,
    // and an array is a size plus type reference, and a pointer contains a type reference.
    // This would create a circular dependency, so we use a pointer indirection
    // to allow such objects to be constructed.
    // It goes without saying, but ensure the default constructor of a value does not
    // infinitely recurse. E.g. for a variant, ensure the default constructor of the
    // variant constructs a non-recursive type.
    template < typename T >
    struct value
    {
        T* m_data;

      public:
        template < typename... Ts >
        value(Ts&&... args)
            : m_data(new T(std::forward< Ts >(args)...))
        {
        }

        value(value< T > const& other)
            : m_data(new T(other.get()))
        {
        }

        value(value< T >&& other)
            : m_data(new T(std::move(other.get())))
        {
        }

        value< T >& operator=(value< T > const& other)
        {
            reset();
            m_data = new T(other.get());
            return *this;
        }

        value< T >& operator=(value< T >&& other)
        {
            reset();
            m_data = new T(std::move(other.get()));
            return *this;
        }

        void reset()
        {
            if (m_data)
            {
                delete m_data;
            }
            m_data = nullptr;
        }

        ~value()
        {
            reset();
        }

        T& get()
        {
            return *m_data;
        }

        T const& get() const
        {
            return *m_data;
        }

        bool operator<(value< T > const& other) const
        {
            static_assert(less_than_comparable< T >, "T must be less than comparable");
            return get() < other.get();
        }

        bool operator==(value< T > const& other) const
        {
            static_assert(less_than_comparable< T >, "T must be less than comparable");

            return get() == other.get();
        }
    };

    class unimplemented
    {
      public:
        unimplemented()
        {
            throw std::logic_error("Unimplemented");
        }

        template < typename T >
        operator T()
        {
            throw std::logic_error("Unimplemented");
        }

        template <typename ... Ts>
        unimplemented & operator()(Ts&&...)
        {
            throw std::logic_error("Unimplemented");
        }



    };
} // namespace rpnx

#endif // QUXLANG__VALUE_HEADER
