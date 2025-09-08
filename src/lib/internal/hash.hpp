#pragma once
#include <unordered_map>
#include <cstddef>
#include "ds_common.hpp"   // ds::Handle<Key>

template <class Key>
class Hash {
public:
    using Handle = ds::Handle<Key>;

    explicit Hash(std::size_t expected_size = 0) {
        if (expected_size) map_.reserve(expected_size);
    }

    std::size_t size()  const noexcept { return map_.size(); }
    bool        empty() const noexcept { return map_.empty(); }
    void        clear()        noexcept { map_.clear(); }

   
    bool contains(const Key& k) const {
        return map_.find(k) != map_.end();
    }

    Handle* get(const Key& k) {
        auto it = map_.find(k);
        return (it == map_.end()) ? nullptr : &it->second;
    }

    const Handle* get(const Key& k) const {
        auto it = map_.find(k);
        return (it == map_.end()) ? nullptr : &it->second;
    }

    bool insert(const Key& k, const Handle& h) {
        auto [it, inserted] = map_.emplace(k, h);
        return inserted;
    }
    bool insert(Key&& k, Handle&& h) {
        auto [it, inserted] = map_.emplace(std::move(k), std::move(h));
        return inserted;
    }

    void upsert(const Key& k, const Handle& h) {
        map_[k] = h;
    }
    void upsert(Key&& k, Handle&& h) {
        map_[std::move(k)] = std::move(h);
    }

    
    bool erase(const Key& k) {
        return map_.erase(k) > 0;
    }

private:
    std::unordered_map<Key, Handle> map_;
};
