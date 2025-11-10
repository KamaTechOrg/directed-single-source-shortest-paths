#pragma once
#include <vector>
#include <utility>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <mutex>
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

#include "sssp/algorithms/relax.hpp"

namespace sssp {

    template<class Key, class IndexOf>
    inline RelaxResult<Key> relax_k_steps_parallel(
        const std::vector<std::vector<std::pair<Key, double>>>& adj,
        std::vector<double>& db,
        const std::vector<Key>& S,
        double B,
        std::size_t K,
        IndexOf vertex_index_fn)
    {
        const std::size_t n = adj.size();
        if (db.size() != n) throw std::invalid_argument("db.size() must equal adj.size()");

        // --- book-keeping ---
        std::vector<std::uint8_t> inW(n, 0);

        // epoch marking (protected by the same shard lock; no atomics)
        static std::vector<std::uint32_t> inWiEpoch;
        static std::uint32_t epoch = 1;
        if (inWiEpoch.size() != n) {
            inWiEpoch.assign(n, 0);
        }

        // finer sharding to reduce lock contention
        constexpr std::size_t SHARDS = 16384; // power of 2
        static std::vector<std::mutex> shard_locks(SHARDS);

        auto shard_of = [&](std::size_t iv) noexcept {
            return iv & (SHARDS - 1);
            };

        RelaxResult<Key> out;
        out.W_union.reserve(std::min<std::size_t>(n, S.size() * (K + 1)));
        out.decreased_keys.reserve(S.size() * 4);

        auto push_union = [&](const Key& vKey) {
            const std::size_t iv = vertex_index_fn(vKey);
            if (!inW[iv]) { inW[iv] = 1; out.W_union.push_back(vKey); }
            };

        // W0 = S
        for (const Key& u : S) push_union(u);
        std::vector<Key> Wi_prev = S;

        for (std::size_t step = 1; step <= K && !Wi_prev.empty(); ++step) {
            std::vector<Key> Wi; Wi.reserve(Wi_prev.size() * 2);

            const std::uint32_t curEpoch = ++epoch;

            // per-thread local buffers
#if HAS_OMP
            const int P = omp_get_max_threads();
#else
            const int P = 1;
#endif
            std::vector<std::vector<Key>> tls_Wi(P);
            std::vector<std::vector<Key>> tls_dec(P);
            for (int t = 0; t < P; ++t) {
                tls_Wi[t].reserve(256);
                tls_dec[t].reserve(256);
            }

            // --- parallel kernel ---
#if HAS_OMP
#pragma omp parallel for schedule(dynamic,64)
#endif
            for (int i = 0; i < static_cast<int>(Wi_prev.size()); ++i) {
#if HAS_OMP
                const int tid = omp_get_thread_num();
#else
                const int tid = 0;
#endif
                auto& LWi = tls_Wi[tid];
                auto& Ldec = tls_dec[tid];

                const Key uKey = Wi_prev[static_cast<std::size_t>(i)];
                const std::size_t iu = vertex_index_fn(uKey);
                const double du = db[iu];

                for (const auto& edge : adj[iu]) {
                    const Key& vKey = edge.first;
                    const double wuv = edge.second;
                    const std::size_t iv = vertex_index_fn(vKey);

                    const double cand = du + wuv;

                    // ---------- FAST PATH (no lock) ----------
                    // if neither decrease nor Wi-entry is needed, skip without locking
                    const double old = db[iv];
                    const bool need_dec = (cand < old);
                    const bool need_enter = (cand < B) && (inWiEpoch[iv] != curEpoch);
                    if (!need_dec && !need_enter) continue;

                    // ---------- SINGLE LOCK PER EDGE ----------
                    std::mutex& m = shard_locks[shard_of(iv)];
                    std::lock_guard<std::mutex> g(m);

                    // re-check under lock (double-checked pattern)
                    if (cand < db[iv]) {
                        db[iv] = cand;
                        Ldec.push_back(vKey);
                    }
                    if ((cand < B) && (inWiEpoch[iv] != curEpoch)) {
                        inWiEpoch[iv] = curEpoch;
                        LWi.push_back(vKey);
                    }
                }
            } // parallel for

            // merge TLS
            for (int t = 0; t < P; ++t) {
                Wi.insert(Wi.end(), tls_Wi[t].begin(), tls_Wi[t].end());
                out.decreased_keys.insert(out.decreased_keys.end(),
                    tls_dec[t].begin(), tls_dec[t].end());
            }

            for (const Key& vKey : Wi) push_union(vKey);
            Wi_prev.swap(Wi);
        }

        return out;
    }

} // namespace sssp

