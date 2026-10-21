#include <bits/stdc++.h>
#include "geom/geom.hpp"
#include "utils/generators/geometry.hpp"

using namespace std;

#define ii pair<int,int>

struct HalfEdge {
    int origin = -1;
    int twin = -1;
    int next = -1;
    int prev = -1;
};

template<class T>
ld qualidade(pt<T> p1, pt<T> p2, pt<T> p3) {
    ld area = abs((ld)sarea2(p1, p2, p3));
    ld dx = (ld)max({p1.x, p2.x, p3.x}) - (ld)min({p1.x, p2.x, p3.x});
    ld dy = (ld)max({p1.y, p2.y, p3.y}) - (ld)min({p1.y, p2.y, p3.y});
    ld bb_area = dx * dy;
    if (bb_area <= eps) return 1.0;
    return clamp((ld)1.0 - (area / bb_area), (ld)0.0, (ld)1.0);
}

void flip_edge(int e, vector<HalfEdge>& he) {
    int t = he[e].twin;
    if (t == -1) return;

    int en = he[e].next;
    int ep = he[e].prev;
    int tn = he[t].next;
    int tp = he[t].prev;

    he[e].origin = he[ep].origin;
    he[t].origin = he[tp].origin;

    he[e].next = tp;
    he[tp].next = en;
    he[en].next = e;
    he[e].prev = en;
    he[en].prev = tp;
    he[tp].prev = e;

    he[t].next = ep;
    he[ep].next = tn;
    he[tn].next = t;
    he[t].prev = tn;
    he[tn].prev = ep;
    he[ep].prev = t;
}

vector<HalfEdge> build_he(const vector<array<int, 3>>& triangulation) {
    int num_tris = sz(triangulation);
    vector<HalfEdge> he(num_tris * 3);
    map<pair<int, int>, int> edge_map;
    for (int f = 0; f < num_tris; f++) {
        auto [u,v,w] = triangulation[f];

        int e0 = 3*f + 0;
        int e1 = 3*f + 1;
        int e2 = 3*f + 2;
        he[e0] = {u, -1, e1, e2};
        he[e1] = {v, -1, e2, e0};
        he[e2] = {w, -1, e0, e1};

        edge_map[{u, v}] = e0;
        edge_map[{v, w}] = e1;
        edge_map[{w, u}] = e2;
    }

    for (int i = 0; i < sz(he); i++) {
        if (he[i].twin != -1) continue;
        int u = he[i].origin;
        int v = he[he[i].next].origin;
        auto it = edge_map.find({v, u});
        if (it != edge_map.end()) {
            int t = it->second;
            he[i].twin = t;
            he[t].twin = i;
        }
    }
    return he;
}

// remove collinear points
template<class T>
vector<int> clean_poly(const vector<pt<T>>& pts) {
    int n = sz(pts);
    if (n < 3) return {};

    vector<int> idx_list(n);
    iota(idx_list.begin(), idx_list.end(), 0);

    vector<int> idx;
    for (int id : idx_list) {
        while (sz(idx) >= 2 && col(pts[idx[sz(idx)-2]], pts[idx.back()], pts[id]))
            idx.pop_back();
        idx.push_back(id);
    }
    while (sz(idx) >= 3 && col(pts[idx[sz(idx)-2]], pts[idx.back()], pts[idx[0]]))
        idx.pop_back();
    while (sz(idx) >= 3 && col(pts[idx.back()], pts[idx[0]], pts[idx[1]]))
        idx.erase(idx.begin());

    return idx;
}

template<class T>
vector<HalfEdge> triangulate(const vector<pt<T>>& pts) {
    vector<int> idx = clean_poly(pts);
    int n = sz(idx);
    if (n < 3) return {};

    vector<int> prev(n), next(n);
    for (int i = 0; i < n; i++) {
        prev[i] = (i-1+n) % n;
        next[i] = (i+1) % n;
    }

    auto check_ear = [&](int cur) -> bool {
        int pre = prev[cur];
        int nxt = next[cur];
        const auto& a = pts[idx[pre]];
        const auto& b = pts[idx[cur]];
        const auto& c = pts[idx[nxt]];

        if (!ccw(a, b, c)) return false;

        T min_x = min({a.x, b.x, c.x});
        T max_x = max({a.x, b.x, c.x});
        T min_y = min({a.y, b.y, c.y});
        T max_y = max({a.y, b.y, c.y});

        int i = next[nxt];
        while (i != pre) {
            const auto& p = pts[idx[i]];
            if (p.x>=min_x && p.x<=max_x && p.y>=min_y && p.y<=max_y) {
                if (in_tri(p, a, b, c)) return false;
            }
            i = next[i];
        }
        return true;
    };

    vector<bool> is_ear(n, false);
    vector<int> ears;
    ears.reserve(n);

    for (int i = 0; i < n; i++) {
        if (check_ear(i)) {
            is_ear[i] = true;
            ears.push_back(i);
        }
    }

    vector<array<int, 3>> triangulation;
    triangulation.reserve(n-2);

    int remaining = n;
    int last_alive = 0;

    while (remaining > 3) {
        while (!ears.empty() && !is_ear[ears.back()])
            ears.pop_back();

        if (ears.empty()) {
            int fallback = -1;
            int it = last_alive;
            for (int step = 0; step < remaining; step++) {
                if (ccw(pts[idx[prev[it]]], pts[idx[it]], pts[idx[next[it]]])) {
                    fallback = it;
                    break;
                }
                it = next[it];
            }
            if (fallback == -1) fallback = last_alive;
            ears.push_back(fallback);
            is_ear[fallback] = true;
        }

        int cur = ears.back(); ears.pop_back();
        int pre = prev[cur];
        int nxt = next[cur];

        triangulation.push_back({idx[pre], idx[cur], idx[nxt]});

        is_ear[cur] = false;
        next[pre] = nxt;
        prev[nxt] = pre;
        remaining--;
        last_alive = pre;

        if (check_ear(pre)) {
            if (!is_ear[pre]) ears.push_back(pre);
            is_ear[pre] = true;
        } else is_ear[pre] = false;

        if (check_ear(nxt)) {
            if (!is_ear[nxt]) ears.push_back(nxt);
            is_ear[nxt] = true;
        } else is_ear[nxt] = false;
    }
    if (remaining == 3) {
        int a = last_alive;
        int b = next[a];
        int c = next[b];
        triangulation.push_back({idx[a], idx[b], idx[c]});
    }
    for (auto& t : triangulation) {
        if (!ccw(pts[t[0]], pts[t[1]], pts[t[2]])) {
            swap(t[0], t[1]);
        }
    }

    return build_he(triangulation);
}

template<class T>
vector<HalfEdge> optimize_steepest_climb(vector<HalfEdge> he, const vector<pt<T>>& pts) {
    int n = sz(he);
    if (n == 0) return he;

    using pld = pair<ld, ld>;
    vector<pld> gain(n, {-1.0, -1.0});
    priority_queue<pair<pld, int>> pq;

    auto eval_gain = [&](int e) -> pld {
        int t = he[e].twin;
        if (t == -1) return {-1, -1};

        int a = he[e].origin;
        int b = he[he[t].prev].origin;
        int c = he[t].origin;
        int d = he[he[e].prev].origin;

        if (!ccw(pts[a], pts[b], pts[c]) || !ccw(pts[b], pts[c], pts[d]) ||
            !ccw(pts[c], pts[d], pts[a]) || !ccw(pts[d], pts[a], pts[b])) {
            return {-1, -1};
        }

        ld qb1 = qualidade(pts[a], pts[c], pts[d]);
        ld qb2 = qualidade(pts[c], pts[a], pts[b]);
        ld qa1 = qualidade(pts[a], pts[b], pts[d]);
        ld qa2 = qualidade(pts[b], pts[c], pts[d]);

        ld wg = max(qb1, qb2) - max(qa1, qa2);
        ld sg = (qb1 + qb2) - (qa1 + qa2);
        return {wg, sg};
    };

    auto push_edge = [&](int e) {
        if (e == -1 || he[e].twin == -1) return;
        int rep = min(e, he[e].twin);
        gain[rep] = eval_gain(rep);
        auto [wg, sg] = gain[rep];
        if (wg > eps || (abs(wg) <= eps && sg > eps)) {
            pq.push({gain[rep], rep});
        }
    };

    for (int i = 0; i < n; i++) 
        if (he[i].twin != -1 && i < he[i].twin) 
            push_edge(i);

    int max_flips = n*n;
    int flips = 0;

    while (!pq.empty() && flips < max_flips) {
        auto [g, e] = pq.top(); pq.pop();
        if (g != gain[e]) continue;

        int t = he[e].twin;
        int en = he[e].next;
        int ep = he[e].prev;
        int tn = he[t].next;
        int tp = he[t].prev;

        flip_edge(e, he);
        flips++;

        gain[min(e, t)] = {-1, -1};
        push_edge(en);
        push_edge(ep);
        push_edge(tn);
        push_edge(tp);
    }

    return he;
}

template<class T>
vector<HalfEdge> optimize_interval_dp(vector<HalfEdge> he, const vector<pt<T>>& pts) {
    vector<int> poly_ids = clean_poly(pts);
    int n = sz(poly_ids);
    if (n < 3) return he;

    vector<pt<T>> poly(n);
    for (int i = 0; i < n; i++) poly[i] = pts[poly_ids[i]];

    auto in_cone = [&](int i, int j) -> bool {
        pt<T> pre = poly[(i-1+n)%n];
        pt<T> cur = poly[i];
        pt<T> nxt = poly[(i+1)%n];
        pt<T> target = poly[j];

        if (sgn(sarea2(pre, cur, nxt)) >= 0) {
            return ccw(cur, target, pre) && ccw(target, cur, nxt);
        }
        return !(sgn(sarea2(cur, target, nxt)) >= 0 &&
                 sgn(sarea2(target, cur, pre)) >= 0);
    };

    vector<vector<bool>> valid_diag(n, vector<bool>(n, false));
    for (int i = 0; i < n; i++) {
        valid_diag[i][(i+1)%n] = true;
        valid_diag[(i+1)%n][i] = true;
    }

    for (int i = 0; i < n; i++) {
        for (int j = i + 2; j < n; j++) {
            if (i == 0 && j == n - 1) continue;
            if (!in_cone(i, j) || !in_cone(j, i)) continue;

            bool ok = true;
            line<T> diag(poly[i], poly[j]);

            for (int k = 0; k < n; k++) {
                if (k != i && k != j && isinseg(poly[k], diag)) {
                    ok = false;
                    break;
                }
                int nk = (k+1)%n;
                if (k == i || k == j || nk == i || nk == j) continue;
                line<T> edge(poly[k], poly[nk]);
                if (interseg(diag, edge)) {
                    ok = false;
                    break;
                }
            }

            if (ok) {
                valid_diag[i][j] = true;
                valid_diag[j][i] = true;
            }
        }
    }

    vector<vector<pair<ld, ld>>> dp(n, vector<pair<ld, ld>>(n, {inf, inf}));
    vector<vector<int>> opt(n, vector<int>(n, -1));

    for (int i = 0; i + 1 < n; i++) {
        dp[i][i+1] = {0, 0};
    }

    for (int len = 2; len < n; len++) {
        for (int i = 0; i + len < n; i++) {
            int j = i + len;
            if (!valid_diag[i][j]) continue;

            for (int k = i+1; k < j; k++) {
                if (!valid_diag[i][k] || !valid_diag[k][j]) continue;
                if (!ccw(poly[i], poly[k], poly[j])) continue;
                if (dp[i][k].first >= inf || dp[k][j].first >= inf) continue;

                ld qt = qualidade(poly[i], poly[k], poly[j]);
                ld worst = max({dp[i][k].first, dp[k][j].first, qt});
                ld sum = dp[i][k].second + dp[k][j].second + qt;

                if (worst < dp[i][j].first - eps ||
                    (abs(worst - dp[i][j].first) <= eps && sum < dp[i][j].second - eps)) {
                    dp[i][j] = {worst, sum};
                    opt[i][j] = k;
                }
            }
        }
    }

    if (opt[0][n-1] == -1) return he;

    vector<array<int, 3>> triangulation;
    auto reconstruct = [&](auto& self, int i, int j) -> void {
        if (j-i < 2) return;
        int k = opt[i][j];
        if (k == -1) return;
        triangulation.push_back({poly_ids[i], poly_ids[k], poly_ids[j]});
        self(self, i, k);
        self(self, k, j);
    };

    reconstruct(reconstruct, 0, n-1);
    return build_he(triangulation);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <n>\n";
        return 1;
    }

    int n = atoi(argv[1]);
    if (n < 3) return 1;

    float min_c = -10000.0f;
    float max_c = 10000.0f;

    vector<pt<float>> pts = random_simple_polygon<float>(n, min_c, max_c);

    vector<HalfEdge> triangulation = triangulate(pts);
    optimize_steepest_climb(triangulation, pts);
    optimize_interval_dp(triangulation, pts);

    return 0;
}
