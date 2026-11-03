// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_LIFETIME_SNAPSHOT_ARCHIVE_HEADER_GUARD
#define QUXLANG_VMIR2_LIFETIME_SNAPSHOT_ARCHIVE_HEADER_GUARD

#include <quxlang/vmir2/lifetime_snapshot_graph.hpp>

#include <array>
#include <bit>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace quxlang::vmir2
{
    /** Associates a logical key with one archived leaf payload. */
    template < typename Key, typename Value >
    struct lifetime_snapshot_value
    {
        Key key;
        Value value;

        auto tie() const { return std::tie(key, value); }
        auto tie() { return std::tie(key, value); }
        auto serialize_interface() const { return tie(); }
        auto deserialize_interface() { return tie(); }
        static constexpr auto strings() { return std::array{std::string("key"), std::string("value")}; }
        static constexpr std::string class_name() { return "lifetime_snapshot_value"; }
        friend bool operator==(lifetime_snapshot_value const&, lifetime_snapshot_value const&) = default;
        friend bool operator<(lifetime_snapshot_value const& left, lifetime_snapshot_value const& right) { return left.tie() < right.tie(); }
    };

    /** Encodes one shared branch or one logical slot version using postorder indices. */
    template < typename Key, typename Value >
    struct lifetime_snapshot_record
    {
        std::optional< std::size_t > left;
        std::optional< std::size_t > right;
        std::optional< lifetime_snapshot_value< Key, Value > > value;

        auto tie() const { return std::tie(left, right, value); }
        auto tie() { return std::tie(left, right, value); }
        auto serialize_interface() const { return tie(); }
        auto deserialize_interface() { return tie(); }
        static constexpr auto strings() { return std::array{std::string("left"), std::string("right"), std::string("value")}; }
        static constexpr std::string class_name() { return "lifetime_snapshot_record"; }
        friend bool operator==(lifetime_snapshot_record const&, lifetime_snapshot_record const&) = default;
        friend bool operator<(lifetime_snapshot_record const& left, lifetime_snapshot_record const& right) { return left.tie() < right.tie(); }
    };

    /** Serializes each represented immutable node once, retaining shared root indices. */
    template < typename Map, typename Value, typename Callback >
    auto archive_lifetime_snapshots(lifetime_snapshot_graph< Map > const& graph, Callback const& callback)
        -> std::vector< lifetime_snapshot_record< typename Map::key_type, Value > >
    {
        using record = lifetime_snapshot_record< typename Map::key_type, Value >;
        std::vector< record > result;
        result.reserve(graph.nodes().size());
        for (typename lifetime_snapshot_graph< Map >::node const& node : graph.nodes())
        {
            record encoded{.left = node.left, .right = node.right};
            if (node.source->value)
            {
                encoded.value = lifetime_snapshot_value< typename Map::key_type, Value >{
                    .key = node.source->value->first,
                    .value = callback(node.source->value->second),
                };
            }
            result.push_back(std::move(encoded));
        }
        return result;
    }

    /** Validates a serialized radix DAG and reconstructs its sharing in postorder. */
    template < typename Map, typename Value, typename Callback >
    auto restore_lifetime_snapshots(std::vector< lifetime_snapshot_record< typename Map::key_type, Value > > const& records, Callback const& callback)
        -> std::vector< typename Map::snapshot >
    {
        using node = typename Map::snapshot_node;
        std::vector< typename Map::snapshot > result;
        result.reserve(records.size());
        for (lifetime_snapshot_record< typename Map::key_type, Value > const& record : records)
        {
            std::shared_ptr< node > decoded = std::make_shared< node >();
            if (record.value.has_value())
            {
                if (record.left.has_value() || record.right.has_value())
                {
                    throw std::invalid_argument("A serialized lifetime leaf cannot contain child indices");
                }
                decoded->value = std::make_shared< typename Map::value_type >(record.value->key, callback(record.value->value));
                decoded->minimum_key = static_cast< std::uint64_t >(record.value->key);
                decoded->maximum_key = decoded->minimum_key;
                decoded->count = 1;
            }
            else
            {
                if (!record.left.has_value() || !record.right.has_value() || *record.left >= result.size() || *record.right >= result.size())
                {
                    throw std::invalid_argument("A serialized lifetime branch requires two preceding child indices");
                }
                decoded->left = std::const_pointer_cast< node >(result[*record.left]);
                decoded->right = std::const_pointer_cast< node >(result[*record.right]);
                if (decoded->left->maximum_key >= decoded->right->minimum_key)
                {
                    throw std::invalid_argument("Serialized lifetime child key ranges must be disjoint and ordered");
                }
                decoded->minimum_key = decoded->left->minimum_key;
                decoded->maximum_key = decoded->right->maximum_key;
                std::uint64_t difference = decoded->minimum_key ^ decoded->maximum_key;
                decoded->branch_bit = std::uint64_t{1} << (63 - std::countl_zero(difference));
                if (decoded->left->branch_bit >= decoded->branch_bit || decoded->right->branch_bit >= decoded->branch_bit ||
                    (decoded->left->maximum_key & decoded->branch_bit) != 0 || (decoded->right->minimum_key & decoded->branch_bit) == 0)
                {
                    throw std::invalid_argument("Serialized lifetime children violate their radix partition");
                }
                if (decoded->left->count > std::numeric_limits< std::size_t >::max() - decoded->right->count)
                {
                    throw std::overflow_error("Serialized lifetime snapshot size exceeds the host representation");
                }
                decoded->count = decoded->left->count + decoded->right->count;
            }
            result.push_back(std::move(decoded));
        }
        return result;
    }
}

#endif // QUXLANG_VMIR2_LIFETIME_SNAPSHOT_ARCHIVE_HEADER_GUARD
