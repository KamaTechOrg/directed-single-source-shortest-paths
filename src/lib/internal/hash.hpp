// hash.hpp
#pragma once
#include <unordered_map>
#include <utility>
#include "ds_common.hpp"

namespace ds {
    template <class Key> class D0;
    template <class Key> class D1;

    template <class Key>
    class Hash {
    public:
        using KV = ds::KV<Key>;
        using Block = ds::Block<Key>;
        using KVList = std::list<KV>;
        using ItemIt = typename KVList::iterator;

        using BlockList = std::list<Block>;
        using BlockIt = typename BlockList::iterator;

        using Map = std::unordered_map<Key, Handle<Key>>;

        explicit Hash(D0<Key>& d0, D1<Key>& d1, std::size_t expected_size = 0)
            : d0_(d0), d1_(d1)
        {
            if (expected_size) map_.reserve(expected_size);
        }

        bool contains(const Key& k) const noexcept {
            return map_.find(k) != map_.end();
        }

        Handle<Key>* get(const Key& k) noexcept {
            auto it = map_.find(k);
            return it == map_.end() ? nullptr : &it->second;
        }
        const Handle<Key>* get(const Key& k) const noexcept {
            auto it = map_.find(k);
            return it == map_.end() ? nullptr : &it->second;
        }

        [[nodiscard]]
        std::pair<Handle<Key>*, bool>
            insert_or_update_in_hash(const Key& k, Tier tier, BlockIt bIt, ItemIt iIt) noexcept {
            auto [it, inserted] = map_.try_emplace(k);
            it->second.tier = tier;
            it->second.blockIt = bIt;
            it->second.itemIt = iIt;
            return { &it->second, inserted };
        }

        //delete item
        bool erase(const Key& k) noexcept {           
            return map_.erase(k) != 0;
        }

		//delete all item in specific block 
        void on_block_erased(BlockIt bIt) {
            for (const auto& kv : bIt->items) {
                map_.erase(kv.key);
            }
        }

        std::size_t size() const noexcept { return map_.size(); }
        bool        empty() const noexcept { return map_.empty(); }
        void        clear() noexcept { map_.clear(); }


    private:
        D0<Key>& d0_;
        D1<Key>& d1_;
        Map      map_;
    };

} // namespace ds
