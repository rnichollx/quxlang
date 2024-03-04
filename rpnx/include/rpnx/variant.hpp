//
// Created by rnicholl on 3/3/24.
//

#ifndef QUXLANG_VARIANT_HPP
#define QUXLANG_VARIANT_HPP

#include <compare>
#include <cstdint>
#include <cinttypes>
#include <tuple>
#include <memory>
#include <variant>

namespace rpnx {
    template<typename T, typename... Ts>
    struct index_of;

    // Base case: T matches the first type in the list.
    template<typename T, typename... Ts>
    struct index_of<T, T, Ts...> : std::integral_constant<std::size_t, 0> {
    };

    // Recursive case: T does not match the first type in the list.
    template<typename T, typename U, typename... Ts>
    struct index_of<T, U, Ts...> : std::integral_constant<std::size_t, 1 + index_of<T, Ts...>::value> {
    };

    template<typename Allocator>
    class variant_detail {
    public:
        using default_new_func = void *(*)(Allocator &);
        using copy_func = void *(*)(Allocator &, void const *);
        using delete_func = void (*)(Allocator &, void *) noexcept;
        using less_func = bool (*)(void const *, void const *);
        using equals_func = bool (*)(void const *, void const *);
        using three_way_func = std::strong_ordering(*)(void const *, void const *);
        using new_move_from_func = void *(*)(Allocator &, void *);

        struct variant_info {
            copy_func m_copy = nullptr;
            less_func m_less = nullptr;
            equals_func m_equals = nullptr;
            three_way_func m_three_way = nullptr;
            delete_func m_destroy = nullptr;
            default_new_func m_default_new = nullptr;
            new_move_from_func m_new_move_from = nullptr;
        };

        // Function templates follow...

        // Helper function to create a variant_info for a specific type
        template<typename T>
        static variant_info make_variant_info() {
            return variant_info{
                    .m_copy = &type_copy_func < T >,
                    .m_less = &type_less_func < T >,
                    .m_equals = &type_equals_func < T >,
                    .m_three_way = &type_three_way_func < T >,
                    .m_destroy = &type_delete_func < T >,
                    .m_default_new = &type_default_new_func < T >,
                    .m_new_move_from = &type_new_from_move_func < T >
            };
        }

    private:
        template<typename T>
        static void *type_copy_func(Allocator &allocator, void const *source) {
            using alloc_triats = std::allocator_traits<Allocator>;
            using rebound_alloc_type = typename alloc_triats::template rebind_alloc<T>;

            rebound_alloc_type rebound_alloc(allocator); // Rebound allocator for type T
            T *ptr = alloc_triats::allocate(rebound_alloc, 1); // Allocate space for one T

            try {
                alloc_triats::construct(rebound_alloc, ptr, *static_cast<T const *>(source)); // Construct T using the copy constructor
            } catch (...) {
                alloc_triats::deallocate(rebound_alloc, ptr, 1); // Ensure deallocation on exception
                throw; // Re-throw the exception
            }
            return ptr;
        }

        // Deallocation and destruction logic using Allocator
        template<typename T>
        static void type_delete_func(Allocator &allocator, void *object) noexcept {
            T *obj_ptr = static_cast<T *>(object);
            std::allocator_traits<Allocator>::destroy(allocator, obj_ptr); // Destroy the object
            std::allocator_traits<Allocator>::deallocate(allocator, obj_ptr, 1); // Deallocate memory
        }

        // Allocation and default construction logic using Allocator
        template<typename T>
        static void *type_default_new_func(Allocator &allocator) {
            using alloc_traits = std::allocator_traits<Allocator>;
            T *ptr = alloc_traits::allocate(allocator, 1); // Allocate space for one T
            try {
                alloc_traits::construct(allocator, ptr); // Default-construct T
            } catch (...) {
                alloc_traits::deallocate(allocator, ptr, 1); // Ensure deallocation on exception
                throw; // Re-throw the exception
            }
            return ptr;
        }

        template<typename T>
        static void *type_new_from_move_func(Allocator &allocator, void *source) {
            using alloc_traits = std::allocator_traits<Allocator>;
            using rebound_alloc_type = typename alloc_traits::template rebind_alloc<T>;

            rebound_alloc_type reboundAlloc(allocator); // Rebound allocator for type T
            T *ptr = alloc_traits::allocate(reboundAlloc, 1); // Allocate space for one T

            try {
                alloc_traits::construct(reboundAlloc, ptr, std::move(*static_cast<T *>(source))); // Move-construct T
            } catch (...) {
                alloc_traits::deallocate(reboundAlloc, ptr, 1); // Ensure deallocation on exception
                throw; // Re-throw the exception
            }
            return ptr;
        }

        // Function to compare two objects of type T for less-than
        template<typename T>
        bool type_less_func(void const *lhs, void const *rhs) {
            return *static_cast<T const *>(lhs) < *static_cast<T const *>(rhs);
        }

        // Function to check equality of two objects of type T
        template<typename T>
        bool type_equals_func(void const *lhs, void const *rhs) {
            return *static_cast<T const *>(lhs) == *static_cast<T const *>(rhs);
        }

        // Function to perform a three-way comparison of two objects of type T
        template<typename T>
        auto type_three_way_func(void const *lhs, void const *rhs) {
            if constexpr (std::three_way_comparable_with<T, T>) {
                // If T supports three-way comparison with itself
                return *static_cast<const T *>(lhs) <=> *static_cast<const T *>(rhs);
            } else {
                // Fallback for types without three-way comparison support.
                const T &l = *static_cast<const T *>(lhs);
                const T &r = *static_cast<const T *>(rhs);
                if (l < r) return std::strong_ordering::less;
                if (r < l) return std::strong_ordering::greater;
                return std::strong_ordering::equal;
            }
        }


    };

    template<typename Allocator, typename ... Ts>
    class basic_variant {
        struct variant_impl_info {
            typename variant_detail<Allocator>::variant_info m_general_info;
            std::size_t m_index;
        };

        template<std::size_t N>
        static constexpr variant_impl_info calc_info() {
            using type = std::tuple_element<N, std::tuple<Ts...>>::type;
            return variant_impl_info{.m_general_info = variant_detail<Allocator>::template make_variant_info<type>(),
                    .m_index = N};
        }

        template<std::size_t N>
        static constexpr variant_impl_info s_v_info_for = calc_info<N>();

        void *m_data = nullptr;
        variant_impl_info *m_vinf = nullptr;
        [[no_unique_address]] Allocator m_alloc;

    public:
        using allocator_type = Allocator;

        basic_variant(const allocator_type &alloc = allocator_type())
                : m_alloc(alloc) {

            m_vinf = &s_v_info_for<0>;
            try {
                m_data = m_vinf->m_general_info.m_default_new(m_alloc);

            }
            catch (...) {
                m_vinf = nullptr;
                throw;
            }
        }

        basic_variant(const basic_variant &other)
                : m_alloc(std::allocator_traits<Allocator>::select_on_container_copy_construction(other.m_alloc)) {
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

        ~basic_variant() {
            if (m_vinf != nullptr) {
                m_vinf->m_general_info.m_destroy(m_alloc, m_data);
            }
        }

        template <typename T2>
        basic_variant(T2 const & value, const allocator_type &alloc = allocator_type())
                : m_alloc(alloc) {
            static_assert(can_construct_with<T2>());

            constexpr std::size_t index = constructor_index<T2>();
            m_vinf = &s_v_info_for<index>;
            try {
                m_data = m_alloc.allocate(1);
                m_alloc.construct(m_data, value);
            }
            catch (...) {
                if (m_data != nullptr) {
                    m_alloc.deallocate(m_data, 1);
                }
                m_vinf = nullptr;
                throw;
            }
        }

        template <typename T>
        static constexpr bool can_construct_with() {
            // If T is convertible to any of the types in Ts..., return true
            return (std::is_convertible_v<T, Ts> || ...);
        }

        template <typename T>
        basic_variant<Allocator, Ts...> & operator=(T const & value) {
            static_assert(can_construct_with<T>());

            auto old_vinf = m_vinf;
            auto old_data = m_data;

            m_vinf = nullptr;
            m_data = nullptr;

            constexpr std::size_t index = constructor_index<T>();
            m_vinf = &s_v_info_for<index>;

            try {
                // Rebind allocator to allocate memory for type T
                using rebound_alloc_type = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
                rebound_alloc_type value_alloc(m_alloc); // Rebound allocator
                m_data = value_alloc.allocate(1); // Allocate memory for type T

                // Construct the value in the allocated memory
                std::allocator_traits<rebound_alloc_type>::construct(value_alloc, m_data, value);
            }
            catch (...) {
                if (m_data != nullptr) {
                    m_alloc.deallocate(m_data, 1);
                }
                m_vinf = nullptr;

                m_vinf = old_vinf;
                m_data = old_data;
                throw;
            }

            if (old_vinf != nullptr) {
                old_vinf->m_general_info.m_destroy(m_alloc, old_data);
            }
            return *this;
        }

        template<typename T>
        T& get_as() {
            // Check if the variant is currently holding a value of type T
            if (m_vinf == nullptr || m_vinf->m_index != index_of<T, Ts...>::value) {
                // If it is not, throw an exception
                throw std::bad_variant_access();
            }
            // If it is, return a reference to the value, casted to T
            return *static_cast<T*>(m_data);
        }

        bool operator==(basic_variant const & other) const {
            if (m_vinf == nullptr || other.m_vinf == nullptr) {
                throw std::bad_variant_access();
            }
            if (m_vinf->m_index != other.m_vinf->m_index) {
                return false;
            }
            return m_vinf->m_general_info.m_equals(m_data, other.m_data);
        }

        bool operator !=(basic_variant const & other) const {
            return !(*this == other);
        }

        bool operator <(basic_variant const & other) const {
            if (m_vinf == nullptr || other.m_vinf == nullptr) {
                throw std::bad_variant_access();
            }
            if (m_vinf->m_index != other.m_vinf->m_index) {
                return m_vinf->m_index < other.m_vinf->m_index;
            }
            return m_vinf->m_general_info.m_less(m_data, other.m_data);
        }

        bool operator >(basic_variant const & other) const {
            return other < *this;
        }

        bool operator <=(basic_variant const & other) const {
            return !(other < *this);
        }

        bool operator >=(basic_variant const & other) const {
            return !(*this < other);
        }

        std::strong_ordering operator<=>(basic_variant const & other) const {
            if (m_vinf == nullptr || other.m_vinf == nullptr) {
                throw std::bad_variant_access();
            }
            if (m_vinf->m_index != other.m_vinf->m_index) {
                return m_vinf->m_index <=> other.m_vinf->m_index;
            }
            return m_vinf->m_general_info.m_three_way(m_data, other.m_data);
        }

        template<typename T>
        bool type_is() const {
            // Check if the variant is currently holding a value of type T
            return m_vinf != nullptr && m_vinf->m_index == index_of<T, Ts...>::value;
        }



    private:

        template <typename T2>
        static constexpr bool has_cvref_removed_identical_type() {
            // If T2 is the same as any of the types in Ts..., return true
            return (std::is_same_v<std::remove_cvref_t<T2>, Ts> || ...);
        }

        template <typename T, std::size_t N>
        static constexpr std::size_t cvref_removed_identical_index(){
            if constexpr (std::is_same_v<std::remove_cvref_t<T>, std::tuple_element_t<N, std::tuple<Ts...>>>) {
                return N;
            } else {
                return cvref_removed_identical_index<T, N + 1>();
            }
        }

        template <typename T>
        static constexpr std::size_t constructor_index() {
            static_assert(can_construct_with<T>());

            // If T is the same as any of the types in Ts..., return the index of the first match assuming it is convertible
            // otherwise, return the index of the first type in Ts... that T is convertible to

            if constexpr (has_cvref_removed_identical_type<T>() && std::is_convertible_v<T, std::tuple_element_t<cvref_removed_identical_index<T, 0>(), std::tuple<Ts...>>>) {
                return cvref_removed_identical_index<T, 0>();
            } else {
                return convertible_index<T, 0>();
            }
        }


        template <typename T, std::size_t N>
        static constexpr std::size_t convertible_index() {
            static_assert(can_construct_with<T>());
            if constexpr (std::is_convertible_v<T, std::tuple_element_t<N, std::tuple<Ts...>>>) {
                return N;
            } else {
                return convertible_index<T, N + 1>();
            }
        }

    };

    template <typename ... Ts>
    using variant = basic_variant<std::allocator<void>, Ts...>;

}

#endif //QUXLANG_VARIANT_HPP
