#include "utils/AlgorithmRecorder.hpp"
#include "utils/generators/graph.hpp"
#include <bits/stdc++.h>

using namespace std;

#define ii pair<int,int>

using T = int;

#define all(x) x.begin(),x.end()


// d^2 of closest pair - O(N log N)
T closest_pair(vector<pt<T>> v, AlgorithmRecorder* recorder = nullptr) {
    if (sz(v) <= 1) return 0;
    sort(all(v));

    if (recorder) {
        for (auto p : v) recorder->record_circle(p, 1.0, BLACK, BLACK);
        recorder->commit_step();
    }

    vector<pt<T>> t(sz(v));
    auto go = [&](auto& go, int l, int r) -> T {
        if (r - l <= 1) return inf;
        int m = l + (r - l) / 2;
        T mid = v[m].x;

        if (recorder) {
            // draw split line and currently considered points
            for (auto p : v) recorder->record_circle(p, 1.0, BLACK, BLACK);
            recorder->record_line(pt(mid, -10000), pt(mid, 10000), BLUE);
            for (int i = l; i < r; i++) recorder->record_circle(v[i], 1.5, BLUE, BLUE);
            recorder->commit_step();
        }

        T ans = min(go(go, l, m), go(go, m, r));

        merge(begin(v) + l, begin(v) + m, begin(v) + m, begin(v) + r,
                begin(t), [](pt<T> a, pt<T> b) { return sgn(a.y - b.y) < 0; });
        copy(t.begin(), t.begin() + (r - l), v.begin() + l);

        if (recorder && ans < inf) {
            // draw split line h limit box
            ld d = sqrt((ld)ans);
            for (auto p : v) recorder->record_circle(p, 1.0, BLACK, BLACK);
            recorder->record_line(pt<T>(mid, -10000), pt<T>(mid, 10000), BLUE);
            for (int i = l; i < r; i++) recorder->record_circle(v[i], 1.5, BLUE, BLUE);
            recorder->record_line(pt<T>(mid - d, -10000), pt<T>(mid - d, 10000), {0.5, 0.5, 0.5, 0.5});
            recorder->record_line(pt<T>(mid + d, -10000), pt<T>(mid + d, 10000), {0.5, 0.5, 0.5, 0.5});
            recorder->commit_step();
        }

        int k = 0;
        for (int i = l; i < r; i++) {
            if (sgn(sq(v[i].x - mid) - ans) >= 0) continue;
            for (int j = k - 1; j >= 0; j--) {
                if (sgn(sq(v[i].y - t[j].y) - ans) >= 0) break;

                T d2 = dist2(v[i], t[j]);
                if (d2 < ans) {
                    ans = d2;
                    if (recorder) {
                        // found better split
                        for (auto p : v) recorder->record_circle(p, 1.0, BLACK, BLACK);
                        recorder->record_line(pt(mid, -10000), pt(mid, 10000), BLUE);
                        for (int i = l; i < r; i++) recorder->record_circle(v[i], 1.5, BLUE, BLUE);
                        recorder->record_circle(v[i], 2.0, RED, RED);
                        recorder->record_circle(t[j], 2.0, RED, RED);
                        recorder->record_line(v[i], t[j], RED);
                        recorder->commit_step();
                    }
                }
            }
            t[k++] = v[i];
        }
        return ans;
    };

    T res = go(go, 0, sz(v));

    if (recorder) {
        for (auto p : v) recorder->record_circle(p, 1.0, BLACK, BLACK);
        recorder->commit_step();
    }

    return res;
}

int main() {
    int n; cin >> n;
    int mn = -100, mx = 100;
    vector<pt<T>> pts(n);
    for (auto &p : pts) p = random_pt(9*mn/10, 9*mx/10);

    AlgorithmRecorder recorder(mn,mx,mn,mx);

    closest_pair(pts, &recorder);

    recorder.run(500);
    return 0;
}
