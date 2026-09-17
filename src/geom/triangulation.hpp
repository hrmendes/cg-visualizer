#pragma once
#include "geom.hpp"

template<class T = float>
struct tri {
    pt<T> p1,p2,p3;
};

template<class T>
vector<tri<T>> triangulate(vector<pt<T>> poly){
    vector<tri<T>> ans;
    int n = poly.size();
    if (n < 3) return ans;

    // Ensure CCW
    ld area = 0;
    for (int i = 0; i < n; i++) area += poly[i] ^ poly[(i + 1) % n];
    if (area < 0) reverse(poly.begin(), poly.end());

    vector<pt<T>> clean_poly;
    for (const auto& p : poly) {
        while (clean_poly.size() >= 2 &&
               col(clean_poly[clean_poly.size() - 2], clean_poly.back(), p)) {
            clean_poly.pop_back();
        }
        clean_poly.push_back(p);
    }
    while (clean_poly.size() >= 3 &&
           col(clean_poly[clean_poly.size() - 2], clean_poly.back(), clean_poly[0])) {
        clean_poly.pop_back();
    }
    while (clean_poly.size() >= 3 &&
           col(clean_poly.back(), clean_poly[0], clean_poly[1])) {
        clean_poly.erase(clean_poly.begin());
    }

    poly = move(clean_poly);
    n = poly.size();
    if (n < 3) return ans;

    vector<int> prev(n), next(n);
    for (int i = 0; i < n; i++) {
        prev[i] = (i - 1 + n) % n;
        next[i] = (i + 1) % n;
    }

    auto check_ear = [&](int cur) -> bool {
        int pre = prev[cur];
        int nxt = next[cur];
        if (!ccw(poly[pre], poly[cur], poly[nxt])) return false;

        T min_x = min({poly[pre].x, poly[cur].x, poly[nxt].x});
        T max_x = max({poly[pre].x, poly[cur].x, poly[nxt].x});
        T min_y = min({poly[pre].y, poly[cur].y, poly[nxt].y});
        T max_y = max({poly[pre].y, poly[cur].y, poly[nxt].y});

        int idx = next[nxt];
        while (idx != pre) {
            if (poly[idx].x >= min_x && poly[idx].x <= max_x &&
                poly[idx].y >= min_y && poly[idx].y <= max_y) {
                if (in_tri(poly[idx], poly[pre], poly[cur], poly[nxt])) return false;
            }
            idx = next[idx];
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

    int remaining = n;
    int last_alive = 0;

    while (remaining > 3) {
        while (!ears.empty() && !is_ear[ears.back()]) {
            ears.pop_back();
        }
        if (ears.empty()) break;

        int cur = ears.back();
        ears.pop_back();

        int pre = prev[cur];
        int nxt = next[cur];

        ans.push_back({poly[pre], poly[cur], poly[nxt]});

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
        ans.push_back({poly[a], poly[b], poly[c]});
    }

    // ensure all triangles are ccw
    for (auto &[p1,p2,p3] : ans){
        if (!ccw(p1,p2,p3)) swap(p1,p2);
    }
    return ans;
}

