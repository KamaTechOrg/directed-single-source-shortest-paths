#pragma once
#include <vector>
#include <utility>
#include <cstddef>
#include <cmath>
#include <algorithm>

#include "sssp/algorithms/relax.hpp"
#include "sssp/algorithms/types.hpp"

namespace sssp {


    namespace detail {
        inline bool tight(double sum, double dv) {
            const double diff = std::abs(sum - dv);
            const double scale = 1.0 + std::max(std::abs(sum), std::abs(dv));
            return diff <= 1e-12 * scale;
        }
        inline bool subtree_at_least_K(std::size_t root_idx,
            const std::vector<std::vector<std::size_t>>& children,
            std::size_t K)
        {
            std::size_t cnt = 0;
            std::vector<std::size_t> st; st.push_back(root_idx);
            while (!st.empty() && cnt < K) {
                auto x = st.back(); st.pop_back();
                ++cnt;
                for (auto y : children[x]) {
                    st.push_back(y);
                    if (cnt >= K) break;
                }
            }
            return cnt >= K;
        }
    } // namespace detail

    template<class Key, class IndexOf>
    inline PivotsResult<Key> find_pivots(
        const std::vector<std::vector<std::pair<Key, double>>>& adj,
        std::vector<double>& db,
        std::vector<Key> S,
        double B,
        std::size_t K,
        IndexOf index_of)
    {
        const std::size_t n = adj.size();
        const std::vector<Key> S0 = S; 

        auto rr = relax_k_steps<Key>(adj, db, std::move(S), B, K, index_of);
        const auto& W = rr.W_union;

        if (W.size() > K * S0.size()) {
            return PivotsResult<Key>{ S0, W };
        }

        std::vector<char> inW(n, 0), inS0(n, 0);
        for (const Key& v : W)  inW[index_of(v)] = 1;
        for (const Key& u : S0) inS0[index_of(u)] = 1;

        std::vector<std::size_t> pred_idx(n, (std::size_t)-1);
        std::vector<char>        pred_set(n, 0);

        for (const Key& uKey : W) {
            const std::size_t iu = index_of(uKey);
            const double du = db[iu];
            for (const auto& e : adj[iu]) {
                const Key& vKey = e.first;
                const double wuv = e.second;
                const std::size_t iv = index_of(vKey);
                if (!inW[iv]) continue;

                const double cand = du + wuv;
                if (detail::tight(cand, db[iv])) {
                    if (!pred_set[iv] || iu < pred_idx[iv]) {
                        pred_set[iv] = 1;
                        pred_idx[iv] = iu;
                    }
                }
            }
        }

        std::vector<int> indeg(n, 0);
        std::vector<std::vector<std::size_t>> children(n);
        for (const Key& vKey : W) {
            const std::size_t iv = index_of(vKey);
            const std::size_t iu = pred_idx[iv];
            if (iu != (std::size_t)-1 && inW[iu]) {
                children[iu].push_back(iv);
                ++indeg[iv];
            }
        }

        std::vector<Key> P;
        P.reserve(S0.size());
        for (const Key& uKey : S0) {
            const std::size_t iu = index_of(uKey);
            if (inW[iu] && indeg[iu] == 0 && detail::subtree_at_least_K(iu, children, K)) {
                P.push_back(uKey);
            }
        }

        return PivotsResult<Key>{ P, W };
    }

} // namespace sssp
