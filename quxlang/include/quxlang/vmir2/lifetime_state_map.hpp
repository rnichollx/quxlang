// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_LIFETIME_STATE_MAP_HEADER_GUARD
#define QUXLANG_VMIR2_LIFETIME_STATE_MAP_HEADER_GUARD

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace quxlang::vmir2
{
    /**
     * Stores ordered lifetime snapshots in a compressed binary radix tree.
     * Copying a snapshot shares its root. Updating one key copies at most one
     * node for each bit of the 64-bit local index. Iterators retain an immutable
     * snapshot and remain valid when the source map changes. Mutable references
     * are valid until the next snapshot or mutation of the same key.
     */
    template < typename Key, typename Value >
    class lifetime_state_map
    {
      public:
        using key_type = Key;
        using mapped_type = Value;
        using value_type = std::pair< Key const, Value >;
        using size_type = std::size_t;

        /** One shared branch or one logical slot version in the snapshot DAG. */
        struct snapshot_node
        {
            std::shared_ptr< snapshot_node > left;
            std::shared_ptr< snapshot_node > right;
            std::shared_ptr< value_type > value;
            std::uint64_t branch_bit = 0;
            std::uint64_t minimum_key = 0;
            std::uint64_t maximum_key = 0;
            size_type count = 0;
        };

        using snapshot = std::shared_ptr< snapshot_node const >;

        /** Iterates over the immutable version captured when iteration begins. */
        class const_iterator
        {
          public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = lifetime_state_map::value_type;
            using difference_type = std::ptrdiff_t;
            using pointer = value_type const*;
            using reference = value_type const&;

            const_iterator() = default;

            reference operator*() const { return *current_->value; }
            pointer operator->() const { return current_->value.get(); }

            const_iterator& operator++()
            {
                seek_successor();
                return *this;
            }

            const_iterator operator++(std::int32_t)
            {
                const_iterator previous = *this;
                ++*this;
                return previous;
            }

            const_iterator& operator--()
            {
                seek_predecessor();
                return *this;
            }

            const_iterator operator--(std::int32_t)
            {
                const_iterator previous = *this;
                --*this;
                return previous;
            }

            friend bool operator==(const_iterator const& left, const_iterator const& right)
            {
                if (!left.current_ || !right.current_) return left.current_ == right.current_;
                return left.root_ == right.root_ && left.current_ == right.current_;
            }

          private:
            friend class lifetime_state_map;
            snapshot root_;
            snapshot_node const* current_ = nullptr;

            explicit const_iterator(snapshot root, snapshot_node const* current)
                : root_(std::move(root)), current_(current) {}

            void seek_successor()
            {
                std::uint64_t key = current_->minimum_key;
                snapshot_node const* candidate = nullptr;
                snapshot_node const* node = root_.get();
                while (node && !node->value)
                {
                    if (key < node->right->minimum_key)
                    {
                        candidate = node->right.get();
                        node = node->left.get();
                    }
                    else node = node->right.get();
                }
                while (candidate && !candidate->value) candidate = candidate->left.get();
                current_ = candidate;
            }

            void seek_predecessor()
            {
                if (!current_)
                {
                    current_ = root_.get();
                    while (current_ && !current_->value) current_ = current_->right.get();
                    return;
                }
                std::uint64_t key = current_->minimum_key;
                snapshot_node const* candidate = nullptr;
                snapshot_node const* node = root_.get();
                while (node && !node->value)
                {
                    if (key > node->left->maximum_key)
                    {
                        candidate = node->left.get();
                        node = node->right.get();
                    }
                    else node = node->left.get();
                }
                while (candidate && !candidate->value) candidate = candidate->right.get();
                current_ = candidate;
            }
        };

        using iterator = const_iterator;
        using const_reverse_iterator = std::reverse_iterator< const_iterator >;
        using reverse_iterator = const_reverse_iterator;

        lifetime_state_map() = default;

        lifetime_state_map(std::initializer_list< value_type > values)
        {
            for (value_type const& value : values) emplace(value.first, value.second);
        }

        /** Shares a snapshot without visiting its slots or delegate metadata. */
        lifetime_state_map(lifetime_state_map const&) = default;
        lifetime_state_map(lifetime_state_map&&) noexcept = default;
        lifetime_state_map& operator=(lifetime_state_map const&) = default;
        lifetime_state_map& operator=(lifetime_state_map&&) noexcept = default;

        /** Returns the root for shared-DAG analysis and serialization. */
        snapshot snapshot_root() const
        {
            restore_deserialized();
            return root_;
        }

        /** Publishes an already validated snapshot without enumerating its keys. */
        static lifetime_state_map from_snapshot(snapshot root)
        {
            lifetime_state_map result;
            result.root_ = std::const_pointer_cast< snapshot_node >(std::move(root));
            return result;
        }

        size_type size() const
        {
            restore_deserialized();
            return root_ ? root_->count : 0;
        }

        bool empty() const { return size() == 0; }

        void clear()
        {
            root_.reset();
            deserialized_.reset();
        }

        bool contains(Key key) const { return locate(key) != nullptr; }

        Value const& at(Key key) const
        {
            snapshot_node const* node = locate(key);
            if (!node) throw std::out_of_range("lifetime state key is absent");
            return node->value->second;
        }

        Value& at(Key key)
        {
            if (!contains(key)) throw std::out_of_range("lifetime state key is absent");
            return edit(root_, static_cast< std::uint64_t >(key));
        }

        Value& operator[](Key key)
        {
            restore_deserialized();
            if (!locate(key)) insert_leaf(root_, make_leaf(key, Value{}));
            return edit(root_, static_cast< std::uint64_t >(key));
        }

        template < typename... Args >
        std::pair< iterator, bool > emplace(Key key, Args&&... arguments)
        {
            restore_deserialized();
            if (contains(key)) return {find(key), false};
            insert_leaf(root_, make_leaf(key, Value(std::forward< Args >(arguments)...)));
            return {find(key), true};
        }

        std::pair< iterator, bool > insert(value_type const& value) { return emplace(value.first, value.second); }
        std::pair< iterator, bool > insert(value_type&& value) { return emplace(value.first, std::move(value.second)); }

        size_type erase(Key key)
        {
            restore_deserialized();
            if (!contains(key)) return 0;
            erase_leaf(root_, static_cast< std::uint64_t >(key));
            return 1;
        }

        const_iterator find(Key key) const
        {
            snapshot_node const* node = locate(key);
            return const_iterator(root_, node);
        }

        const_iterator begin() const
        {
            restore_deserialized();
            snapshot_node const* node = root_.get();
            while (node && !node->value) node = node->left.get();
            return const_iterator(root_, node);
        }

        const_iterator end() const
        {
            restore_deserialized();
            return const_iterator(root_, nullptr);
        }

        const_iterator cbegin() const { return begin(); }
        const_iterator cend() const { return end(); }
        const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
        const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
        const_reverse_iterator crbegin() const { return rbegin(); }
        const_reverse_iterator crend() const { return rend(); }

        /** Provides ordinary map serialization for standalone diagnostic values. */
        std::map< Key, Value > serialize_interface() const
        {
            return std::map< Key, Value >(begin(), end());
        }

        /** Accepts serialized entries and materializes their shared tree on first access. */
        std::map< Key, Value >& deserialize_interface()
        {
            clear();
            deserialized_ = std::make_shared< std::map< Key, Value > >();
            return *deserialized_;
        }

        /** Exposes logical entries to JSON diagnostics and serialization type hashing. */
        auto tie() const { return std::tuple< std::map< Key, Value > >(serialize_interface()); }
        static constexpr auto strings() { return std::array{std::string("entries")}; }
        static constexpr std::string class_name() { return "lifetime_state_map"; }

        friend bool operator==(lifetime_state_map const& left, lifetime_state_map const& right)
        {
            if (left.snapshot_root() == right.snapshot_root()) return true;
            if (left.size() != right.size()) return false;
            return std::equal(left.begin(), left.end(), right.begin());
        }

        friend bool operator<(lifetime_state_map const& left, lifetime_state_map const& right)
        {
            if (left.snapshot_root() == right.snapshot_root()) return false;
            return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
        }

        friend bool operator>(lifetime_state_map const& left, lifetime_state_map const& right) { return right < left; }
        friend bool operator<=(lifetime_state_map const& left, lifetime_state_map const& right) { return !(right < left); }
        friend bool operator>=(lifetime_state_map const& left, lifetime_state_map const& right) { return !(left < right); }

      private:
        mutable std::shared_ptr< snapshot_node > root_;
        mutable std::shared_ptr< std::map< Key, Value > > deserialized_;

        static std::shared_ptr< snapshot_node > make_leaf(Key key, Value value)
        {
            std::shared_ptr< snapshot_node > node = std::make_shared< snapshot_node >();
            node->value = std::make_shared< value_type >(key, std::move(value));
            node->minimum_key = static_cast< std::uint64_t >(key);
            node->maximum_key = node->minimum_key;
            node->count = 1;
            return node;
        }

        static void detach(std::shared_ptr< snapshot_node >& node)
        {
            if (node.use_count() != 1) node = std::make_shared< snapshot_node >(*node);
        }

        static void refresh(snapshot_node& node)
        {
            node.minimum_key = node.left->minimum_key;
            node.maximum_key = node.right->maximum_key;
            node.count = node.left->count + node.right->count;
        }

        static void insert_leaf(std::shared_ptr< snapshot_node >& node, std::shared_ptr< snapshot_node > leaf)
        {
            if (!node)
            {
                node = std::move(leaf);
                return;
            }
            std::uint64_t difference = node->minimum_key ^ leaf->minimum_key;
            std::uint64_t bit = std::uint64_t{1} << (63 - std::countl_zero(difference));
            if (node->value || node->branch_bit < bit)
            {
                std::shared_ptr< snapshot_node > branch = std::make_shared< snapshot_node >();
                branch->branch_bit = bit;
                if ((leaf->minimum_key & bit) == 0)
                {
                    branch->left = std::move(leaf);
                    branch->right = std::move(node);
                }
                else
                {
                    branch->left = std::move(node);
                    branch->right = std::move(leaf);
                }
                refresh(*branch);
                node = std::move(branch);
                return;
            }
            detach(node);
            if ((leaf->minimum_key & node->branch_bit) == 0) insert_leaf(node->left, std::move(leaf));
            else insert_leaf(node->right, std::move(leaf));
            refresh(*node);
        }

        static Value& edit(std::shared_ptr< snapshot_node >& node, std::uint64_t key)
        {
            detach(node);
            if (node->value)
            {
                if (node->value.use_count() != 1) node->value = std::make_shared< value_type >(*node->value);
                return node->value->second;
            }
            return (key & node->branch_bit) == 0 ? edit(node->left, key) : edit(node->right, key);
        }

        static void erase_leaf(std::shared_ptr< snapshot_node >& node, std::uint64_t key)
        {
            if (node->value)
            {
                node.reset();
                return;
            }
            detach(node);
            if ((key & node->branch_bit) == 0) erase_leaf(node->left, key);
            else erase_leaf(node->right, key);
            if (!node->left) node = std::move(node->right);
            else if (!node->right) node = std::move(node->left);
            else refresh(*node);
        }

        snapshot_node const* locate(Key key) const
        {
            restore_deserialized();
            std::uint64_t index = static_cast< std::uint64_t >(key);
            snapshot_node const* node = root_.get();
            while (node && !node->value) node = (index & node->branch_bit) == 0 ? node->left.get() : node->right.get();
            return node && node->minimum_key == index ? node : nullptr;
        }

        void restore_deserialized() const
        {
            if (!deserialized_) return;
            for (std::pair< Key const, Value > const& value : *deserialized_) insert_leaf(root_, make_leaf(value.first, value.second));
            deserialized_.reset();
        }
    };

    /** Shares immutable delegate metadata between versions of one logical slot. */
    template < typename Value >
    class shared_lifetime_optional
    {
      public:
        shared_lifetime_optional() = default;
        shared_lifetime_optional(std::nullopt_t) {}
        shared_lifetime_optional(Value value) : value_(std::make_shared< std::optional< Value > >(std::move(value))) {}
        shared_lifetime_optional(std::optional< Value > value)
        {
            if (value.has_value()) value_ = std::make_shared< std::optional< Value > >(std::move(value));
        }

        bool has_value() const { return value_ && value_->has_value(); }
        explicit operator bool() const { return has_value(); }
        Value const& operator*() const { return value(); }
        Value const* operator->() const { return &value(); }
        Value const& value() const
        {
            if (!has_value()) throw std::bad_optional_access();
            return **value_;
        }

        Value value_or(Value fallback) const { return has_value() ? value() : std::move(fallback); }
        void reset() { value_.reset(); }

        /** Explicitly edits delegate metadata while preserving previously published snapshots. */
        Value& edit()
        {
            if (!has_value()) throw std::bad_optional_access();
            if (value_.use_count() != 1) value_ = std::make_shared< std::optional< Value > >(*value_);
            return **value_;
        }

        std::optional< Value > const& serialize_interface() const
        {
            static std::optional< Value > const empty;
            return value_ ? *value_ : empty;
        }

        std::optional< Value >& deserialize_interface()
        {
            value_ = std::make_shared< std::optional< Value > >();
            return *value_;
        }

        /** Exposes the shared payload's logical value to serialization metadata. */
        auto tie() const { return std::tie(serialize_interface()); }
        static constexpr auto strings() { return std::array{std::string("value")}; }
        static constexpr std::string class_name() { return "shared_lifetime_optional"; }

        operator std::optional< Value >() const { return serialize_interface(); }

        friend bool operator==(shared_lifetime_optional const& left, shared_lifetime_optional const& right)
        {
            return left.value_ == right.value_ || left.serialize_interface() == right.serialize_interface();
        }
        friend bool operator<(shared_lifetime_optional const& left, shared_lifetime_optional const& right)
        {
            return left.value_ != right.value_ && left.serialize_interface() < right.serialize_interface();
        }
        friend bool operator>(shared_lifetime_optional const& left, shared_lifetime_optional const& right) { return right < left; }
        friend bool operator<=(shared_lifetime_optional const& left, shared_lifetime_optional const& right) { return !(right < left); }
        friend bool operator>=(shared_lifetime_optional const& left, shared_lifetime_optional const& right) { return !(left < right); }

      private:
        std::shared_ptr< std::optional< Value > > value_;
    };

    /** Shares ordered array-element ownership sets between lifetime snapshots. */
    template < typename Key >
    class lifetime_index_set
    {
        using map_type = lifetime_state_map< Key, bool >;

      public:
        using value_type = Key;

        /** Iterates over local indices without exposing the map's occupancy values. */
        class const_iterator
        {
          public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = Key;
            using difference_type = std::ptrdiff_t;
            using pointer = Key const*;
            using reference = Key const&;
            const_iterator() = default;
            explicit const_iterator(typename map_type::const_iterator value) : value_(std::move(value)) {}
            reference operator*() const { return value_->first; }
            pointer operator->() const { return &value_->first; }
            const_iterator& operator++() { ++value_; return *this; }
            const_iterator operator++(std::int32_t) { const_iterator copy = *this; ++*this; return copy; }
            const_iterator& operator--() { --value_; return *this; }
            const_iterator operator--(std::int32_t) { const_iterator copy = *this; --*this; return copy; }
            friend bool operator==(const_iterator const&, const_iterator const&) = default;
          private:
            typename map_type::const_iterator value_;
        };

        void insert(Key key) { values_.emplace(key, true); }
        void erase(Key key) { values_.erase(key); }
        void clear() { values_.clear(); }
        bool contains(Key key) const { return values_.contains(key); }
        bool empty() const { return values_.empty(); }
        std::size_t size() const { return values_.size(); }
        const_iterator begin() const { return const_iterator(values_.begin()); }
        const_iterator end() const { return const_iterator(values_.end()); }
        map_type const& serialize_interface() const { return values_; }
        map_type& deserialize_interface() { return values_; }
        /** Exposes logical membership to serialization metadata. */
        auto tie() const { return std::tie(values_); }
        static constexpr auto strings() { return std::array{std::string("values")}; }
        static constexpr std::string class_name() { return "lifetime_index_set"; }
        friend bool operator==(lifetime_index_set const&, lifetime_index_set const&) = default;
        friend bool operator<(lifetime_index_set const& left, lifetime_index_set const& right) { return left.values_ < right.values_; }
        friend bool operator>(lifetime_index_set const& left, lifetime_index_set const& right) { return right < left; }
        friend bool operator<=(lifetime_index_set const& left, lifetime_index_set const& right) { return !(right < left); }
        friend bool operator>=(lifetime_index_set const& left, lifetime_index_set const& right) { return !(left < right); }

      private:
        map_type values_;
    };
}

#endif // QUXLANG_VMIR2_LIFETIME_STATE_MAP_HEADER_GUARD
