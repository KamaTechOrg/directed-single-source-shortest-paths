#pragma once
#include <vector>
#include <utility>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <stdexcept>

#ifndef HAS_OMP
#  ifdef _OPENMP
#    include <omp.h>
#    define HAS_OMP 1
#  else
#    define HAS_OMP 0
#  endif
#else
#  if HAS_OMP
#    include <omp.h>
#  endif
#endif

#include "sssp/algorithms/relax.hpp"  // בשביל RelaxResult<Key>

namespace sssp {

    /// גרסה מקבילית קלה (light): ללא נעילות, רק מיזוג סופי של עדכונים
    template<class Key, class IndexOf>
    inline RelaxResult<Key> relax_k_steps_parallel_light(
        const std::vector<std::vector<std::pair<Key, double>>>& adj,
        std::vector<double>& db,
        const std::vector<Key>& S,
        double B,
        std::size_t K,
        IndexOf vertex_index_fn)
    {
        const std::size_t n = adj.size();
        if (db.size() != n) throw std::invalid_argument("db.size() must equal adj.size()");

        std::vector<std::uint8_t> inW(n, 0);

        static std::vector<std::uint32_t> inWiEpoch;
        static std::uint32_t epoch = 1;
        if (inWiEpoch.size() != n)
            inWiEpoch.assign(n, 0);

        RelaxResult<Key> out;
        out.W_union.reserve(std::min<std::size_t>(n, S.size() * (K + 1)));
        out.decreased_keys.reserve(S.size() * 4);

        auto push_union = [&](const Key& vKey) {
            const std::size_t iv = vertex_index_fn(vKey);
            if (!inW[iv]) { inW[iv] = 1; out.W_union.push_back(vKey); }
            };

        // שכבה ראשונה
        for (const Key& u : S) push_union(u);
        std::vector<Key> Wi_prev = S;

        for (std::size_t step = 1; step <= K && !Wi_prev.empty(); ++step) {
            const std::uint32_t curEpoch = ++epoch;

#if HAS_OMP
            const int P = omp_get_max_threads();
#else
            const int P = 1;
#endif

            struct Update {
                std::size_t iv;
                double cand;
                Key vKey;
                bool wantEnter;
            };

            std::vector<std::vector<Update>> tls_updates(P);
            for (int t = 0; t < P; ++t)
                tls_updates[t].reserve(256);

#if HAS_OMP
#pragma omp parallel for schedule(dynamic,64)
#endif
            for (int i = 0; i < static_cast<int>(Wi_prev.size()); ++i) {
#if HAS_OMP
                const int tid = omp_get_thread_num();
#else
                const int tid = 0;
#endif
                auto& updates = tls_updates[tid];
                const Key uKey = Wi_prev[static_cast<std::size_t>(i)];
                const std::size_t iu = vertex_index_fn(uKey);
                const double du = db[iu];

                for (const auto& edge : adj[iu]) {
                    const Key vKey = edge.first;
                    const std::size_t iv = vertex_index_fn(vKey);
                    const double cand = du + edge.second;
                    bool needDec = (cand < db[iv]);
                    bool needEnter = (cand < B);
                    if (needDec || needEnter)
                        updates.push_back(Update{ iv, cand, vKey, needEnter });
                }
            }

            std::vector<Key> Wi;
            Wi.reserve(Wi_prev.size() * 2);

            for (int t = 0; t < P; ++t)
                for (const auto& up : tls_updates[t]) {
                    if (up.cand < db[up.iv]) {
                        db[up.iv] = up.cand;
                        out.decreased_keys.push_back(up.vKey);
                    }
                    if (up.wantEnter && inWiEpoch[up.iv] != curEpoch) {
                        inWiEpoch[up.iv] = curEpoch;
                        Wi.push_back(up.vKey);
                    }
                }

            for (const Key& vKey : Wi) push_union(vKey);
            Wi_prev.swap(Wi);
        }

        return out;
    }

} // namespace sssp
