// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef RANGE_HPP
#define RANGE_HPP
#include <typeindex>


#include <typeinfo>
#include <typeindex>
#include <utility>

namespace rpnx
{
    template <typename T>
    class type_info_holder
    {
    public:
        static inline const std::type_index type = std::type_index(typeid(T));
    };

    template <typename V>
    class dyn_input_iter
    {
        struct iterator_vtable
        {
            using get_input_f = V(*)(void* self);
            using copy_f = void*(*)(void const* self);
            using advance_f = void(*)(void* self);
            using delete_f = void(*)(void* self);

            get_input_f v_get_input = {};
            copy_f v_copy = {};
            advance_f v_advance = {};
            delete_f v_delete = {};
            std::type_index const* v_type = &type_info_holder< void >::type;
        };


        template <typename T>
        static constexpr iterator_vtable vtable = {
            .v_get_input = [](void* self) -> V { return **static_cast< T* >(self); },
            .v_copy = [](void const* self) -> void* { return new T(*static_cast< T const* >(self)); },
            .v_advance = [](void* self) -> void { ++(*static_cast< T* >(self)); },
            .v_delete = [](void* self) -> void { delete static_cast< T* >(self); },
            .v_type = &type_info_holder< T >::type,
        };

        iterator_vtable const* m_vtable;
        void* m_self;

    public:
        dyn_input_iter() : m_vtable(nullptr), m_self(nullptr)
        {
        }

        template <typename It>
        dyn_input_iter(It it) : m_vtable(&vtable< It >), m_self(new It(std::move(it)))
        {
        }

        dyn_input_iter(dyn_input_iter< V > const& other) :
            m_vtable(other.m_vtable),
            m_self(other.m_vtable->v_copy(other.m_self))
        {
        }

        V operator*() const
        {
            return m_vtable->v_get_input(m_self);
        }

        dyn_input_iter< V >& operator++()
        {
            m_vtable->v_advance(m_self);
            return *this;
        }

        dyn_input_iter< V > operator++(int)
        {
            dyn_input_iter< V > copy(*this);
            m_vtable->v_advance(m_self);
            return copy;
        }
    };

    template <typename V>
    class dyn_comparable_input_iter
    {
        struct iterator_vtable
        {
            using get_input_f = V(*)(void* self);
            using copy_f = void*(*)(void const* self);
            using advance_f = void(*)(void* self);
            using less_f = bool(*)(void const* self, void const* other);
            using equal_f = bool(*)(void const* self, void const* other);
            using delete_f = void(*)(void* self);

            get_input_f v_get_input = {};
            copy_f v_copy = {};
            advance_f v_advance = {};
            less_f v_less = {};
            equal_f v_equal = {};
            delete_f v_delete = {};
            std::type_index const* v_type = &type_info_holder< void >::type;
        };


        template <typename T>
        static constexpr iterator_vtable vtable = {
            .v_get_input = [](void* self) -> V { return **static_cast< T* >(self); },
            .v_copy = [](void const* self) -> void* { return new T(*static_cast< T const* >(self)); },
            .v_advance = [](void* self) -> void { ++(*static_cast< T* >(self)); },
            .v_less = [](void const* self, void const* other) -> bool { return *static_cast< T const* >(self) < *static_cast< T const* >(other); },
            .v_equal = [](void const* self, void const* other) -> bool { return *static_cast< T const* >(self) == *static_cast< T const* >(other); },
            .v_delete = [](void* self) -> void { delete static_cast< T* >(self); },
            .v_type = &type_info_holder< T >::type,
        };

        iterator_vtable const* m_vtable;
        void* m_self;

    public:
        dyn_comparable_input_iter() : m_vtable(nullptr), m_self(nullptr)
        {
        }

        template <typename It>
        dyn_comparable_input_iter(It it) : m_vtable(&vtable< It >), m_self(new It(std::move(it)))
        {
        }

        dyn_comparable_input_iter(dyn_comparable_input_iter< V > const& other) :
            m_vtable(other.m_vtable),
            m_self(other.m_vtable->v_copy(other.m_self))
        {
        }

        V operator*() const
        {
            return m_vtable->v_get_input(m_self);
        }

        dyn_comparable_input_iter< V >& operator++()
        {
            m_vtable->v_advance(m_self);
            return *this;
        }

        dyn_comparable_input_iter< V > operator++(int)
        {
            dyn_comparable_input_iter< V > copy(*this);
            m_vtable->v_advance(m_self);
            return copy;
        }

        bool operator<(dyn_comparable_input_iter< V > const& other) const
        {
            if (m_vtable == nullptr && other.m_vtable == nullptr)
            {
                return false;
            }
            if (m_vtable == nullptr)
            {
                return true;
            }
            if (other.m_vtable == nullptr)
            {
                return false;
            }
            if (m_vtable->v_type != other.m_vtable->v_type)
            {
                return m_vtable->v_type < other.m_vtable->v_type;
            }

            if (m_vtable->v_less(m_self, other.m_self))
            {
                return true;
            }

            return false;
        }

        bool operator==(dyn_comparable_input_iter< V > const& other) const
        {
            if (m_vtable == nullptr && other.m_vtable == nullptr)
            {
                return true;
            }
            if (m_vtable == nullptr || other.m_vtable == nullptr)
            {
                return false;
            }

            if (m_vtable->v_type != other.m_vtable->v_type)
            {
                return false;
            }

            return m_vtable->v_equal(m_self, other.m_self);
        }

        bool operator !=(dyn_comparable_input_iter< V > const& other) const
        {
            return !(*this == other);
        }
    };

    template <typename V>
    class dyn_bidirectional_input_iter
    {
        struct iterator_vtable
        {
            using get_value_f = V const &(*)(void* self);
            using copy_f = void*(*)(void const* self);
            using advance_f = void(*)(void* self);
            using recede_f = void(*)(void* self);
            using less_f = bool(*)(void const* self, void const* other);
            using equal_f = bool(*)(void const* self, void const* other);
            using delete_f = void(*)(void* self);

            get_value_f v_get_value = {};
            copy_f v_copy = {};
            advance_f v_advance = {};
            recede_f v_recede = {};
           // less_f v_less = {};
            equal_f v_equal = {};
            delete_f v_delete = {};
            std::type_index const* v_type = &type_info_holder< void >::type;
        };

        template <typename T>
        static constexpr iterator_vtable vtable = {
            .v_get_value = [](void* self) -> V const& { return **static_cast< T* >(self); },
            .v_copy = [](void const* self) -> void* { return new T(*static_cast< T const* >(self)); },
            .v_advance = [](void* self) -> void { ++(*static_cast< T* >(self)); },
            .v_recede = [](void* self) -> void { --(*static_cast< T* >(self)); },
        //    .v_less = [](void const* self, void const* other) -> bool { return *static_cast< T const* >(self) < *static_cast< T const* >(other); },
            .v_equal = [](void const* self, void const* other) -> bool { return *static_cast< T const* >(self) == *static_cast< T const* >(other); },
            .v_delete = [](void* self) -> void { delete static_cast< T* >(self); },
            .v_type = &type_info_holder< T >::type,
        };

        iterator_vtable const* m_vtable;
        void* m_self;

    public:
        dyn_bidirectional_input_iter() : m_vtable(nullptr), m_self(nullptr)
        {
        }

        template <typename It>
        dyn_bidirectional_input_iter(It it) : m_vtable(&vtable< It >), m_self(new It(std::move(it)))
        {
        }

        dyn_bidirectional_input_iter(dyn_bidirectional_input_iter< V > const& other) :
            m_vtable(other.m_vtable),
            m_self(other.m_vtable->v_copy(other.m_self))
        {
        }

        V operator*() const
        {
            return m_vtable->v_get_value(m_self);
        }

        dyn_bidirectional_input_iter< V >& operator++()
        {
            m_vtable->v_advance(m_self);
            return *this;
        }

        dyn_bidirectional_input_iter< V > operator++(int)
        {
            dyn_bidirectional_input_iter< V > copy(*this);
            m_vtable->v_advance(m_self);
            return copy;
        }

        dyn_bidirectional_input_iter< V >& operator--()
        {
            m_vtable->v_recede(m_self);
            return *this;
        }

        dyn_bidirectional_input_iter< V > operator--(int)
        {
            dyn_bidirectional_input_iter< V > copy(*this);
            m_vtable->v_recede(m_self);
            return copy;
        }
        /*

        bool operator<(dyn_bidirectional_input_iter< V > const& other) const
        {
            if (m_vtable == nullptr && other.m_vtable == nullptr)
            {
                return false;
            }
            if (m_vtable == nullptr)
            {
                return true;
            }
            if (other.m_vtable == nullptr)
            {
                return false;
            }
            if (m_vtable->v_type != other.m_vtable->v_type)
            {
                return m_vtable->v_type < other.m_vtable->v_type;
            }

            return m_vtable->v_less(m_self, other.m_self);
        }
*/
        bool operator==(dyn_bidirectional_input_iter< V > const& other) const
        {
            if (m_vtable == nullptr && other.m_vtable == nullptr)
            {
                return true;
            }
            if (m_vtable == nullptr || other.m_vtable == nullptr)
            {
                return false;
            }

            if (m_vtable->v_type != other.m_vtable->v_type)
            {
                return false;
            }

            return m_vtable->v_equal(m_self, other.m_self);
        }

        bool operator!=(dyn_bidirectional_input_iter< V > const& other) const
        {
            return !(*this == other);
        }

    };


    template <typename V>
    class dyn_input_range
    {
        dyn_comparable_input_iter< V > m_pos;
        dyn_comparable_input_iter< V > m_end;

    public:
        dyn_input_range(dyn_comparable_input_iter< V > begin, dyn_comparable_input_iter< V > end) : m_pos(begin), m_end(end)
        {
        }

        dyn_comparable_input_iter< V > begin() const
        {
            return m_pos;
        }

        dyn_comparable_input_iter< V > end() const
        {
            return m_end;
        }
    };


    template <typename V>
    class dyn_output_iter
    {
        struct iterator_vtable
        {
            using set_output_f = void(*)(void* self, V const& value);
            using copy_f = void*(*)(void const* self);
            using advance_f = void(*)(void* self);
            using delete_f = void(*)(void* self);

            set_output_f v_set_output = {};
            copy_f v_copy = {};
            advance_f v_advance = {};
            delete_f v_delete = {};
            std::type_index const* v_type = &type_info_holder< void >::type;
        };

        template <typename T>
        static constexpr iterator_vtable vtable = {
            .v_set_output = [](void* self, V const& value) { *(*static_cast< T* >(self)) = value; },
            .v_copy = [](void const* self) -> void* { return new T(*static_cast< T const* >(self)); },
            .v_advance = [](void* self) -> void { ++(*static_cast< T* >(self)); },
            .v_delete = [](void* self) -> void { delete static_cast< T* >(self); },
            .v_type = &type_info_holder< T >::type,
        };

        iterator_vtable const* m_vtable;
        void* m_self;

        struct proxy
        {
            dyn_output_iter< V > const* m_self;

            void operator=(V const& value)
            {
                m_self->m_vtable->v_set_output(m_self->m_self, value);
            }
        };

    public:
        dyn_output_iter() : m_vtable(nullptr), m_self(nullptr)
        {
        }

        template <typename It>
        dyn_output_iter(It it) : m_vtable(&vtable< It >), m_self(new It(std::move(it)))
        {
        }

        dyn_output_iter(dyn_output_iter< V >&& other) :
            m_vtable(other.m_vtable),
            m_self(other.m_self)
        {
            other.m_vtable = nullptr;
            other.m_self = nullptr;
        }


        dyn_output_iter(dyn_output_iter< V > const& other) :
            m_vtable(other.m_vtable),
            m_self(other.m_vtable->v_copy(other.m_self))
        {
        }


        template <typename It>
        dyn_output_iter& operator=(It it)
        {
            auto vt = &vtable< It >;
            auto self = new It(std::move(it));

            if (m_vtable)
            {
                m_vtable->v_delete(m_self);
            }

            m_vtable = vt;
            m_self = self;

            return *this;
        }

        dyn_output_iter& operator=(dyn_output_iter< V > const& other)
        {
            if (other.m_vtable == nullptr)
            {
                if (m_vtable)
                {
                    m_vtable->v_delete(m_self);
                }
                m_vtable = nullptr;
                m_self = nullptr;
                return *this;
            }
            auto copy = other.m_vtable->v_copy(other.m_self);

            m_vtable = other.m_vtable;
            m_self = copy;

            return *this;
        }

        dyn_output_iter& operator=(dyn_output_iter< V >&& other)
        {
            if (other.m_vtable == nullptr)
            {
                if (m_vtable)
                {
                    m_vtable->v_delete(m_self);
                }
                m_vtable = nullptr;
                m_self = nullptr;
                return *this;
            }
            auto copy = other.m_vtable->v_copy(other.m_self);

            m_vtable = other.m_vtable;
            m_self = copy;

            return *this;
        }

        dyn_output_iter& operator=(dyn_output_iter< V >& other)
        {
            if (other.m_vtable == nullptr)
            {
                if (m_vtable)
                {
                    m_vtable->v_delete(m_self);
                }
                m_vtable = nullptr;
                m_self = nullptr;
                return *this;
            }
            auto copy = other.m_vtable->v_copy(other.m_self);

            m_vtable = other.m_vtable;
            m_self = copy;

            return *this;
        }


        proxy operator*() const
        {
            return proxy{.m_self = this};
        }

        dyn_output_iter& operator++()
        {
            m_vtable->v_advance(m_self);
            return *this;
        }

        dyn_output_iter operator++(int)
        {
            dyn_output_iter copy(*this);
            m_vtable->v_advance(m_self);
            return copy;
        }

        ~dyn_output_iter()
        {
            if (m_self)
            {
                m_vtable->v_delete(m_self);
            }
        }

        // Note: Comparison operators (==, !=, <) are typically not used for output iterators
        // but could be implemented similarly to dyn_input_iter if needed.
    };


    class writer
    {
        struct writer_vtable
        {
            void*(*v_copy)(void* self);
            void (*v_destroy)(void* self);
            void (*v_write)(void* self, dyn_input_range< std::byte > data);
        };
    };
}


#endif //RANGE_HPP