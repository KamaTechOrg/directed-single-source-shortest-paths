// ds_common.hpp
#pragma once
#include <list>
#include <limits>
#include <cstddef>

namespace ds {

    namespace detail {
        inline constexpr double neg_inf() {
            return -std::numeric_limits<double>::infinity();
        }
        inline constexpr double pos_inf() {
            return  std::numeric_limits<double>::infinity();
        }
    }

    template <class Key>
    struct KV {
        Key    key;    
        double value;  
    };

    template <class Key>
    struct Block {
        std::list<KV<Key>> items;

        double upper = detail::neg_inf(); 
        double lower = detail::pos_inf(); 

        std::size_t size() const noexcept { return items.size(); }

        void recompute_upper() {
            if (items.empty()) { upper = detail::neg_inf(); return; }
            double u = items.front().value;
            for (auto& kv : items) if (kv.value > u) u = kv.value;
            upper = u;
        }

        void recompute_lower() {
            if (items.empty()) { lower = detail::pos_inf(); return; }
            double l = items.front().value;
            for (auto& kv : items) if (kv.value < l) l = kv.value;
            lower = l;
        }

    };

    enum class Tier { D0, D1 };

    //that is the value for key in the hash table
    template <class Key>
    struct Handle {
        using KVList = std::list<KV<Key>>;
        using ItemIt = typename KVList::iterator;

        using BlockList = std::list<Block<Key>>;
        using BlockIt = typename BlockList::iterator;

        Tier     tier{ Tier::D1 }; 
        BlockIt  blockIt{};
        ItemIt   itemIt{};

        const Key& key()   const { return it->key; }
        const double& value() const { return it->value; }

    };

} // namespace ds
