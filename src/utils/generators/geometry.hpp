#pragma once

#include "rng.hpp"
#include "../../geom/geom.hpp"

template<class T>
pt<T> random_pt(T min_c, T max_c) {
    if constexpr (is_floating_point_v<T>) {
        uniform_real_distribution<ld> dist(min_c, max_c);
        return {dist(rng), dist(rng)};
    }
    uniform_int_distribution<int> dist(min_c, max_c);
    return {dist(rng), dist(rng)};
}

template<class T>
line<T> random_line(T min_c, T max_c) {
    return {random_pt(min_c, max_c), random_pt(min_c, max_c)};
}

template<class T>
vector<pt<T>> random_simple_polygon(int n, T min_c, T max_c) {
    vector<pt<T>> pts(n);
    for (auto &p : pts) p = random_pt(min_c, max_c);
    ld cx = 0, cy = 0;
    for (auto p : pts) { cx += p.x; cy += p.y; }
    cx /= n; cy /= n;
    sort(pts.begin(), pts.end(), [cx,cy](const auto& a, const auto& b) {
        return atan2(a.y-cy, a.x-cx) < atan2(b.y-cy, b.x-cx);
    });
    return pts;
}