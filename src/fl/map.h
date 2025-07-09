#pragma once

//#include <stddef.h>
#include "fl/stdint.h"

#include "fl/assert.h"
#include "fl/comparators.h"
#include "fl/insert_result.h"
#include "fl/namespace.h"
#include "fl/pair.h"
#include "fl/template_magic.h"
#include "fl/type_traits.h"
#include "fl/vector.h"
#include "fl/rbtree.h"
#include "fl/allocator.h"

namespace fl {

// A simple unordered map implementation with a fixed size.
// The user is responsible for making sure that the inserts
// do not exceed the capacity of the set, otherwise they will
// fail. Because of this limitation, this set is not a drop in
// replacement for std::map.
template <typename Key, typename Value, fl::size N> class FixedMap {
  public:
    using PairKV = fl::pair<Key, Value>;

    typedef FixedVector<PairKV, N> VectorType;
    typedef typename VectorType::iterator iterator;
    typedef typename VectorType::const_iterator const_iterator;

    // Constructor
    constexpr FixedMap() = default;

    iterator begin() { return data.begin(); }
    iterator end() { return data.end(); }
    const_iterator begin() const { return data.begin(); }
    const_iterator end() const { return data.end(); }

    iterator find(const Key &key) {
        for (auto it = begin(); it != end(); ++it) {
            if (it->first == key) {
                return it;
            }
        }
        return end();
    }

    const_iterator find(const Key &key) const {
        for (auto it = begin(); it != end(); ++it) {
            if (it->first == key) {
                return it;
            }
        }
        return end();
    }

    template <typename Less> iterator lowest(Less less_than = Less()) {
        iterator lowest = end();
        for (iterator it = begin(); it != end(); ++it) {
            if (lowest == end() || less_than(it->first, lowest->first)) {
                lowest = it;
            }
        }
        return lowest;
    }

    template <typename Less>
    const_iterator lowest(Less less_than = Less()) const {
        const_iterator lowest = end();
        for (const_iterator it = begin(); it != end(); ++it) {
            if (lowest == end() || less_than(it->first, lowest->first)) {
                lowest = it;
            }
        }
        return lowest;
    }

    template <typename Less> iterator highest(Less less_than = Less()) {
        iterator highest = end();
        for (iterator it = begin(); it != end(); ++it) {
            if (highest == end() || less_than(highest->first, it->first)) {
                highest = it;
            }
        }
        return highest;
    }

    template <typename Less>
    const_iterator highest(Less less_than = Less()) const {
        const_iterator highest = end();
        for (const_iterator it = begin(); it != end(); ++it) {
            if (highest == end() || less_than(highest->first, it->first)) {
                highest = it;
            }
        }
        return highest;
    }

    // We differ from the std standard here so that we don't allow
    // dereferencing the end iterator.
    bool get(const Key &key, Value *value) const {
        const_iterator it = find(key);
        if (it != end()) {
            *value = it->second;
            return true;
        }
        return false;
    }

    Value get(const Key &key, bool *has = nullptr) const {
        const_iterator it = find(key);
        if (it != end()) {
            if (has) {
                *has = true;
            }
            return it->second;
        }
        if (has) {
            *has = false;
        }
        return Value();
    }

    Pair<bool, iterator> insert(const Key &key, const Value &value,
                                InsertResult *result = nullptr) {
        iterator it = find(key);
        if (it != end()) {
            if (result) {
                *result = InsertResult::kExists;
            }
            // return false;
            return {false, it};
        }
        if (data.size() < N) {
            data.push_back(PairKV(key, value));
            if (result) {
                *result = InsertResult::kInserted;
            }
            // return true;
            return {true, data.end() - 1};
        }
        if (result) {
            *result = InsertResult::kMaxSize;
        }
        // return false;
        return {false, end()};
    }

    // Move version of insert
    Pair<bool, iterator> insert(Key &&key, Value &&value,
                                InsertResult *result = nullptr) {
        iterator it = find(key);
        if (it != end()) {
            if (result) {
                *result = InsertResult::kExists;
            }
            return {false, it};
        }
        if (data.size() < N) {
            data.push_back(PairKV(fl::move(key), fl::move(value)));
            if (result) {
                *result = InsertResult::kInserted;
            }
            return {true, data.end() - 1};
        }
        if (result) {
            *result = InsertResult::kMaxSize;
        }
        return {false, end()};
    }

    bool update(const Key &key, const Value &value,
                bool insert_if_missing = true) {
        iterator it = find(key);
        if (it != end()) {
            it->second = value;
            return true;
        } else if (insert_if_missing) {
            return insert(key, value).first;
        }
        return false;
    }

    // Move version of update
    bool update(const Key &key, Value &&value,
                bool insert_if_missing = true) {
        iterator it = find(key);
        if (it != end()) {
            it->second = fl::move(value);
            return true;
        } else if (insert_if_missing) {
            return insert(key, fl::move(value)).first;
        }
        return false;
    }

    Value &operator[](const Key &key) {
        iterator it = find(key);
        if (it != end()) {
            return it->second;
        }
        data.push_back(PairKV(key, Value()));
        return data.back().second;
    }

    const Value &operator[](const Key &key) const {
        const_iterator it = find(key);
        if (it != end()) {
            return it->second;
        }
        static Value default_value;
        return default_value;
    }

    bool next(const Key &key, Key *next_key,
              bool allow_rollover = false) const {
        const_iterator it = find(key);
        if (it != end()) {
            ++it;
            if (it != end()) {
                *next_key = it->first;
                return true;
            } else if (allow_rollover && !empty()) {
                *next_key = begin()->first;
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
                *prev_key = it->first;
                return true;
            } else if (allow_rollover && !empty()) {
                *prev_key = data[data.size() - 1].first;
                return true;
            }
        }
        return false;
    }

    // Get the current size of the vector
    constexpr fl::size size() const { return data.size(); }

    constexpr bool empty() const { return data.empty(); }

    // Get the capacity of the vector
    constexpr fl::size capacity() const { return N; }

    // Clear the vector
    void clear() { data.clear(); }

    bool has(const Key &it) const { return find(it) != end(); }

    bool contains(const Key &key) const { return has(key); }

  private:
    VectorType data;
};

// Closest data structure to an std::map. Always sorted.
// O(n + log(n)) for insertions, O(log(n)) for searches, O(n) for iteration.
template <typename Key, typename Value, typename Less = fl::DefaultLess<Key>>
class SortedHeapMap {
  public:
    // Standard typedefs to match std::map interface
    using key_type = Key;
    using mapped_type = Value;
    using value_type = fl::pair<Key, Value>;
    using size_type = fl::size;
    using difference_type = ptrdiff_t;
    using key_compare = Less;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;

  private:
    struct PairLess {
        Less less;
        bool operator()(const value_type &a, const value_type &b) const {
            return less(a.first, b.first);
        }
    };

    SortedHeapVector<value_type, PairLess> data;

    // Value comparison class for std::map compatibility
    class value_compare {
        friend class SortedHeapMap;
        Less comp_;
        value_compare(Less c) : comp_(c) {}
    public:
        bool operator()(const value_type& x, const value_type& y) const {
            return comp_(x.first, y.first);
        }
    };

  public:
    typedef typename SortedHeapVector<value_type, PairLess>::iterator iterator;
    typedef typename SortedHeapVector<value_type, PairLess>::const_iterator const_iterator;

    // Constructors
    SortedHeapMap(Less less = Less()) : data(PairLess{less}) {}
    SortedHeapMap(const SortedHeapMap& other) = default;
    SortedHeapMap& operator=(const SortedHeapMap& other) = default;

    // Iterators
    iterator begin() { return data.begin(); }
    iterator end() { return data.end(); }
    const_iterator begin() const { return data.begin(); }
    const_iterator end() const { return data.end(); }
    const_iterator cbegin() const { return data.begin(); }
    const_iterator cend() const { return data.end(); }

    // Capacity
    fl::size size() const { return data.size(); }
    bool empty() const { return data.empty(); }
    bool full() const { return data.full(); }
    fl::size capacity() const { return data.capacity(); }
    fl::size max_size() const { return data.capacity(); }

    // FastLED specific methods
    void setMaxSize(fl::size n) { data.setMaxSize(n); }
    void reserve(fl::size n) { data.reserve(n); }

    // Element access
    Value &operator[](const Key &key) {
        iterator it = find(key);
        if (it != end()) {
            return it->second;
        }
        value_type pair(key, Value());
        bool ok = data.insert(pair);
        FASTLED_ASSERT(ok, "Failed to insert into SortedHeapMap");
        return data.find(pair)->second; // TODO: optimize.
    }

    Value &at(const Key &key) {
        iterator it = find(key);
        FASTLED_ASSERT(it != end(), "SortedHeapMap::at: key not found");
        return it->second;
    }

    const Value &at(const Key &key) const {
        const_iterator it = find(key);
        FASTLED_ASSERT(it != end(), "SortedHeapMap::at: key not found");
        return it->second;
    }

    // Modifiers
    void clear() { data.clear(); }

    fl::pair<iterator, bool> insert(const value_type& value) {
        InsertResult result;
        bool success = data.insert(value, &result);
        iterator it = success ? data.find(value) : data.end();
        return fl::pair<iterator, bool>(it, success);
    }

    // Move version of insert
    fl::pair<iterator, bool> insert(value_type&& value) {
        InsertResult result;
        bool success = data.insert(fl::move(value), &result);
        iterator it = success ? data.find(value) : data.end();
        return fl::pair<iterator, bool>(it, success);
    }

    bool insert(const Key &key, const Value &value, InsertResult *result = nullptr) {
        return data.insert(value_type(key, value), result);
    }

    // Move version of insert with key and value
    bool insert(Key &&key, Value &&value, InsertResult *result = nullptr) {
        return data.insert(value_type(fl::move(key), fl::move(value)), result);
    }

    template<class... Args>
    fl::pair<iterator, bool> emplace(Args&&... args) {
        value_type pair(fl::forward<Args>(args)...);
        InsertResult result;
        bool success = data.insert(pair, &result);
        iterator it = success ? data.find(pair) : data.end();
        return fl::pair<iterator, bool>(it, success);
    }

    iterator erase(const_iterator pos) {
        Key key = pos->first;
        data.erase(*pos);
        return upper_bound(key);
    }

    fl::size erase(const Key& key) {
        return data.erase(value_type(key, Value())) ? 1 : 0;
    }

    bool erase(iterator it) { 
        return data.erase(it); 
    }

    void swap(SortedHeapMap &other) { 
        data.swap(other.data); 
    }

    // Lookup
    fl::size count(const Key& key) const {
        return contains(key) ? 1 : 0;
    }

    iterator find(const Key &key) { 
        return data.find(value_type(key, Value())); 
    }

    const_iterator find(const Key &key) const { 
        return data.find(value_type(key, Value())); 
    }

    bool contains(const Key &key) const { 
        return has(key); 
    }

    bool has(const Key &key) const { 
        return data.has(value_type(key, Value())); 
    }

    fl::pair<iterator, iterator> equal_range(const Key& key) {
        iterator lower = lower_bound(key);
        iterator upper = upper_bound(key);
        return fl::pair<iterator, iterator>(lower, upper);
    }

    fl::pair<const_iterator, const_iterator> equal_range(const Key& key) const {
        const_iterator lower = lower_bound(key);
        const_iterator upper = upper_bound(key);
        return fl::pair<const_iterator, const_iterator>(lower, upper);
    }

    iterator lower_bound(const Key &key) { 
        return data.lower_bound(value_type(key, Value())); 
    }

    const_iterator lower_bound(const Key &key) const {
        return data.lower_bound(value_type(key, Value()));
    }

    iterator upper_bound(const Key &key) {
        iterator it = lower_bound(key);
        if (it != end() && it->first == key) {
            ++it;
        }
        return it;
    }

    const_iterator upper_bound(const Key &key) const {
        const_iterator it = lower_bound(key);
        if (it != end() && it->first == key) {
            ++it;
        }
        return it;
    }

    // Observers
    key_compare key_comp() const {
        return key_compare();
    }

    value_compare value_comp() const {
        return value_compare(key_comp());
    }

    // Additional methods
    value_type &front() { return data.front(); }
    const value_type &front() const { return data.front(); }
    value_type &back() { return data.back(); }
    const value_type &back() const { return data.back(); }

    void update(const Key &key, const Value &value) {
        if (!insert(key, value)) {
            iterator it = find(key);
            it->second = value;
        }
    }

    // Move version of update
    void update(const Key &key, Value &&value) {
        if (!insert(key, fl::move(value))) {
            iterator it = find(key);
            it->second = fl::move(value);
        }
    }

    // Matches FixedMap<>::get(...).
    bool get(const Key &key, Value *value) const {
        const_iterator it = find(key);
        if (it != end()) {
            *value = it->second;
            return true;
        }
        return false;
    }

    // Comparison operators
    bool operator==(const SortedHeapMap &other) const {
        if (size() != other.size()) {
            return false;
        }
        for (const_iterator it = begin(), other_it = other.begin(); it != end();
             ++it, ++other_it) {
            if (it->first != other_it->first ||
                it->second != other_it->second) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const SortedHeapMap &other) const {
        return !(*this == other);
    }
};

// fl::map<Key, Value, Compare, Allocator> - Ordered map implementation using MapRedBlackTree
// This is an ordered map that keeps elements sorted by key, similar to std::map
template <typename Key, typename Value, typename Compare = fl::DefaultLess<Key>, typename Allocator = fl::allocator_slab<char>>
class map {
  private:
    using TreeType = fl::MapRedBlackTree<Key, Value, Compare, Allocator>;
    TreeType tree_data;
    
  public:
    // Standard map typedefs
    using key_type = Key;
    using mapped_type = Value;
    using value_type = fl::pair<Key, Value>;
    using size_type = fl::size;
    using difference_type = ptrdiff_t;
    using key_compare = Compare;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    
    // Iterator types
    using iterator = typename TreeType::iterator;
    using const_iterator = typename TreeType::const_iterator;
    
    // Value comparison class for std::map compatibility
    class value_compare {
        friend class map;
        Compare comp_;
        value_compare(Compare c) : comp_(c) {}
    public:
        bool operator()(const value_type& x, const value_type& y) const {
            return comp_(x.first, y.first);
        }
    };
    
    // Constructors
    map(const Compare& comp = Compare(), const Allocator& alloc = Allocator()) 
        : tree_data(comp, fl::PairKeyExtractor<value_type>(), alloc) {}
    map(const map& other) = default;
    map(map&& other) = default;
    map& operator=(const map& other) = default;
    map& operator=(map&& other) = default;
    
    // Iterators
    iterator begin() { return tree_data.begin(); }
    iterator end() { return tree_data.end(); }
    const_iterator begin() const { return tree_data.begin(); }
    const_iterator end() const { return tree_data.end(); }
    const_iterator cbegin() const { return tree_data.cbegin(); }
    const_iterator cend() const { return tree_data.cend(); }
    
    // Capacity
    bool empty() const { return tree_data.empty(); }
    size_type size() const { return tree_data.size(); }
    size_type max_size() const { return tree_data.max_size(); }
    
    // Element access
    Value& operator[](const Key& key) {
        auto it = tree_data.find(key);
        if (it != tree_data.end()) {
            return it->second;
        }
        
        auto result = tree_data.insert(value_type(key, Value()));
        return result.first->second;
    }
    
    Value& at(const Key& key) {
        auto it = tree_data.find(key);
        FASTLED_ASSERT(it != tree_data.end(), "map::at: key not found");
        return it->second;
    }
    
    const Value& at(const Key& key) const {
        auto it = tree_data.find(key);
        FASTLED_ASSERT(it != tree_data.end(), "map::at: key not found");
        return it->second;
    }
    
    // Modifiers
    void clear() { tree_data.clear(); }
    
    fl::pair<iterator, bool> insert(const value_type& value) {
        return tree_data.insert(value);
    }
    
    fl::pair<iterator, bool> insert(value_type&& value) {
        return tree_data.insert(fl::move(value));
    }
    
    template<class... Args>
    fl::pair<iterator, bool> emplace(Args&&... args) {
        return tree_data.emplace(fl::forward<Args>(args)...);
    }
    
    iterator erase(const_iterator pos) {
        return tree_data.erase(pos);
    }
    
    size_type erase(const Key& key) {
        return tree_data.erase(key);
    }
    
    void swap(map& other) {
        tree_data.swap(other.tree_data);
    }
    
    // Lookup
    size_type count(const Key& key) const {
        return tree_data.count(key);
    }
    
    iterator find(const Key& key) {
        return tree_data.find(key);
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
    
    fl::pair<iterator, iterator> equal_range(const Key& key) {
        return tree_data.equal_range(key);
    }
    
    fl::pair<const_iterator, const_iterator> equal_range(const Key& key) const {
        return tree_data.equal_range(key);
    }
    
    iterator lower_bound(const Key& key) {
        return tree_data.lower_bound(key);
    }
    
    const_iterator lower_bound(const Key& key) const {
        return tree_data.lower_bound(key);
    }
    
    iterator upper_bound(const Key& key) {
        return tree_data.upper_bound(key);
    }
    
    const_iterator upper_bound(const Key& key) const {
        return tree_data.upper_bound(key);
    }
    
    // Observers
    key_compare key_comp() const {
        return tree_data.key_comp();
    }
    
    value_compare value_comp() const {
        return value_compare(key_comp());
    }
    
    // Comparison operators
    bool operator==(const map& other) const {
        return tree_data == other.tree_data;
    }
    
    bool operator!=(const map& other) const {
        return tree_data != other.tree_data;
    }
};

} // namespace fl

// Drop-in replacement for std::map
// Note: We use "fl_map" instead of "map" because Arduino.h defines a map() function
// which would conflict with fl::map usage in sketches that include Arduino.h and
// are using `using namespace fl`
namespace fl {

// Default map uses slab allocator for better performance
// Can't use fl::map because it conflicts with Arduino.h's map() function when
// the user is using `using namespace fl`
template <typename Key, typename T, typename Compare = fl::DefaultLess<Key>>
using fl_map = fl::map<Key, T, Compare, fl::allocator_slab<char>>;


} // namespace fl
