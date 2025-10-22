#pragma once
#include <vector>
#include <utility>
#include <cstddef>
#include <cassert>
#include <algorithm>
#include <stdexcept>

namespace sssp {

    template<class Key>
    struct RelaxResult {
        std::vector<Key> W_union;
        std::vector<Key> decreased_keys;
    };

    template<class Key, class IndexOf>
    inline RelaxResult<Key> relax_k_steps(
        const std::vector<std::vector<std::pair<Key, double>>>& adj,
        std::vector<double>& db,
       /* std::vector<Key> S,*/
        std::vector<Key>& S,
        double B,
        std::size_t K,
        IndexOf vertex_index_fn)
    {
        const std::size_t n = adj.size();
        assert(db.size() == n && "db.size() must equal adj.size()");
        if (db.size() != n) throw std::invalid_argument("db.size() must equal adj.size()");
        RelaxResult<Key> out;
        out.W_union.reserve(n);

        std::vector<char> inW(n, 0);
        std::vector<char> decMarked(n, 0);

        auto push_union = [&](const Key& vKey) {
            std::size_t iv = vertex_index_fn(vKey);
            if (!inW[iv]) { inW[iv] = 1; out.W_union.push_back(vKey); }
            };

        // W0 = S
        for (const Key& u : S) push_union(u);
        std::vector<Key> Wi_prev = S;
       /* std::vector<Key> Wi_prev = std::move(S);*/
        std::vector<Key> Wi; Wi.reserve(256);
        std::vector<char> inWi(n, 0);
        for (std::size_t step = 1; step <= K; ++step) {
            Wi.clear();
            std::fill(inWi.begin(), inWi.end(), 0); 

            for (const Key& uKey : Wi_prev) {
                const std::size_t iu = vertex_index_fn(uKey);
                const double du = db[iu];
                for (const auto& [vKey, wuv] : adj[iu]) {
                    const std::size_t iv = vertex_index_fn(vKey);
                    const double cand = du + wuv;

                    if (cand <= db[iv]) {
                        if (cand < db[iv]) {
                            db[iv] = cand;
                            if (!decMarked[iv]) {
                                decMarked[iv] = 1;
                                out.decreased_keys.push_back(vKey);
                            }
                        }
                        if (cand < B && !inWi[iv]) {
                            inWi[iv] = 1;
                            Wi.push_back(vKey);
                        }
                    }
                }
            }

            for (const Key& vKey : Wi) push_union(vKey);
            if (Wi.empty()) break;
            Wi_prev.swap(Wi);
        }

        return out;
    }

} // namespace sssp
