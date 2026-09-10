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
    T area = 0;
    for (int i = 0; i < n; i++) area += poly[i]^poly[(i+1)%n];
    if (area < 0) reverse(poly.begin(), poly.end());

    vector<int> prev(n), next(n);

    for (int i = 0; i < n; i++) {
        prev[i] = (i+n-1) % n;
        next[i] = (i+1) % n;
    }

    vector<bool> is_ear(n, false);
    vector<int> ears;
    ears.reserve(n);

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
            // avoid cross products when outside bounding box
            if (sgn(poly[idx].x - min_x) == 1 &&
                sgn(poly[idx].x - max_x) ==-1 &&
                sgn(poly[idx].y - min_y) == 1 &&
                sgn(poly[idx].y - max_y) ==-1
            ) {
                if (
                    in_tri(poly[idx], poly[pre], poly[cur], poly[nxt])
                ) return false;
            }
            idx = next[idx];
        }

        return true;
    };

    for (int i = 0; i < n; i++) {
        if (check_ear(i)) {
            is_ear[i] = true;
            ears.push_back(i);
        }
    }

    int remaining = n;
    while (remaining > 3) {
        int cur = ears.back(); 
        ears.pop_back();
        while (!is_ear[cur]) {
            cur = ears.back();
            ears.pop_back();
        }
        int pre = prev[cur];
        int nxt = next[cur];

        ans.push_back({
            poly[pre],
            poly[cur],
            poly[nxt]
        });

        is_ear[cur] = false;
        next[pre] = nxt;
        prev[nxt] = pre;

        remaining--;

        if (check_ear(pre)) {
            if (!is_ear[pre]) ears.push_back(pre);
            is_ear[pre] = true;
        } else is_ear[pre] = false;

        if (check_ear(nxt)) {
            if (!is_ear[nxt]) ears.push_back(nxt);
            is_ear[nxt] = true;
        } else is_ear[nxt] = false;
    }

    int a = ears.empty() ? 0 : ears.back();
    int b = next[a];
    int c = next[b];

    ans.push_back({poly[a], poly[b], poly[c]});

    return ans;
}


// O(n^3) version of ears clipping triangulation
template<class T>
vector<tri<T>> triangulate_deprecated(vector<pt<T>> poly) {
    vector<tri<T>> ans;
    int n = sz(poly);
    if (n < 3) return ans;

    // precisa q seja ccw
    T signed_area = 0;
    for (int i = 0; i < n; i++) {
        signed_area += poly[i] ^ poly[(i + 1) % n];
    }
    if (signed_area < 0) {
        reverse(poly.begin(), poly.end());
    }

    vector<int> id(n);
    iota(id.begin(), id.end(), 0);
    
    auto check_ear = [&](int i) -> bool {
        int m = sz(id);
        int pre = id[(i-1+m) % m];
        int cur = id[i];
        int nxt = id[(i+1) % m];
        if (!ccw(poly[pre], poly[cur], poly[nxt])) return false;
        for (int j = 0; j < m; j++) {
            int idx = id[j];
            if (idx == pre || idx == cur || idx == nxt) continue;
            if (in_tri(poly[idx], poly[pre], poly[cur], poly[nxt])) {
                return false;
            }
        }
        return true;
    };

    while (sz(id) > 3) {
        int m = sz(id);
        for (int i = 0; i < m; i++){
            if (!check_ear(i)) continue;
            int pre = id[(i-1+m) % m];
            int cur = id[i];
            int nxt = id[(i+1) % m];
            ans.push_back({poly[pre], poly[cur], poly[nxt]});
            id.erase(id.begin()+i);
            break;
        }
    }
    if (sz(id) == 3){
        ans.push_back({poly[id[0]], poly[id[1]], poly[id[2]]});
    }
    return ans;
}

