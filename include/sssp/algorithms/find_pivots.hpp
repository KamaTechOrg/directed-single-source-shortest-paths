//אחרי שיפור נוסף
#pragma once
#include <vector>
#include <utility>
#include <cstddef>
#include <cmath>
#include <algorithm>
#include <limits>
#include <cstdint>

#include "sssp/algorithms/relax.hpp"
#include "sssp/algorithms/relax_parallel.hpp"
#include "sssp/algorithms/relax_parallel_light.hpp"
#include "sssp/algorithms/relax_parallel_cas.hpp"
#include "sssp/algorithms/types.hpp"

namespace sssp {

    namespace detail {
        // מהיר יותר ל-double: fabs/fmax, inline
        inline bool tight(double sum, double dv) {
            const double diff = std::fabs(sum - dv);
            const double scale = 1.0 + std::fmax(std::fabs(sum), std::fabs(dv));
            return diff <= 1e-12 * scale;
        }
    } // namespace detail

    // בחירת הורה "tight" + קילוף עלים לחישוב גודל תת־עץ עד K — ללא children דו־ממדי וללא מיונים.
    template<class Key, class IndexOf>
    inline PivotsResult<Key> find_pivots(
        const std::vector<std::vector<std::pair<Key, double>>>& adj,
        std::vector<double>& db,
        std::vector<Key> S,
        double B,
        std::size_t K,
        IndexOf vertex_index_fn)
    {
        const std::size_t n = adj.size();
        const std::vector<Key> S0 = S;

        // ====== באפרים ממוחזרים (thread_local) ======
        // סימונים: חבר ב-W, יש הורה "tight"
        static thread_local std::vector<std::uint8_t> t_inW;
        static thread_local std::vector<std::uint8_t> t_predSet;

        // הורה (אינדקס) לכל צומת ב-W; -1 אם אין
        static thread_local std::vector<std::size_t>  t_parent;

        // #children בגרף ה"הורות" (לצורך קילוף עלים)
        static thread_local std::vector<int>          t_indeg;

        // גודל תת־עץ מצומצם ל-K
        static thread_local std::vector<int>          t_sizeCapK;

        // תורי עבודה
        static thread_local std::vector<std::size_t>  t_queue;
        static thread_local std::vector<std::size_t>  w_idx;    // אינדקסים של צמתים ב-W

        if (t_inW.size() != n) {
            t_inW.assign(n, 0);
            t_predSet.assign(n, 0);
            t_parent.assign(n, (std::size_t)-1);
            t_indeg.assign(n, 0);
            t_sizeCapK.assign(n, 0);
            t_queue.clear();
            w_idx.clear();
        }

        // ====== K-Relax (גרסה שלך) ======
        // הערה: מומלץ לשלב בעתיד epoch-marking בתוך relax_k_steps כדי לצמצם עלות איפוסי db.
        //auto rr = relax_k_steps<Key>(adj, db, S, B, K, vertex_index_fn);

        // חדש (מקבילי):
        //auto rr = relax_k_steps_parallel<Key>(adj, db, S, B, K, vertex_index_fn);

        //מקבילי עם שיפור
        //auto rr = relax_k_steps_parallel_light<Key>(adj, db, S, B, K, vertex_index_fn);

        //cas
        auto rr = sssp::relax_k_steps_parallel_cas<Key>(adj, db, S, B, K, vertex_index_fn);



        const auto& W = rr.W_union;

        // אם W גדול מדי — לפי ההגדרה שלכם: חוזרים עם S0 (אין פיבוטים חדשים)
        if (W.size() > K * S0.size()) {
            return PivotsResult<Key>{ S0, W };
        }

        // ====== סימון נקודתי של W ======
        w_idx.clear();
        w_idx.reserve(W.size());
        for (const Key& v : W) {
            const std::size_t iv = vertex_index_fn(v);
            if (!t_inW[iv]) {
                t_inW[iv] = 1;
                w_idx.push_back(iv);
            }
        }

        // איפוס נקודתי לשדות שנשתמש בהם רק עבור הצמתים ב-W
        for (std::size_t iv : w_idx) {
            t_parent[iv] = (std::size_t)-1;
            t_predSet[iv] = 0;
            t_indeg[iv] = 0;
            t_sizeCapK[iv] = 0;  // נציב ל-1 בהמשך
        }

        // ====== מציאת הורה "tight" לכל iv ∈ W ======
        // עוברים על כל u ∈ W, בודקים שכנים v ב-W, אם du + wuv "tight" ל-db[v] — u הוא הורה אפשרי של v.
        // שברירי ביצועים:
        //  - חישוב iu/du פעם אחת לכל u
        //  - דילוג מהיר אם v לא ב-W (t_inW[iv] == 0)
        for (const Key& uKey : W) {
            const std::size_t iu = vertex_index_fn(uKey);
            const double du = db[iu];

            const auto& row = adj[iu];
            for (const auto& edge : row) {
                const std::size_t iv = vertex_index_fn(edge.first);
                if (!t_inW[iv]) continue; // v לא ב-W → לא חלק מהעצים שלנו

                const double cand = du + edge.second;
                // cand "שווה" ל-db[iv] בטולרנס → קצה tight
                if (detail::tight(cand, db[iv])) {
                    // נשמור הורה יחיד: אם אין עדיין, או נשבור שוויון לפי אינדקס קטן יותר
                    if (!t_predSet[iv] || iu < t_parent[iv]) {
                        t_predSet[iv] = 1;
                        t_parent[iv] = iu;
                    }
                }
            }
        }

        // ====== קילוף עלים + צבירת גדלים עד K ======
        // אתחול: כל צומת סופר את עצמו
        for (std::size_t iv : w_idx) {
            t_sizeCapK[iv] = 1;
        }
        // חישוב מספר ילדים להורים
        for (std::size_t iv : w_idx) {
            const std::size_t p = t_parent[iv];
            if (p != (std::size_t)-1 && t_inW[p]) {
                ++t_indeg[p];
            }
        }

        // תור התחלה: כל מי שאין לו ילדים
        t_queue.clear();
        t_queue.reserve(w_idx.size());
        for (std::size_t iv : w_idx) {
            if (t_indeg[iv] == 0) t_queue.push_back(iv);
        }

        // קליפות: עלים → הורה, עד שאין ילדים. מצמצמים ל-K למניעת גדילה מיותרת.
        for (std::size_t qi = 0; qi < t_queue.size(); ++qi) {
            const std::size_t v = t_queue[qi];
            const std::size_t p = t_parent[v];
            if (p != (std::size_t)-1 && t_inW[p]) {
                int& sp = t_sizeCapK[p];
                const int nv = t_sizeCapK[v];
                const int sum = sp + nv;
                sp = (sum < static_cast<int>(K) ? sum : static_cast<int>(K));
                if (--t_indeg[p] == 0) {
                    t_queue.push_back(p);
                }
            }
        }

        // ====== בחירת פיבוטים: u ∈ S0 וגם u ∈ W וגם |subtree(u)| ≥ K ======
        std::vector<Key> P;
        P.reserve(S0.size());
        for (const Key& uKey : S0) {
            const std::size_t iu = vertex_index_fn(uKey);
            if (t_inW[iu] && t_sizeCapK[iu] >= static_cast<int>(K)) {
                P.push_back(uKey);
            }
        }

        // ====== ניקוי סימונים נקודתי ======
        for (std::size_t iv : w_idx) {
            t_inW[iv] = 0;
            t_predSet[iv] = 0;
            // t_parent/indeg/sizeCapK יאותחלו ממילא בריצה הבאה נקודתית
        }

        return PivotsResult<Key>{ P, W };
    }

} // namespace sssp
