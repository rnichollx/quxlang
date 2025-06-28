//
// Created by Ryan Nicholl on 2025-06-26.
//

#ifndef UINT64_BASE_HPP
#define UINT64_BASE_HPP

#include <cstdint>

namespace rpnx
{
    template < typename derived_t >
    class uint64_base
    {
      private:
        std::uint64_t value_;

      public:
        // Default constructor
        uint64_base() : value_(0)
        {
        }

        // Constructor from std::uint64_t
        explicit uint64_base(std::uint64_t val) : value_(val)
        {
        }

        // Copy constructor (for the same derived type)
        uint64_base(const derived_t& other) : value_(other.value_)
        {
        }

        // Assignment operator (for the same derived type)
        derived_t& operator=(const derived_t& other)
        {
            if (this != &other)
            {
                value_ = other.value_;
            }
            return static_cast< derived_t& >(*this);
        }

        // Implicit conversion to std::uint64_t (for operations with raw uint64_t)
        operator std::uint64_t() const
        {
            return value_;
        }

        // Arithmetic operators
        derived_t operator+(const derived_t& other) const
        {
            return derived_t(value_ + other.value_);
        }

        derived_t operator-(const derived_t& other) const
        {
            return derived_t(value_ - other.value_);
        }

        derived_t operator*(const derived_t& other) const
        {
            return derived_t(value_ * other.value_);
        }

        derived_t operator/(const derived_t& other) const
        {
            // You might want to add error handling for division by zero
            return derived_t(value_ / other.value_);
        }

        derived_t operator%(const derived_t& other) const
        {
            return derived_t(value_ % other.value_);
        }

        // Compound assignment operators
        derived_t& operator+=(const derived_t& other)
        {
            value_ += other.value_;
            return static_cast< derived_t& >(*this);
        }

        derived_t& operator-=(const derived_t& other)
        {
            value_ -= other.value_;
            return static_cast< derived_t& >(*this);
        }

        derived_t& operator*=(const derived_t& other)
        {
            value_ *= other.value_;
            return static_cast< derived_t& >(*this);
        }

        derived_t& operator/=(const derived_t& other)
        {
            value_ /= other.value_;
            return static_cast< derived_t& >(*this);
        }

        derived_t& operator%=(const derived_t& other)
        {
            value_ %= other.value_;
            return static_cast< derived_t& >(*this);
        }

        // Increment and Decrement operators
        derived_t& operator++()
        { // Pre-increment
            ++value_;
            return static_cast< derived_t& >(*this);
        }

        derived_t operator++(int)
        { // Post-increment
            derived_t temp = static_cast< derived_t& >(*this);
            ++value_;
            return temp;
        }

        derived_t& operator--()
        { // Pre-decrement
            --value_;
            return static_cast< derived_t& >(*this);
        }

        derived_t operator--(int)
        { // Post-decrement
            derived_t temp = static_cast< derived_t& >(*this);
            --value_;
            return temp;
        }

        // Comparison operators
        bool operator==(const derived_t& other) const
        {
            return value_ == other.value_;
        }

        bool operator==( uint64_base<derived_t> const& other) const
        {
            return value_ == other.value_;
        }

        bool operator!=(const derived_t& other) const
        {
            return value_ != other.value_;
        }

        bool operator!=(uint64_base<derived_t> const& other) const
        {
            return value_ != other.value_;
        }

        bool operator<(const derived_t& other) const
        {
            return value_ < other.value_;
        }

        bool operator>(const derived_t& other) const
        {
            return value_ > other.value_;
        }

        bool operator<=(const derived_t& other) const
        {
            return value_ <= other.value_;
        }

        bool operator>=(const derived_t& other) const
        {
            return value_ >= other.value_;
        }

        // Bitwise operators
        derived_t operator&(const derived_t& other) const
        {
            return derived_t(value_ & other.value_);
        }

        derived_t operator|(const derived_t& other) const
        {
            return derived_t(value_ | other.value_);
        }

        derived_t operator^(const derived_t& other) const
        {
            return derived_t(value_ ^ other.value_);
        }

        derived_t operator~() const
        {
            return derived_t(~value_);
        }

        derived_t operator<<(int shift) const
        {
            return derived_t(value_ << shift);
        }

        derived_t operator>>(int shift) const
        {
            return derived_t(value_ >> shift);
        }

        // Compound bitwise assignment operators
        derived_t& operator&=(const derived_t& other)
        {
            value_ &= other.value_;
            return static_cast< derived_t& >(*this);
        }

        derived_t& operator|=(const derived_t& other)
        {
            value_ |= other.value_;
            return static_cast< derived_t& >(*this);
        }

        derived_t& operator^=(const derived_t& other)
        {
            value_ ^= other.value_;
            return static_cast< derived_t& >(*this);
        }

        derived_t& operator<<=(int shift)
        {
            value_ <<= shift;
            return static_cast< derived_t& >(*this);
        }

        derived_t& operator>>=(int shift)
        {
            value_ >>= shift;
            return static_cast< derived_t& >(*this);
        }

        // Serialization interfaces
        std::uint64_t const& serialize_interface() const
        {
            return value_;
        }

        std::uint64_t& deserialize_interface()
        {
            return value_;
        }

        auto tie() const
        {
            return std::tie(value_);
        }
        auto tie()
        {
            return std::tie(value_);
        }

        static auto constexpr strings()
        {
            return std::vector< std::string >{ "value" };
        }


    };
} // namespace rpnx

// RPNX_UNIQUE_U64 implements a unique type derived from uint64_base
// This type has the properties of a uint64_t, but different types
// cannot interconvert with each other, even if they have the same
// bit width.
#define RPNX_UNIQUE_U64(name) \
    struct name : public rpnx::uint64_base< name > \
    { \
        using rpnx::uint64_base< name >::uint64_base; \
        static constexpr auto class_name() { return #name; } \
        using rpnx::uint64_base< name >::operator=; \
        name() = default; \
        using rpnx::uint64_base< name >::operator==; \
        using rpnx::uint64_base< name >::operator!=; \
        using rpnx::uint64_base< name >::operator<; \
        using rpnx::uint64_base< name >::operator>; \
        using rpnx::uint64_base< name >::operator<=; \
        using rpnx::uint64_base< name >::operator>=; \
    };

#endif // UINT64_BASE_HPP
