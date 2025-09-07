// dual_block_structure.cpp
template<class Key>
typename D1<Key>::BlockIt
DualBlockStructure<Key>::choose_block_for_value_in_d1(double /*v*/) {
	//need implementation of tree in D1.cpp
}

template<class Key>
typename DualBlockStructure<Key>::InsertResult
DualBlockStructure<Key>::insert_or_decrease(const Key& k, double newVal)
{
    if (auto* h = hash_.get(k)) {
        const double oldVal = h->value(); 
        if (newVal >= oldVal) {
            return InsertResult::NoChange;
        }


        if (h->tier == Tier::D0) {
            auto& blk = *h->d0_;
            blk.items.erase(h->item);
            if (blk.items.empty()) {
                remove_block_d0(h->d0Blk); 
            }
        }
        else {
            auto& blk = *h->d1_;
            blk.items.erase(h->item);
            if (blk.items.empty()) {
                remove_block_d1(h->d1Blk);  
            }
            
        }

        auto bIt = choose_block_for_value_in_d1(newVal);
        auto iIt = bIt->items.emplace_front(KV{ k, newVal });


        if (bIt->items.size() > M1_) {
            split_if_needed(bIt); 
        }

        hash_.set(k, Handle{/*tier=*/Tier::D1, /*d0*/{}, /*d1=*/bIt, /*item=*/iIt });
        return InsertResult::Decreased;
    }

    auto bIt = choose_block_for_value_in_d1(newVal); 
    auto iIt = bIt->items.emplace_front(KV{ k, newVal });

    if (bIt->items.size() > M1_) {
        split_if_needed(bIt);
    }

    hash_.set(k, Handle{ Tier::D1, {}, bIt, iIt });
    return InsertResult::InsertedNew;
}

