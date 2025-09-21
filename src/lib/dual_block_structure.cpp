#include "sssp/dual_block_structure.hpp"

#include "d0.hpp"
#include "d1.hpp"
#include "hash.hpp"
#include <string>
#include <limits>

template <class Key>
DualBlockStructure<Key>::DualBlockStructure(std::size_t maxBlockSize, std::size_t globalUpperBound, std::size_t expected_keys)
    : maxBlockSize_(maxBlockSize)
    , globalUpperBound_(globalUpperBound)
    , d0_(maxBlockSize)
    , d1_(maxBlockSize, static_cast<double>(globalUpperBound))
    , hash_(expected_keys)                    
{
    assert(maxBlockSize_ > 0 && "DualBlockStructure: maxBlockSize must be > 0");
}

template <class Key>
void DualBlockStructure<Key>::initialize(std::size_t maxBlockSize, std::size_t globalUpperBound, std::size_t expected_keys)
{
    assert(maxBlockSize > 0 && "DualBlockStructure::initialize: maxBlockSize must be > 0");

    maxBlockSize_ = maxBlockSize;
    globalUpperBound_ = globalUpperBound;

    d0_ = D0<Key>(maxBlockSize);
    d1_ = D1<Key>(maxBlockSize, static_cast<double>(globalUpperBound));
    hash_ = Hash<Key>(expected_keys);
}

template <class Key>
void DualBlockStructure<Key>::batch_prepend(NodeList&& items) {
    d0_.batchPrepend(std::move(items));
}

template <class Key>
std::pair<typename DualBlockStructure<Key>::NodeList, double>
DualBlockStructure<Key>::pull() {
    NodeList out;

    double bound = std::numeric_limits<double>::infinity();

    double d0_second = std::numeric_limits<double>::infinity();
    auto [from_d0, remaining] = d0_.pull(&d0_second, d0_.maxBlockSize());
    out.splice(out.end(), from_d0);

    if (remaining > 0) {
        d0_second = std::numeric_limits<double>::infinity();

        auto [from_d1, d1_second] = d1_.pull(remaining);
        out.splice(out.end(), from_d1);
        bound = d1_second; 
    }
    else {
        bound = d0_second;
    }

    return { std::move(out), bound };
}

template <class Key>
bool DualBlockStructure<Key>::insert(const Key& key, double value) {
    if (auto* h = hash_.get(key)) {
        double old_val = h->value();
        if (old_val == value) {
            return false; 
        }
        erase_existing_(*h);
        hash_.erase(key);
    }

    auto bit = d1_.choose_block_for_value(value);

    auto& items = bit->items;                       
    auto iit = items.insert(items.end(), Node{ key, value });

    ds::Handle<Key> nh;
    nh.blockIt = bit;
    nh.itemIt = iit;

    hash_.insert(key, nh);
    return true;
}

template <class Key>
void DualBlockStructure<Key>::erase_existing_(const ds::Handle<Key>& h) {
    d1_.delete_item(h.blockIt, &*h.itemIt);
}


template class DualBlockStructure<int>;
template class DualBlockStructure<char>;
template class DualBlockStructure<std::string>;

