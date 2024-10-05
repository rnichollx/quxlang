// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef RPNX_METADATA_H_HPP
#define RPNX_METADATA_H_HPP

#include <optional>
#include <set>
#include <vector>

namespace rpnx
{
    template <typename E> struct enum_traits;
}

#include "rpnx/compare.hpp"

#define RPNX_EXPAND(x) x

#define RPNX_EMPTY_METADATA(ty) \
    auto tie() const { return std::tie(); } \
    auto tie() { return std::tie(); } \
    auto serial_interface() const { return tie(); } \
    auto serial_interface() { return tie(); } \
    auto operator<=>(ty const& other) const { return rpnx::compare(tie(), other.tie()); } \
    bool operator==(ty const& other) const { return tie() == other.tie(); } \
    bool operator!=(ty const& other) const { return tie() != other.tie(); } \
    static auto constexpr strings() { \
        std::string s; \
        std::vector<std::string> result{}; \
        return result; \
    } \
    static constexpr std::string class_name() { return #ty; }

#define RPNX_MEMBER_METADATA_IMPL(ty, ...) \
    auto tie() const { return std::tie(__VA_ARGS__); } \
    auto tie() { return std::tie(__VA_ARGS__); } \
    auto serial_interface() const { return tie(); } \
    auto serial_interface() { return tie(); } \
    auto operator<=>(ty const& other) const { return rpnx::compare(tie(), other.tie()); } \
    bool operator==(ty const& other) const { return tie() == other.tie(); } \
    bool operator!=(ty const& other) const { return tie() != other.tie(); } \
    static auto constexpr strings() { \
        std::string str = #__VA_ARGS__; \
        std::string s; \
        std::vector<std::string> result{}; \
        for (char c : str) { \
            if (c == ',') { \
                result.push_back(std::move(s)); \
                s.clear(); \
            } else if (c != ' ') { \
                s.push_back(c); \
            } \
        } \
        if (!s.empty()) result.push_back(std::move(s)); \
        return result; \
    } \
    static constexpr std::string class_name() { return #ty; }

#define RPNX_MEMBER_METADATA(ty, ...) RPNX_EXPAND(RPNX_MEMBER_METADATA_IMPL(ty, __VA_ARGS__))


#define RPNX_ENUM(ns, ty, int_ty, ...) \
    namespace ns { \
       enum class ty : int_ty { __VA_ARGS__ }; \
    } \
    namespace rpnx { \
    template <> struct enum_traits< ns :: ty > \
    { \
        static auto constexpr strings()  { std::string str = #__VA_ARGS__; std::string s; std::vector<std::string> result{ }; \
        for (char c: str) { if (c == ',') { result.push_back(std::move(s)); s.clear(); } else if (c != ' ') { s.push_back(c); } } if (!s.empty()) result.push_back(std::move(s)); return result; } \
        static auto constexpr to_string(ns::ty const & v) { return strings().at(static_cast<int_ty>(v)); } \
        static auto constexpr from_string(std::string_view s) { for (int_ty i = 0; i < strings().size(); i++) { if (strings()[i] == s) return static_cast<ns::ty>(i); } throw std::runtime_error("invalid"); } \
    }; \
    }



#endif //METADATA_HPP
