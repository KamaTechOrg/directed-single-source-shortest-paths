#pragma once
#include <vector>
#include <utility>
#include <cstddef>
#include <algorithm>
#include <limits>
#include <type_traits>

#include "sssp/algorithms/types.hpp"        
#include "sssp/algorithms/find_pivots.hpp"  
#include "sssp/dual_block_structure.hpp"    
#include "sssp/algorithms/base_case.hpp"



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
        std::size_t Ksz    
    ) {
        static_assert(std::is_invocable_r_v<std::size_t, IndexOf, Key>,
            "index_of must be callable as size_t(Key)");

        // בסיס (placeholder):  BaseCase אמיתי כשיהיה מוכן
        if (l == 0) {
            //return BMSSPResult<Key>{ B, {} };
            return base_case<Key>(B, S, adj, db, index_of, Ksz);
        }

        // 4: FindPivots
        auto pw = find_pivots<Key>(adj, db, S, B, Ksz, index_of);
        const auto& P = pw.P;
        const auto& W = pw.W;

        // 5: Initialize(M,B)
        DualBlockStructure<Key> D;
        D.initialize(M, B); 

        // 6: Insert ⟨x, d̂[x]⟩ for x ∈ P
        for (const Key& x : P) {
            D.insert(x, db[index_of(x)]); 
        }

        // 7: i←0; B'0 ← min_{x∈P} d̂[x]; U←∅ (או B אם P ריק)
        int i = 0;
        double B0_prime = B;
        if (!P.empty()) {
            auto it = std::min_element(
                P.begin(), P.end(),
                [&](const Key& a, const Key& b) {
                    return db[index_of(a)] < db[index_of(b)];
                });
            B0_prime = db[index_of(*it)];
        }
        std::vector<Key> U;

        // 8: while |U| < k^{2ℓt} && D לא ריק
        // כרגע placeholder: Ksz*Ksz. אם יש לך t, חשבי k^(2*l*t)
        const std::size_t U_limit = Ksz * Ksz;

        while (U.size() < U_limit && !D.empty()) {
            // 9: i ← i + 1
            ++i;

            // 10: (Bi, Si) ← D.Pull()
            auto [Si, Bi] = D.pull();   

            // 11: (B'i, Ui) ← BMSSP(l−1, Bi, Si)
            auto sub = bmssp<Key>(l - 1, Bi, Si, adj, db, index_of,
                /*M'*/ std::max<std::size_t>(1, M / 2), 
                Ksz);

            const double Bi_prime = sub.Bprime;
            const auto& Ui = sub.U;

            // 12: U ← U ∪ Ui
            U.insert(U.end(), Ui.begin(), Ui.end());

            // 13: K ← ∅
            std::vector<std::pair<Key, double>> Kbatch;
            Kbatch.reserve(Ui.size() * 2);

            // Optional: marker to avoid pushing the same vertex into K twice in this iteration
            std::vector<char> inK(db.size(), 0);

            // 14–20: Single-pass relax over outgoing edges of Ui.
            // - Update db[v] if cand <= db[v]  (<= is important; see Remark 3.4)
            // - If db[v] ∈ [Bi, B): push into D
            // - If db[v] ∈ [B'i, Bi): push into Kbatch (dedup with inK)
            for (const Key& u : Ui) {
                const std::size_t ui = index_of(u);
                for (const auto& [v, wuv] : adj[ui]) {
                    const std::size_t vi = index_of(v);
                    const double cand = db[ui] + wuv;

                    if (cand <= db[vi]) {
                        db[vi] = cand;

                        if (cand >= Bi && cand < B) {
                            // Insert into D for the upper band [Bi, B)
                            D.insert(v, db[vi]);
                        }
                        else if (cand >= Bi_prime && cand < Bi) {
                            // Collect into K for the lower band [B'i, Bi)
                            if (!inK[vi]) {
                                Kbatch.emplace_back(v, db[vi]);
                                inK[vi] = 1;
                            }
                        }
                        // else: cand < B'i or cand >= B → ignore in this iteration
                    }
                }
            }

            // 21: BatchPrepend( K ∪ { <x, d̂[x]> : x ∈ S, d̂[x] ∈ [B'i, Bi) } )
            // Also add from S into K (avoid duplicates using the same marker).
            for (const Key& x : S) {
                const std::size_t xi = index_of(x);
                const double dx = db[xi];
                if (dx >= Bi_prime && dx < Bi) {
                    if (!inK[xi]) {
                        Kbatch.emplace_back(x, dx);
                        inK[xi] = 1;
                    }
                }
            }

            // If your DualBlockStructure::batch_prepend expects a NodeList,
            // convert here. If you've changed it to accept vector<pair<Key,double>>,
            // you can pass Kbatch directly.
            if (!Kbatch.empty()) {
                // Convert Kbatch (vector<pair<Key,double>>) into NodeList for D.batch_prepend
                std::list<ds::Node<Key>> nl;
                nl.clear();
                nl.resize(0);

                // Reserve is not available for std::list; just emplace_back items
                for (const auto& kv : Kbatch) {
                    nl.emplace_back(ds::Node<Key>{ kv.first, kv.second });
                }

                // Splice the whole list into D0 via DualBlockStructure::batch_prepend
                D.batch_prepend(std::move(nl));

            }


            //  B'0
            B0_prime = std::min(B0_prime, Bi_prime);
        }

        // 22:  – B' = min{B'i, B} ו-U ← U ∪ { x∈W : d̂[x] < B' }
        const double Bprime = std::min(B0_prime, B);
        for (const Key& x : W) {
            if (db[index_of(x)] < Bprime) U.push_back(x);
        }

        std::sort(U.begin(), U.end(),
            [&](const Key& a, const Key& b) { return index_of(a) < index_of(b); });
        U.erase(std::unique(U.begin(), U.end(),
            [&](const Key& a, const Key& b) { return index_of(a) == index_of(b); }),
            U.end());

        return BMSSPResult<Key>{ Bprime, std::move(U) };
    }

} // namespace sssp
