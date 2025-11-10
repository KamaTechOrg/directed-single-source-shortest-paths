#pragma once
#include <vector>
#include <utility>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <atomic>

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

    /// גרסה מקבילית עם CAS:
    /// - מרחקים נשמרים גם בווקטור אטומי זמני כדי שחוטים יוכלו לעדכן במקביל
    /// - כניסה לשכבה הבאה עם compare_exchange כדי למנוע כפילות
    /// - איסוף תוצאות per-thread ואז מיזוג סופי
    template<class Key, class IndexOf>
    inline RelaxResult<Key> relax_k_steps_parallel_cas(
        const std::vector<std::vector<std::pair<Key, double>>>& adj,
        std::vector<double>& db,
        const std::vector<Key>& S,
        double B,
        std::size_t K,
        IndexOf vertex_index_fn)
    {
        const std::size_t n = adj.size();
        if (db.size() != n) {
            throw std::invalid_argument("db.size() must equal adj.size()");
        }

        // שכבה אטומית של מרחקים כדי שחוטים יוכלו לעדכן במקביל
        std::vector<std::atomic<double>> db_atomic(n);
        for (std::size_t i = 0; i < n; ++i) {
            db_atomic[i].store(db[i], std::memory_order_relaxed);
        }

        // סימון גלובלי מי כבר הופיע ב-W_union
        std::vector<std::uint8_t> inW(n, 0);

        // סימון לשכבה הנוכחית (epoch) – אטומי לכל צומת
        std::vector<std::atomic<std::uint32_t>> inWiEpoch(n);
        for (std::size_t i = 0; i < n; ++i) {
            inWiEpoch[i].store(0, std::memory_order_relaxed);
        }
        std::atomic<std::uint32_t> epoch{ 1 };

        RelaxResult<Key> out;
        out.W_union.reserve(std::min<std::size_t>(n, S.size() * (K + 1)));
        out.decreased_keys.reserve(S.size() * 4);

        // הוספה בטוחה לאיחוד (לא במקביל)
        auto push_union = [&](const Key& vKey) {
            const std::size_t iv = vertex_index_fn(vKey);
            if (!inW[iv]) {
                inW[iv] = 1;
                out.W_union.push_back(vKey);
            }
            };

        // שכבה ראשונה
        for (const Key& u : S) {
            push_union(u);
        }
        std::vector<Key> Wi_prev = S;

        for (std::size_t step = 1; step <= K && !Wi_prev.empty(); ++step) {

            // epoch חדש לצעד הזה
            const std::uint32_t curEpoch =
                epoch.fetch_add(1, std::memory_order_acq_rel) + 1;

#if HAS_OMP
            const int P = omp_get_max_threads();
#else
            const int P = 1;
#endif

            // לכל חוט יהיו:
            // 1. וקטור צמתים לשכבה הבאה
            // 2. וקטור decreased keys
            std::vector<std::vector<Key>> tls_next(P);
            std::vector<std::vector<Key>> tls_decreased(P);
            for (int t = 0; t < P; ++t) {
                tls_next[t].reserve(128);
                tls_decreased[t].reserve(128);
            }

#if HAS_OMP
#pragma omp parallel for schedule(dynamic,64)
#endif
            for (int i = 0; i < static_cast<int>(Wi_prev.size()); ++i) {

#if HAS_OMP
                const int tid = omp_get_thread_num();
#else
                const int tid = 0;
#endif
                auto& my_next = tls_next[tid];
                auto& my_dec = tls_decreased[tid];

                const Key uKey = Wi_prev[static_cast<std::size_t>(i)];
                const std::size_t iu = vertex_index_fn(uKey);
                const double du = db_atomic[iu].load(std::memory_order_relaxed);

                // עיבוד כל השכנים של u
                for (const auto& edge : adj[iu]) {
                    const Key vKey = edge.first;
                    const std::size_t iv = vertex_index_fn(vKey);
                    const double cand = du + edge.second;

                    // --- 1) ניסיון להקטין את המרחק עם CAS ---
                    double old = db_atomic[iv].load(std::memory_order_relaxed);
                    while (cand < old) {
                        // ננסה להחליף old ב-cand
                        if (db_atomic[iv].compare_exchange_weak(
                            old,
                            cand,
                            std::memory_order_acq_rel,
                            std::memory_order_relaxed))
                        {
                            // הצליח – נרשום שהצומת הזה ירד
                            my_dec.push_back(vKey);
                            break;
                        }

                    }

                    // --- 2) בדיקה אם נכנס לשכבה הבאה ---
                    if (cand < B) {
                        std::uint32_t expected =
                            inWiEpoch[iv].load(std::memory_order_relaxed);
                        if (expected != curEpoch) {
                            // נסמן את הצומת לשכבה הזו, רק אם אף אחד לא הספיק
                            if (inWiEpoch[iv].compare_exchange_strong(
                                expected,
                                curEpoch,
                                std::memory_order_acq_rel,
                                std::memory_order_relaxed))
                            {
                                my_next.push_back(vKey);
                            }
                        }
                    }
                }
            } // end parallel for

            // מיזוג שכבה חדשה וכל ה-decreased
            std::vector<Key> Wi;
            for (int t = 0; t < P; ++t) {
                // השכבה הבאה
                for (const Key& vKey : tls_next[t]) {
                    Wi.push_back(vKey);
                    push_union(vKey);
                }
                // ה-decreased
                for (const Key& vKey : tls_decreased[t]) {
                    out.decreased_keys.push_back(vKey);
                }
            }

            // נעדכן את db הרגיל רק עבור מי שנמצא ב-Wi
            // (אפשר גם לעשות לכולם בסוף – זול יחסית)
            for (const Key& vKey : Wi) {
                const std::size_t iv = vertex_index_fn(vKey);
                db[iv] = db_atomic[iv].load(std::memory_order_relaxed);
            }

            Wi_prev.swap(Wi);
        }

        // בסוף נסנכרן את כל ה-db מהאטומי (כדי שלא יהיה פער)
        for (std::size_t i = 0; i < n; ++i) {
            db[i] = db_atomic[i].load(std::memory_order_relaxed);
        }

        return out;
    }

} // namespace sssp
