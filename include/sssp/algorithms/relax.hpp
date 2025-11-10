#pragma once
#include <vector>
#include <utility>
#include <cstddef>
#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <cstdint>

namespace sssp {

    template<class Key>
    struct RelaxResult {
        std::vector<Key> W_union;
        std::vector<Key> decreased_keys;
    };

    // - epoch marking бочен fill(inWi)
    template<class Key, class IndexOf>
    inline RelaxResult<Key> relax_k_steps(
        const std::vector<std::vector<std::pair<Key, double>>>& adj,
        std::vector<double>& db,
        std::vector<Key>& S,     
        double B,
        std::size_t K,
        IndexOf vertex_index_fn)
    {
        const std::size_t n = adj.size();
        assert(db.size() == n && "db.size() must equal adj.size()");
        if (db.size() != n) throw std::invalid_argument("db.size() must equal adj.size()");

        static std::vector<std::uint8_t> t_inW;
        static std::vector<std::uint8_t> t_decMarked;
        static std::vector<std::uint32_t> t_inWiEpoch;
        static std::uint32_t t_epoch = 1;

        static std::vector<Key> t_Wi_prev;
        static std::vector<Key> t_Wi;

        if (t_inW.size() != n)        t_inW.assign(n, 0);
        if (t_decMarked.size() != n)  t_decMarked.assign(n, 0);
        if (t_inWiEpoch.size() != n)  t_inWiEpoch.assign(n, 0);

        RelaxResult<Key> out;
        out.W_union.clear();
        out.W_union.reserve(std::min<std::size_t>(n, S.size() * (K + 1)));
        out.decreased_keys.clear();
        out.decreased_keys.reserve(S.size() * 4);

        auto push_union = [&](const Key& vKey) {
            std::size_t iv = vertex_index_fn(vKey);
            if (!t_inW[iv]) { t_inW[iv] = 1; out.W_union.push_back(vKey); }
            };

        // W0 = S
        for (const Key& u : S) push_union(u);
        t_Wi_prev.assign(S.begin(), S.end());
        t_Wi.clear(); t_Wi.reserve(256);

        for (std::size_t step = 1; step <= K; ++step) {
            t_Wi.clear();
            ++t_epoch; 

            for (const Key& uKey : t_Wi_prev) {
                const std::size_t iu = vertex_index_fn(uKey);
                const double du = db[iu];

                for (const auto& [vKey, wuv] : adj[iu]) {
                    const std::size_t iv = vertex_index_fn(vKey);
                    const double cand = du + wuv;

                    if (cand <= db[iv]) {
                        if (cand < db[iv]) {
                            db[iv] = cand;
                            if (!t_decMarked[iv]) {
                                t_decMarked[iv] = 1;
                                out.decreased_keys.push_back(vKey);
                            }
                        }
                        if (cand < B && t_inWiEpoch[iv] != t_epoch) {
                            t_inWiEpoch[iv] = t_epoch;
                            t_Wi.push_back(vKey);
                        }
                    }
                }
            }

            for (const Key& vKey : t_Wi) push_union(vKey);
            if (t_Wi.empty()) break;
            t_Wi_prev.swap(t_Wi);
        }

        for (const Key& v : out.W_union) t_inW[vertex_index_fn(v)] = 0;
        for (const Key& v : out.decreased_keys) t_decMarked[vertex_index_fn(v)] = 0;

        return out;
    }

} // namespace sssp
