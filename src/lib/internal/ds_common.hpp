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
    struct Node {
        Key    key;    
        double value;  
    };

    template <class Key>
    struct Block {
        std::list<Node<Key>> items;

        double upper = detail::neg_inf(); 

        std::size_t size() const noexcept { return items.size(); }

    };


    //that is the value for key in the hash table
    template <class Key>
    struct Handle {
        using KVList = std::list<Node<Key>>;
        using ItemIt = typename KVList::iterator;

        using BlockList = std::list<Block<Key>>;
        using BlockIt = typename BlockList::iterator;

        BlockIt  blockIt{};
        ItemIt   itemIt{};

        const Key& key()   const { return itemIt->key; }
        const double& value() const { return itemIt->value; }

    };

} // namespace ds
