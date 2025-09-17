#pragma once
#include <vector>
#include <algorithm>
#include <cstddef>

#include "sssp/algorithms/types.hpp"        
#include "sssp/algorithms/find_pivots.hpp"  

namespace sssp {

    template<class Key, class IndexOf>
    BMSSPResult<Key> bmssp(
        int l,
        double B,
        const std::vector<Key>& S,
        const AdjList<Key>& adj,
        std::vector<double>& db,
        IndexOf index_of,
        std::size_t M,          
        std::size_t K           
    ) {
        (void)M; // השתקה זמנית של אזהרות “unused” בשלב 1

        if (l == 0) {
            return BMSSPResult<Key>{B, {}};
        }

        auto pw = find_pivots<Key>(adj, db, S, B, K, index_of);
        const auto& P = pw.P;

		//find the pivot with the smallest db value
        double B0_prime = B;
        if (!P.empty()) {
            auto it = std::min_element(
                P.begin(), P.end(),
                [&](const Key& a, const Key& b) {
                    return db[index_of(a)] < db[index_of(b)];
                }
            );

            B0_prime = db[index_of(*it)];
        }

        BMSSPResult<Key> out;
        out.Bprime = B0_prime;
        out.U = {};
        return out;
    }

} // namespace sssp
