// ds_common.hpp
//ds_common.hpp defines the core data types used across the project :
// Node(a key–value pair),
// Block(a list of nodes with an upper bound),
// and Handle(iterators pointing to a specific node within a block)
// .In short, it provides the shared building blocks that D0, D1, and Hash rely on.
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

    //that is a struct node
    template <class Key>
    struct Node {
        Key    key;    
        double value;  
    };

    //that is the struct block
    template <class Key>
    struct Block {
        std::list<Node<Key>> items;

        double blockUpper = detail::neg_inf();

        std::size_t size() const noexcept { return items.size(); }

    };


    //that is the value for key in the hash table
    template <class Key>
    struct Handle {
        using NodeList = std::list<Node<Key>>;
        using ItemIt = typename NodeList::iterator;

        using BlockList = std::list<Block<Key>>;
        using BlockIt = typename BlockList::iterator;

        BlockIt  blockIt{};
        ItemIt   itemIt{};

        const Key& key()   const { return itemIt->key; }
        const double& value() const { return itemIt->value; }

    };

} // namespace ds
