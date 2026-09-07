#include "geometry.hpp"

pt random_pt(ld min_c, ld max_c) {
    uniform_real_distribution<ld> dist(min_c, max_c);
    return {dist(rng), dist(rng)};
}

vector<pt> random_simple_polygon(int n, ld min_c, ld max_c) {
    vector<pt> pts(n);
    for (pt &p : pts) p = random_pt(min_c, max_c);
    ld cx = 0, cy = 0;
    for (auto p : pts) { cx += p.x; cy += p.y; }
    cx /= n; cy /= n;
    sort(pts.begin(), pts.end(), [cx,cy](const pt& a, const pt& b) {
        return atan2(a.y-cy, a.x-cx) < atan2(b.y-cy, b.x-cx);
    });
    return pts;
}