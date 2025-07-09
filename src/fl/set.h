#pragma once

//#include <stddef.h>
#include "fl/stdint.h"

#include "fl/namespace.h"
#include "fl/vector.h"
#include "fl/map.h"
#include "fl/allocator.h"


namespace fl {

template <typename Key, typename Compare, typename Allocator> class set;

// VectorSet stores values in order of insertion.
template <typename Key, size N> class VectorSetFixed;
template <typename Key, typename Allocator> class VectorSet;

template <typename Key, size N>
using FixedSet = VectorSetFixed<Key, N>;  // Backwards compatibility

// A simple unordered set implementation with a fixed size.
// The user is responsible for making sure that the inserts
// do not exceed the capacity of the set, otherwise they will
// fail. Because of this limitation, this set is not a drop in
// replacement for std::set.
template <typename Key, size N> class VectorSetFixed {
  public:
    typedef FixedVector<Key, N> VectorType;
    typedef typename VectorType::iterator iterator;
    typedef typename VectorType::const_iterator const_iterator;

    // Constructor
    constexpr VectorSetFixed() = default;

    iterator begin() { return data.begin(); }
    iterator end() { return data.end(); }
    const_iterator begin() const { return data.begin(); }
    const_iterator end() const { return data.end(); }

    iterator find(const Key &key) {
        for (auto it = begin(); it != end(); ++it) {
            if (*it == key) {
                return it;
            }
        }
        return end();
    }

    const_iterator find(const Key &key) const {
        for (auto it = begin(); it != end(); ++it) {
            if (*it == key) {
                return it;
            }
        }
        return end();
    }

    bool insert(const Key &key) {
        if (data.size() < N) {
            auto it = find(key);
            if (it == end()) {
                data.push_back(key);
                return true;
            }
        }
        return false;
    }

    // Move version of insert
    bool insert(Key &&key) {
        if (data.size() < N) {
            auto it = find(key);
            if (it == end()) {
                data.push_back(fl::move(key));
                return true;
            }
        }
        return false;
    }

    // Emplace - construct in place with perfect forwarding
    template<typename... Args>
    bool emplace(Args&&... args) {
        if (data.size() < N) {
            // Create a temporary to check if it already exists
            Key temp_key(fl::forward<Args>(args)...);
            auto it = find(temp_key);
            if (it == end()) {
                data.push_back(fl::move(temp_key));
                return true;
            }
        }
        return false;
    }

    bool erase(const Key &key) {
        auto it = find(key);
        if (it != end()) {
            data.erase(it);
            return true;
        }
        return false;
    }

    bool erase(iterator pos) {
        if (pos != end()) {
            data.erase(pos);
            return true;
        }
        return false;
    }

    bool next(const Key &key, Key *next_key,
              bool allow_rollover = false) const {
        const_iterator it = find(key);
        if (it != end()) {
            ++it;
            if (it != end()) {
                *next_key = *it;
                return true;
            } else if (allow_rollover && !empty()) {
                *next_key = *begin();
                return true;
            }
        }
        return false;
    }

    bool prev(const Key &key, Key *prev_key,
              bool allow_rollover = false) const {
        const_iterator it = find(key);
        if (it != end()) {
            if (it != begin()) {
                --it;
                *prev_key = *it;
                return true;
            } else if (allow_rollover && !empty()) {
                *prev_key = data[data.size() - 1];
                return true;
            }
        }
        return false;
    }

    // Get the current size of the set
    constexpr fl::size size() const { return data.size(); }

    constexpr bool empty() const { return data.empty(); }

    // Get the capacity of the set
    constexpr fl::size capacity() const { return N; }

    // Clear the set
    void clear() { data.clear(); }

    bool has(const Key &key) const { return find(key) != end(); }

    // Return the first element of the set
    const Key &front() const { return data.front(); }

    // Return the last element of the set
    const Key &back() const { return data.back(); }

  private:
    VectorType data;
};

template <typename Key, typename Allocator = fl::allocator<Key>> class VectorSet {
  public:
    typedef fl::HeapVector<Key, Allocator> VectorType;
    typedef typename VectorType::iterator iterator;
    typedef typename VectorType::const_iterator const_iterator;

    // Constructor
    constexpr VectorSet() = default;

    iterator begin() { return data.begin(); }
    iterator end() { return data.end(); }
    const_iterator begin() const { return data.begin(); }
    const_iterator end() const { return data.end(); }

    iterator find(const Key &key) {
        for (auto it = begin(); it != end(); ++it) {
            if (*it == key) {
                return it;
            }
        }
        return end();
    }

    const_iterator find(const Key &key) const {
        for (auto it = begin(); it != end(); ++it) {
            if (*it == key) {
                return it;
            }
        }
        return end();
    }

    bool insert(const Key &key) {
        auto it = find(key);
        if (it == end()) {
            data.push_back(key);
            return true;
        }
        return false;
    }

    // Move version of insert
    bool insert(Key &&key) {
        auto it = find(key);
        if (it == end()) {
            data.push_back(fl::move(key));
            return true;
        }
        return false;
    }

    // Emplace - construct in place with perfect forwarding
    template<typename... Args>
    bool emplace(Args&&... args) {
        // Create a temporary to check if it already exists
        Key temp_key(fl::forward<Args>(args)...);
        auto it = find(temp_key);
        if (it == end()) {
            data.push_back(fl::move(temp_key));
            return true;
        }
        return false;
    }

    bool erase(const Key &key) {
        auto it = find(key);
        if (it != end()) {
            data.erase(it);
            return true;
        }
        return false;
    }

    bool erase(iterator pos) {
        if (pos != end()) {
            data.erase(pos);
            return true;
        }
        return false;
    }

    // Get the current size of the set
    constexpr fl::size size() const { return data.size(); }

    constexpr bool empty() const { return data.empty(); }

    // Get the capacity of the set
    constexpr fl::size capacity() const { return data.capacity(); }

    // Clear the set
    void clear() { data.clear(); }

    bool has(const Key &key) const { return find(key) != end(); }

    // Return the first element of the set
    const Key &front() const { return data.front(); }

    // Return the last element of the set
    const Key &back() const { return data.back(); }

  private:
    VectorType data;
};

// fl::set<T, Allocator> - Ordered set implementation using SetRedBlackTree
// This is an ordered set that keeps elements sorted, similar to std::set
template <typename Key, typename Compare = fl::DefaultLess<Key>, typename Allocator = fl::allocator_slab<char>> 
class set {
  private:
    using TreeType = fl::SetRedBlackTree<Key, Compare, Allocator>;
    TreeType tree_data;
    
  public:
    // Standard set typedefs
    using key_type = Key;
    using value_type = Key;
    using size_type = fl::size;
    using difference_type = ptrdiff_t;
    using key_compare = Compare;
    using value_compare = Compare;
    using reference = const Key&;
    using const_reference = const Key&;
    using pointer = const Key*;
    using const_pointer = const Key*;
    
    // Iterator types - we only provide const iterators since set elements are immutable
    using const_iterator = typename TreeType::const_iterator;
    using iterator = const_iterator;  // set only provides const iterators
    
    // Constructors
    set(const Compare& comp = Compare(), const Allocator& alloc = Allocator()) 
        : tree_data(comp, fl::IdentityKeyExtractor<Key>(), alloc) {}
    set(const set& other) = default;
    set(set&& other) = default;
    set& operator=(const set& other) = default;
    set& operator=(set&& other) = default;
    
    // Iterators
    const_iterator begin() const { return tree_data.begin(); }
    const_iterator end() const { return tree_data.end(); }
    const_iterator cbegin() const { return tree_data.cbegin(); }
    const_iterator cend() const { return tree_data.cend(); }
    
    // Capacity
    bool empty() const { return tree_data.empty(); }
    size_type size() const { return tree_data.size(); }
    size_type max_size() const { return tree_data.max_size(); }
    
    // Modifiers
    void clear() { tree_data.clear(); }
    
    fl::pair<const_iterator, bool> insert(const Key& key) {
        auto result = tree_data.insert(key);
        return fl::pair<const_iterator, bool>(result.first, result.second);
    }
    
    fl::pair<const_iterator, bool> insert(Key&& key) {
        auto result = tree_data.insert(fl::move(key));
        return fl::pair<const_iterator, bool>(result.first, result.second);
    }
    
    template<typename... Args>
    fl::pair<const_iterator, bool> emplace(Args&&... args) {
        auto result = tree_data.emplace(fl::forward<Args>(args)...);
        return fl::pair<const_iterator, bool>(result.first, result.second);
    }
    
    const_iterator erase(const_iterator pos) {
        return tree_data.erase(pos);
    }
    
    size_type erase(const Key& key) {
        return tree_data.erase(key);
    }
    
    void swap(set& other) {
        tree_data.swap(other.tree_data);
    }
    
    // Lookup
    size_type count(const Key& key) const {
        return tree_data.count(key);
    }
    
    const_iterator find(const Key& key) const {
        return tree_data.find(key);
    }
    
    bool contains(const Key& key) const {
        return tree_data.contains(key);
    }
    
    bool has(const Key& key) const {
        return contains(key);
    }
    
    fl::pair<const_iterator, const_iterator> equal_range(const Key& key) const {
        return tree_data.equal_range(key);
    }
    
    const_iterator lower_bound(const Key& key) const {
        return tree_data.lower_bound(key);
    }
    
    const_iterator upper_bound(const Key& key) const {
        return tree_data.upper_bound(key);
    }
    
    // Observers
    key_compare key_comp() const {
        return tree_data.key_comp();
    }
    
    value_compare value_comp() const {
        return key_comp();
    }
};

} // namespace fl
