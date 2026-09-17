#pragma once

#include "rng.hpp"
#include "primitives.hpp"
#include "../../geom/geom.hpp"
#include <cmath>
#include <vector>
#include <algorithm>

template<class T>
pt<T> random_pt(T min_c, T max_c) {
    if constexpr (is_floating_point_v<T>) {
        uniform_real_distribution<ld> dist(min_c, max_c);
        return {dist(rng), dist(rng)};
    }
    uniform_int_distribution<long long> dist(min_c, max_c);
    return {dist(rng), dist(rng)};
}

template<class T>
line<T> random_line(T min_c, T max_c) {
    return {random_pt(min_c, max_c), random_pt(min_c, max_c)};
}

// 1. Angular sorted around centroid (mostly convex / star-shaped)
template<class T>
vector<pt<T>> random_angular_polygon(int n, T min_c, T max_c) {
    vector<pt<T>> pts(n);
    for (auto &p : pts) p = random_pt(min_c, max_c);
    ld cx = 0, cy = 0;
    for (auto p : pts) { cx += p.x; cy += p.y; }
    cx /= n; cy /= n;
    sort(pts.begin(), pts.end(), [cx, cy](const auto& a, const auto& b) {
        return atan2(a.y - cy, a.x - cx) < atan2(b.y - cy, b.x - cx);
    });
    return pts;
}

// 2. Comb / Sawtooth
template<class T>
vector<pt<T>> random_comb_polygon(int n, T min_c, T max_c) {
    if (n < 4) return random_angular_polygon(n, min_c, max_c);

    vector<pt<T>> pts;
    pts.reserve(n);

    T width = max_c - min_c;
    T height = max_c - min_c;

    pts.push_back({min_c+0.05*width, min_c + 0.1*height});
    pts.push_back({max_c-0.05*width, min_c + 0.1*height});

    int remaining = n - 2;
    int teeth = remaining/2;
    ld dx = (ld)0.9*width / (teeth+1);

    while(teeth--){
        ld xr = (ld)min_c+0.05*width + (teeth+1)*dx + dx*0.3;
        ld xl = (ld)min_c+0.05*width + (teeth+1)*dx - dx*0.3;
        pts.push_back({(T)xr, (T)(min_c + height * random_float(0.85, 0.98))});
        pts.push_back({(T)xl, (T)(min_c + height * random_float(0.15, 0.30))});
    }

    while ((int)pts.size() < n) {
        pts.push_back({(T)(min_c + width * 0.05), (T)(min_c + height * 0.15)});
    }

    return pts;
}

// 3. Spiked Star 
template<class T>
vector<pt<T>> random_spiked_star_polygon(int n, T min_c, T max_c) {
    vector<pt<T>> pts(n);
    ld cx = (ld)(min_c + max_c) / 2.0;
    ld cy = (ld)(min_c + max_c) / 2.0;
    ld max_r = ((ld)(max_c - min_c) / 2.0) * 0.95;
    ld min_r = max_r * 0.25;

    ld step = (2.0 * M_PI) / n;
    for (int i = 0; i < n; i++) {
        ld theta = i*step;
        ld r = (i%2 == 0) ? max_r*random_float(0.5, 1.0) : min_r*random_float(0.5,1.0);
        pts[i] = {
            (T)(cx + r*cos(theta)),
            (T)(cy + r*sin(theta))
        };
    }
    return pts;
}

// 4. Inward/Outward Spiral
template<class T>
vector<pt<T>> random_spiral_polygon(int n, T min_c, T max_c) {
    if (n < 4) return random_angular_polygon(n, min_c, max_c);

    vector<pt<T>> pts;
    pts.reserve(n);

    ld cx = (ld)(min_c + max_c) / 2.0;
    ld cy = (ld)(min_c + max_c) / 2.0;
    ld max_span = ((ld)(max_c - min_c) / 2.0) * 0.90;

    int m = n / 2;
    int k = n - m;

    ld turns = max((ld)1.5, min((ld)5.0, (ld)n / 15.0));
    ld max_u = turns * 2.0 * M_PI;
    ld min_u = 3.0;
    ld c = max_span / max_u;

    ld corridor = c * M_PI * 0.9;

    auto f = [&](ld u, ld w) -> pt<T> {
        ld offset = corridor * w * random_float(0.05, 0.20);
        if (rng() % 2) offset = corridor * w * random_float(0.65, 0.95);
        ld r = c * u + offset;
        return {
            (T)(cx + r * cos(u)),
            (T)(cy + r * sin(u))
        };
    };

    auto g = [&](ld u, ld w) -> pt<T> {
        ld offset = corridor * w * random_float(0.05, 0.20);
        if (rng() % 2) offset = corridor * w * random_float(0.65, 0.95);
        offset = min(offset, c * u * 0.70);
        ld r = c * u - offset;
        return {
            (T)(cx + r * cos(u)),
            (T)(cy + r * sin(u))
        };
    };

    auto sample_u = [&](int count) {
        vector<ld> u(count);
        u[0] = 0;
        for (int i = 1; i < count; i++) {
            u[i] = u[i - 1] + random_float(0.6, 1.4);
        }
        ld span = u.back();
        for (int i = 0; i < count; i++) {
            u[i] = min_u + (u[i] / span) * (max_u - min_u);
        }
        return u;
    };

    int pts_per_arm = n / 2;
    vector<ld> u_vals = sample_u(pts_per_arm);

    for (int i = 0; i < pts_per_arm; i++) {
        ld w = (i == 0 || i == pts_per_arm - 1) ? 0.10 : 1.0;
        pts.push_back(f(u_vals[i], w));
    }

    for (int i = pts_per_arm - 1; i >= 0; i--) {
        ld w = (i == 0 || i == pts_per_arm - 1) ? 0.10 : 1.0;
        pts.push_back(g(u_vals[i], w));
    }

    if ((int)pts.size() < n) {
        pts.push_back(g(u_vals[0], 0.10));
    }

    return pts;
}

// Selects randomly among all strategies
template<class T>
vector<pt<T>> random_simple_polygon(int n, T min_c, T max_c) {
    uniform_int_distribution<int> dist(0, 3);
    int type = dist(rng);

    switch (type) {
        case 0: return random_angular_polygon(n, min_c, max_c);
        case 1: return random_comb_polygon(n, min_c, max_c);
        case 2: return random_spiked_star_polygon(n, min_c, max_c);
        case 3: return random_spiral_polygon(n, min_c, max_c);
        default: return random_angular_polygon(n, min_c, max_c);
    }
}
