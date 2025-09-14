#pragma once
#include <list>
#include <cstddef>
#include <utility>
#include <algorithm>
#include <cassert>


#include "src/lib/internal/ds_common.hpp" 

template <class Key> class D0;
template <class Key> class D1;
template <class Key> class Hash;

template <class Key>
class DualBlockStructure {
public:
    using Node = ds::Node<Key>;
    using Block = ds::Block<Key>;
    using NodeList = std::list<Node>;

    DualBlockStructure(std::size_t maxBlockSize, std::size_t globalUpperBound, std::size_t expected_keys = 0);
    void initialize(std::size_t maxBlockSize, std::size_t globalUpperBound, std::size_t expected_keys = 0);

    std::size_t maxBlockSize() const noexcept { return maxBlockSize_; }
    std::size_t globalUpperBound() const noexcept { return globalUpperBound_; }

    void batch_prepend(NodeList&& items);

    std::pair<NodeList, double> pull();

    bool insert(const Key& key, double value);

    D0<Key>& d0()       noexcept { return d0_; }
    const D0<Key>& d0() const noexcept { return d0_; }
    D1<Key>& d1()       noexcept { return d1_; }
    const D1<Key>& d1() const noexcept { return d1_; }
    const Hash<Key>& map() const noexcept { return hash_; }

private:
    std::size_t maxBlockSize_;
    std::size_t globalUpperBound_;

    D0<Key>   d0_;
    D1<Key>   d1_;
    Hash<Key> hash_;

    void erase_existing_(const ds::Handle<Key>& h);
};
