//
// Created by rnicholl on 3/3/24.
//

#ifndef RPNX_VARIANT_HPP
#define RPNX_VARIANT_HPP

#include <cassert>
#include <cinttypes>
#include <compare>
#include <cstdint>
#include <memory>
#include <tuple>
#include <typeindex>
#include <typeinfo>
#include <variant>

namespace rpnx
{

    template < typename T, typename... Ts >
    struct index_of;

    // Base case: T matches the first type in the list.
    template < typename T, typename... Ts >
    struct index_of< T, T, Ts... > : std::integral_constant< std::size_t, 0 >
    {
    };

    // Recursive case: T does not match the first type in the list.
    template < typename T, typename U, typename... Ts >
    struct index_of< T, U, Ts... > : std::integral_constant< std::size_t, 1 + index_of< T, Ts... >::value >
    {
    };

    template < typename Allocator, typename... Ts >
    class basic_variant;

    template < typename Allocator >
    class variant_detail
    {
      public:
        template < typename T >
        class is_supported_variant : public std::false_type
        {
        };

        template < typename Allocator2, typename... Ts >
        class is_supported_variant< basic_variant< Allocator2, Ts... > > : public std::true_type
        {
        };

        using default_new_func = void* (*)(Allocator&);
        using copy_func = void* (*)(Allocator&, void const*);
        using delete_func = void (*)(Allocator&, void*) noexcept;
        using less_func = bool (*)(void const*, void const*);
        using equals_func = bool (*)(void const*, void const*);
        using three_way_func = std::strong_ordering (*)(void const*, void const*);
        using new_move_from_func = void* (*)(Allocator&, void*);

        struct variant_info
        {
            copy_func m_copy = nullptr;
            less_func m_less = nullptr;
            equals_func m_equals = nullptr;
            three_way_func m_three_way = nullptr;
            delete_func m_destroy = nullptr;
            default_new_func m_default_new = nullptr;
            new_move_from_func m_new_move_from = nullptr;
            std::type_info const* m_type_info = nullptr;
        };

        // Function templates follow...

        // Helper function to create a variant_info for a specific type
        template < typename T >
        static constexpr variant_info make_variant_info()
        {
            return variant_info{.m_copy = &type_copy_func< T >, .m_less = &type_less_func< T >, .m_equals = &type_equals_func< T >, .m_three_way = &type_three_way_func< T >, .m_destroy = &type_delete_func< T >, .m_default_new = &type_default_new_func< T >, .m_new_move_from = &type_new_from_move_func< T >, .m_type_info = &typeid(T)};
        }

      private:
        template < typename T >
        static void* type_copy_func(Allocator& allocator, void const* source)
        {
            static_assert(!std::is_same_v< T, void >, "T must not be void");
            using alloc_triats = std::allocator_traits< Allocator >;
            using rebound_alloc_type = typename alloc_triats::template rebind_alloc< T >;
            using rebound_alloc_traits = std::allocator_traits< rebound_alloc_type >;

            rebound_alloc_type rebound_alloc(allocator);               // Rebound allocator for type T
            T* ptr = rebound_alloc_traits::allocate(rebound_alloc, 1); // Allocate space for one T

            try
            {
                rebound_alloc_traits::construct(rebound_alloc, ptr,
                                                *static_cast< T const* >(source)); // Construct T using the copy constructor
            }
            catch (...)
            {
                rebound_alloc_traits::deallocate(rebound_alloc, ptr, 1); // Ensure deallocation on exception
                throw;                                                   // Re-throw the exception
            }
            return ptr;
        }

        // Deallocation and destruction logic using Allocator
        template < typename T >
        static void type_delete_func(Allocator& allocator, void* object) noexcept
        {
            using rebound_allocator_type = typename std::allocator_traits< Allocator >::template rebind_alloc< T >;
            rebound_allocator_type typed_allocator(allocator); // Rebind the allocator to T
            T* obj_ptr = static_cast< T* >(object);
            std::allocator_traits< rebound_allocator_type >::destroy(typed_allocator, obj_ptr);       // Destroy the object
            std::allocator_traits< rebound_allocator_type >::deallocate(typed_allocator, obj_ptr, 1); // Deallocate memory
        }

        // Allocation and default construction logic using Allocator
        template < typename T >
        static void* type_default_new_func(Allocator& allocator)
        {
            using allocator_type = typename std::allocator_traits< Allocator >::template rebind_alloc< T >;
            using alloc_traits = std::allocator_traits< allocator_type >;

            allocator_type typed_allocator(allocator);           // Rebind the allocator to T
            T* ptr = alloc_traits::allocate(typed_allocator, 1); // Allocate space for one T

            try
            {
                alloc_traits::construct(typed_allocator, ptr); // Default-construct T
            }
            catch (...)
            {
                alloc_traits::deallocate(typed_allocator, ptr, 1); // Ensure deallocation on exception
                throw;                                             // Re-throw the exception
            }

            return ptr;
        }

        template < typename T >
        static void* type_new_from_move_func(Allocator& allocator, void* source)
        {
            using alloc_traits = std::allocator_traits< Allocator >;
            using rebound_alloc_type = typename alloc_traits::template rebind_alloc< T >;
            using rebound_alloc_traits = std::allocator_traits< rebound_alloc_type >;
            rebound_alloc_type reboundAlloc(allocator);                                      // Rebound allocator for type T
            T* ptr = std::allocator_traits< rebound_alloc_type >::allocate(reboundAlloc, 1); // Allocate space for one T

            try
            {
                rebound_alloc_traits::construct(reboundAlloc, ptr, std::move(*static_cast< T* >(source))); // Move-construct T
            }
            catch (...)
            {
                rebound_alloc_traits::deallocate(reboundAlloc, ptr, 1); // Ensure deallocation on exception
                throw;                                                  // Re-throw the exception
            }
            return ptr;
        }

        // Function to compare two objects of type T for less-than
        template < typename T >
        static bool type_less_func(void const* lhs, void const* rhs)
        {
            return *static_cast< T const* >(lhs) < *static_cast< T const* >(rhs);
        }

        // Function to check equality of two objects of type T
        template < typename T >
        static constexpr bool type_equals_func(void const* lhs, void const* rhs)
        {
            // If
            if constexpr (std::equality_comparable_with< T, T >)
            {
                return *static_cast< T const* >(lhs) == *static_cast< T const* >(rhs);
            }
            else
            {
                // synthesize from <=>
                return *static_cast< T const* >(lhs) <=> *static_cast< T const* >(rhs) == std::strong_ordering::equal;
            }
        }

        // Function to perform a three-way comparison of two objects of type T
        template < typename T >
        static constexpr auto type_three_way_func(void const* lhs, void const* rhs)
        {
            if constexpr (std::three_way_comparable_with< T, T >)
            {
                // If T supports three-way comparison with itself
                return *static_cast< const T* >(lhs) <=> *static_cast< const T* >(rhs);
            }
            else
            {
                // Fallback for types without three-way comparison support.
                const T& l = *static_cast< const T* >(lhs);
                const T& r = *static_cast< const T* >(rhs);

                static_assert(std::is_same< decltype(l), decltype(r) >::value, "T must be the same type as T");
                if (l < r)
                    return std::strong_ordering::less;
                if (r < l)
                    return std::strong_ordering::greater;
                return std::strong_ordering::equal;
            }
        }
    };

    template < typename... Ts >
    using variant = basic_variant< std::allocator< void >, Ts... >;

    template < typename T, typename... Ts >
    inline constexpr auto& get_as(variant< Ts... >& v)
    {
        return v.template get_as< T >();
    }

    template < typename T, typename... Ts >
    inline constexpr auto const& get_as(variant< Ts... > const& v)
    {
        return v.template get_as< T >();
    }

    enum class call_type { required, optional, except_on_missing };

    template < typename V, typename F, typename R, std::size_t N, call_type C >
    inline R apply_nth_visitor(V&& variant, F&& func)
    {
        if constexpr (C == call_type::required)
        {
            return func(variant.template get_n< N >());
        }
        else if constexpr (C == call_type::except_on_missing)
        {
            if (std::is_invocable< F, decltype(variant.template get_n< N >()) >::value)
            {
                return func(variant.template get_n< N >());
            }
            else
            {
                throw std::bad_variant_access();
            }
        }
        else if constexpr (C == call_type::optional)
        {
            if (std::is_invocable< F, decltype(variant.template get_n< N >()) >::value)
            {
                return func(variant.template get_n< N >());
            }
            else if constexpr (!std::is_same< R, void >::value)
            {
                return R{};
            }
        }
    }

    template < typename V, typename F, typename R >
    using variant_invoke_executor = R (*)(V&&, F&&);

    template < typename F, typename R, typename A, typename... Vs >
    auto consteval variant_invoke_table_gen()
    {
        using vexecptr = variant_invoke_executor< rpnx::basic_variant< A, Vs... >&, F, R >;
        std::array< vexecptr, std::tuple_size_v< std::tuple< Vs... > > > result{};

        update_variant_invoke_table_lvalue< 0, F, R, A, Vs... >(result);

        return result;
    }

    template < typename V, std::size_t N >
    class variant_nth_member;

    template < typename A, typename... Vs, std::size_t N >
    class variant_nth_member< rpnx::basic_variant< A, Vs... >, N >
    {
      public:
        using type = std::tuple_element_t< N, std::tuple< Vs... > >;
    };

    template < typename V >
    class variant_size;

    template < typename A, typename... Vs >
    class variant_size< rpnx::basic_variant< A, Vs... > >
    {
      public:
        using type = std::integral_constant< std::size_t, sizeof...(Vs) >;
        static constexpr std::size_t value = sizeof...(Vs);
    };

    template < typename V >
    static constexpr std::size_t variant_size_v = variant_size< V >::value;

    template < typename V, std::size_t N >
    using variant_nth_member_t = typename variant_nth_member< V, N >::type;

    template < typename F, typename R, typename V, call_type C >
    auto constexpr variant_invoke_table_gen2()
    {
        using vexecptr = variant_invoke_executor< V, F, R >;
        std::array< vexecptr, variant_size_v< std::remove_cvref_t< V > > > result{};

        update_variant_invoke_table2< 0, F, R, V, C >(result);

        return result;
    }

    template < std::size_t N, typename F, typename R, typename V, call_type C >
    constexpr void update_variant_invoke_table2(std::array< variant_invoke_executor< V, F, R >, variant_size_v< std::remove_cvref_t< V > > >& table)
    {
        using invoke_ptr = variant_invoke_executor< V, F, R >;

        if constexpr (N < variant_size_v< std::remove_cvref_t< V > >)
        {
            invoke_ptr ptr = &apply_nth_visitor< V, F, R, N, C >;
            table[N] = ptr;
            update_variant_invoke_table2< N + 1, F, R, V, C >(table);
        }
    }

    template < typename F, typename R, typename V, call_type C >
    inline constexpr auto variant_invoke_table2 = variant_invoke_table_gen2< F, R, V, C >();

    template < typename R, typename V, typename F >
    inline R apply_visitor(F&& func, V&& variant)
    {
        return variant_invoke_table2< F, R, V&&, call_type::required >[variant.index()](std::forward< V >(variant), std::forward< F >(func));
    }

    template < typename R, typename V, typename F >
    inline R apply_visitor_checked(F&& func, V&& variant)
    {
        return variant_invoke_table2< F, R, V&&, call_type::except_on_missing >[variant.index()](std::forward< V >(variant), std::forward< F >(func));
    }

    template < typename R, typename V, typename F >
    inline R try_apply_visitor(F&& func, V&& variant)
    {
        return variant_invoke_table2< F, R, V&&, call_type::optional >[variant.index()](std::forward< V >(variant), std::forward< F >(func));
    }

    template < typename A, typename... Ts >
    class variant_convert_to
    {
        basic_variant< A, Ts... >& m_val;

      public:
        variant_convert_to(basic_variant< A, Ts... >& val)
            : m_val(val)
        {
        }
        template < typename T2 >
        bool operator()(T2&& other) const
        {
            m_val = std::forward< T2 >(other);
            return true;
        }
    };

    template < typename Allocator, typename... Ts >
    class basic_variant
    {
        struct variant_impl_info
        {
            typename variant_detail< Allocator >::variant_info m_general_info;
            std::size_t m_index;
        };

        template < std::size_t N >
        static consteval variant_impl_info calc_info()
        {
            using type = typename std::tuple_element< N, std::tuple< Ts... > >::type;
            variant_impl_info result{};
            auto v_info = variant_detail< Allocator >::template make_variant_info< type >();

            result.m_general_info = v_info;
            result.m_index = N;
            return result;
        }

        template < std::size_t N >
        static constexpr variant_impl_info s_v_info_for = calc_info< N >();

        void* m_data = nullptr;
        variant_impl_info const* m_vinf = nullptr;
        [[no_unique_address]] Allocator m_alloc;

        template < typename T2 >
        static constexpr bool has_cvref_removed_identical_type()
        {
            // If T2 is the same as any of the types in Ts..., return true
            return (std::is_same_v< std::remove_cvref_t< T2 >, Ts > || ...);
        }

      public:
        using allocator_type = Allocator;

        constexpr basic_variant(const allocator_type& alloc = allocator_type())
            : m_alloc(alloc)
        {
            assert((m_vinf == nullptr) == (m_data == nullptr));
            m_vinf = &s_v_info_for< 0 >;
            try
            {
                m_data = m_vinf->m_general_info.m_default_new(m_alloc);
            }
            catch (...)
            {
                m_vinf = nullptr;
                throw;
            }
        }

        constexpr basic_variant(basic_variant< Allocator, Ts... >&& other)
            : m_alloc(std::move(other.m_alloc))
        {
            assert((m_vinf == nullptr) == (m_data == nullptr));

            m_vinf = nullptr;
            m_data = nullptr;

            std::swap(m_vinf, other.m_vinf);
            std::swap(m_data, other.m_data);

            assert((m_vinf == nullptr) == (m_data == nullptr));
        }

        constexpr basic_variant(basic_variant< Allocator, Ts... > const& other)
            : m_alloc(std::allocator_traits< Allocator >::select_on_container_copy_construction(other.m_alloc))
        {
            m_vinf = nullptr;
            m_data = nullptr;
            assert((other.m_vinf == nullptr) == (other.m_data == nullptr));

            if (other.m_vinf == nullptr)
            {
                return;
            }
            m_vinf = other.m_vinf;
            try
            {
                m_data = m_vinf->m_general_info.m_copy(m_alloc, other.m_data);
            }
            catch (...)
            {
                m_vinf = nullptr;
                throw;
            }
        }

        ~basic_variant()
        {
            assert((m_vinf == nullptr) == (m_data == nullptr));
            reset();
            assert((m_vinf == nullptr) == (m_data == nullptr));
        }

        void reset()
        {
            assert((m_vinf == nullptr) == (m_data == nullptr));
            if (m_vinf != nullptr)
            {
                assert(m_data != nullptr);
                m_vinf->m_general_info.m_destroy(m_alloc, m_data);
                m_data = nullptr;
                m_vinf = nullptr;
            }
            assert((m_vinf == nullptr) == (m_data == nullptr));
        }

        template < typename... Ts2 >
        basic_variant(basic_variant< Allocator, Ts2... > const& other, std::enable_if_t< !std::is_same_v< basic_variant< Allocator, Ts... >, basic_variant< Allocator, Ts2... > > && !has_cvref_removed_identical_type< basic_variant< Allocator, Ts2... > >(), int > = 0)
            : basic_variant()
        {
            reset();
            assert((m_vinf == nullptr) == (m_data == nullptr));
            rpnx::apply_visitor< bool >(variant_convert_to< Allocator, Ts... >(*this), other);
        }

        template < typename T >
        static consteval bool can_construct_subtype_with()
        {
            // Don't construct members using a reference to selftype, even if this looks possible
            // because this is usually not what was intended.
            // This can occur for example,  expression = variant<plus, negate>, struct negate { expression expr; }
            // In this case, a negate can be constructed using a single expression argument, which can ab
            auto constexpr ok1 = !std::is_same_v< std::remove_cvref_t< T >, basic_variant< Allocator, Ts... > >;

            // And there must be some constructible member type
            auto constexpr ok2 = (std::is_convertible_v< T, Ts > || ...);
            return ok1 && ok2;
        }

        template < typename T2 >
        constexpr basic_variant(T2 const& value, const allocator_type& alloc = allocator_type(), std::enable_if_t< rpnx::basic_variant< Allocator, Ts... >::can_construct_subtype_with< T2 >(), int > = 0)
            : m_alloc(alloc)
        {

            constexpr std::size_t index = constructor_index< T2 >();
            using rebound_alloc_type = typename std::allocator_traits< allocator_type >::template rebind_alloc< T2 >;
            rebound_alloc_type rebound_alloc(alloc);

            m_vinf = &s_v_info_for< index >;
            try
            {
                // Rebind allocator to allocate memory for type T2
                // Rebound allocator
                m_data = std::allocator_traits< rebound_alloc_type >::allocate(rebound_alloc,
                                                                               1); // Allocate memory for type T2

                // Construct the value in the allocated memory
                std::allocator_traits< rebound_alloc_type >::construct(rebound_alloc, static_cast< T2* >(m_data), value);
            }
            catch (...)
            {
                if (m_data != nullptr)
                {
                    rebound_alloc.deallocate(static_cast< T2* >(m_data), 1);
                }
                m_vinf = nullptr;

                throw;
            }
            assert((m_vinf == nullptr) == (m_data == nullptr));
        }

        template < typename T >
        constexpr basic_variant< Allocator, Ts... >& operator=(T const& value)
        {
            static_assert(can_construct_subtype_with< T >());
            assert((m_vinf == nullptr) == (m_data == nullptr));

            auto old_vinf = m_vinf;
            auto old_data = m_data;

            m_vinf = nullptr;
            m_data = nullptr;

            constexpr std::size_t index = constructor_index< T >();
            m_vinf = &s_v_info_for< index >;

            // Rebind allocator to allocate memory for type T
            using rebound_alloc_type = typename std::allocator_traits< allocator_type >::template rebind_alloc< T >;
            rebound_alloc_type value_alloc(m_alloc); // Rebound allocator
            try
            {
                m_data = value_alloc.allocate(1); // Allocate memory for type T
                // Construct the value in the allocated memory
                std::allocator_traits< rebound_alloc_type >::construct(value_alloc, static_cast< T* >(m_data), value);
            }
            catch (...)
            {
                if (m_data != nullptr)
                {
                    std::allocator_traits< rebound_alloc_type >::deallocate(value_alloc, static_cast< T* >(m_data), 1);
                }
                m_vinf = nullptr;

                m_vinf = old_vinf;
                m_data = old_data;
                old_vinf = nullptr;
                old_data = nullptr;
                throw;
            }

            if (old_vinf != nullptr)
            {
                old_vinf->m_general_info.m_destroy(m_alloc, old_data);
            }
            assert((m_vinf == nullptr) == (m_data == nullptr));
            return *this;
        }

        basic_variant< Allocator, Ts... >& operator=(basic_variant< Allocator, Ts... > const& other)
        {
            assert((m_vinf == nullptr) == (m_data == nullptr));
            reset();

            m_alloc = other.m_alloc;
            m_vinf = other.m_vinf;
            m_data = m_vinf->m_general_info.m_copy(m_alloc, other.m_data);

            assert((m_vinf == nullptr) == (m_data == nullptr));
            return *this;
        }

        template < typename T >
        T& get_as()
        {
            assert((m_vinf == nullptr) == (m_data == nullptr));
            // static_assert(has_cvref_removed_identical_type<T>(), "Must be in type list");
            //  Check if the variant is currently holding a value of type T
            if (m_vinf == nullptr || m_vinf->m_index != index_of< T, Ts... >::value)
            {
                // If it is not, throw an exception
                throw std::bad_variant_access();
            }
            // If it is, return a reference to the value, casted to T
            return *static_cast< T* >(m_data);
        }

        template < typename T >
        T const& get_as() const
        {
            assert((m_vinf == nullptr) == (m_data == nullptr));
            static_assert(has_cvref_removed_identical_type< T >(), "Must be in type list");

            // Check if the variant is currently holding a value of type T
            if (m_vinf == nullptr || m_vinf->m_index != index_of< T, Ts... >::value)
            {
                // If it is not, throw an exception
                throw std::bad_variant_access();
            }
            assert((m_vinf == nullptr) == (m_data == nullptr));
            // If it is, return a reference to the value, casted to T
            return *static_cast< T const* >(m_data);
        }

        template < std::size_t N >
        auto const& get_n() const
        {
            assert((m_vinf == nullptr) == (m_data == nullptr));

            // Check if the variant is currently holding a value of type T
            if (m_vinf == nullptr || m_vinf->m_index != N)
            {
                // If it is not, throw an exception
                throw std::bad_variant_access();
            }

            assert((m_vinf == nullptr) == (m_data == nullptr));
            using type = std::tuple_element_t< N, std::tuple< Ts... > >;
            // If it is, return a reference to the value, casted to T
            return *static_cast< type const* >(m_data);
        }

        bool operator==(basic_variant< Allocator, Ts... > const& other) const
        {
            assert((m_vinf == nullptr) == (m_data == nullptr));
            if (m_vinf == nullptr || other.m_vinf == nullptr)
            {
                throw std::bad_variant_access();
            }
            if (m_vinf->m_index != other.m_vinf->m_index)
            {
                return false;
            }
            assert((m_vinf == nullptr) == (m_data == nullptr));
            return m_vinf->m_general_info.m_equals(m_data, other.m_data);
        }

        bool operator!=(basic_variant< Allocator, Ts... > const& other) const
        {
            assert((m_vinf == nullptr) == (m_data == nullptr));
            return !(*this == other);
        }

      public:
        bool operator<(basic_variant< Allocator, Ts... > const& other) const
        {
            assert((m_vinf == nullptr) == (m_data == nullptr));
            if (m_vinf == nullptr || other.m_vinf == nullptr)
            {
                throw std::bad_variant_access();
            }
            if (m_vinf->m_index != other.m_vinf->m_index)
            {
                return m_vinf->m_index < other.m_vinf->m_index;
            }
            assert((m_vinf == nullptr) == (m_data == nullptr));
            return m_vinf->m_general_info.m_less(m_data, other.m_data);
        }

      public:
        std::strong_ordering operator<=>(basic_variant< Allocator, Ts... > const& other) const
        {
            if (m_vinf == nullptr || other.m_vinf == nullptr)
            {
                throw std::bad_variant_access();
            }

            if (m_vinf->m_index != other.m_vinf->m_index)
            {
                return m_vinf->m_index <=> other.m_vinf->m_index;
            }
            return m_vinf->m_general_info.m_three_way(m_data, other.m_data);
        }

        template < typename T >
        bool type_is() const
        {
            assert((m_vinf == nullptr) == (m_data == nullptr));
            // Check if the variant is currently holding a value of type T
            return m_vinf != nullptr && m_vinf->m_index == index_of< T, Ts... >::value;
        }

        std::type_info const& type() const
        {
            if (m_vinf == nullptr)
            {
                throw std::bad_variant_access();
            }
            return *m_vinf->m_general_info.m_type_info;
        }

        std::type_index type_index() const
        {
            return std::type_index(type());
        }

        template < std::size_t N >
        auto& get_n()
        {
            if (m_vinf == nullptr || m_vinf->m_index != N)
            {
                throw std::bad_variant_access();
            }
            return *static_cast< std::tuple_element_t< N, std::tuple< Ts... > >* >(m_data);
        }

        std::size_t index() const
        {
            if (m_vinf == nullptr)
            {
                throw std::bad_variant_access();
            }
            return m_vinf->m_index;
        }

      private:
        template < typename T, std::size_t N >
        static constexpr std::size_t cvref_removed_identical_index()
        {
            if constexpr (N >= sizeof...(Ts))
            {
                return N;
            }
            else
            {
                if constexpr (std::is_same_v< std::remove_cvref_t< T >, std::tuple_element_t< N, std::tuple< Ts... > > >)
                {
                    return N;
                }
                else
                {
                    return cvref_removed_identical_index< T, N + 1 >();
                }
            }
        }

        template < typename T >
        static constexpr std::size_t constructor_index()
        {
            static_assert(can_construct_subtype_with< T >());

            // If T is the same as any of the types in Ts..., return the index of the first match assuming it is convertible
            // otherwise, return the index of the first type in Ts... that T is convertible to

            if constexpr (has_cvref_removed_identical_type< T >() && std::is_convertible_v< T, std::tuple_element_t< cvref_removed_identical_index< T, 0 >(), std::tuple< Ts... > > >)
            {
                return cvref_removed_identical_index< T, 0 >();
            }
            else
            {
                return convertible_index< T, 0 >();
            }
        }

        template < typename T, std::size_t N >
        static constexpr std::size_t convertible_index()
        {
            static_assert(can_construct_subtype_with< T >());
            if constexpr (std::is_convertible_v< T, std::tuple_element_t< N, std::tuple< Ts... > > >)
            {
                return N;
            }
            else
            {
                return convertible_index< T, N + 1 >();
            }
        }
    };

} // namespace rpnx

#endif // QUXLANG_VARIANT_HPP

#ifdef RPNX_SERIALIZER_HPP
#include "rpnx/compat/variant_serializer.hpp"
#endif