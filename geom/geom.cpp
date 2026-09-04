#include "geom.hpp"

#define sz(x) ((int)x.size())

using ld = long double;
using T = long long; // change to ld if necessary
const ld DINF = 2e18;
const ld pi = acosl(-1.0);
const ld eps = 1e-9;
#define sq(x) ((x)*(x))
// -1 for negative, 0 for zero, 1 for positive
int sgn(T x) {
    if constexpr (is_floating_point_v<T>) 
        return (x > eps) - (x < -eps);
    return (x > 0) - (x < 0);
}

ld dist(pt p, pt q) { 
    return hypot((ld)(p.y - q.y), (ld)(p.x - q.x)); 
}

T dist2(pt p, pt q) {return sq(p.x-q.x) + sq(p.y-q.y);}

ld norm(pt v) { return dist(pt(0, 0), v); }

// Angle with +x axis in [0, 2*pi)
ld angle(pt v) { 
    ld ang = atan2((ld)v.y, (ld)v.x); 
    return ang < 0 ? ang + 2*pi : ang; 
}

// 2x signed area of triangle p-q-r (>0 if ccw)
T sarea2(pt p, pt q, pt r) { 
    return (q-p)^(r-q); 
}

// True if p, q, r are collinear
bool col(pt p,pt q,pt r){
    return sgn(sarea2(p,q,r))==0;
}

// True if r is strictly to the left of line p->q
bool ccw(pt p,pt q,pt r) {return sgn(sarea2(p,q,r))>0;}

bool isvertical(line r) {return sgn(r.p.x-r.q.x) == 0;}

// True if point p lies on segment r
bool isinseg(pt p, line r) {
    pt a = r.p - p, b = r.q - p;
    return sgn(a ^ b) == 0 && sgn(a * b) <= 0;
}

// True if line segments intersect
bool interseg(line r, line s) {
    if (isinseg(r.p, s) || isinseg(r.q, s) || 
            isinseg(s.p, r) || isinseg(s.q, r)) return 1;
    return ccw(r.p, r.q, s.p) != ccw(r.p, r.q, s.q) &&
                 ccw(s.p, s.q, r.p) != ccw(s.p, s.q, r.q);
}

ld disttoline(pt p, line r) { 
    return (ld)abs(sarea2(p, r.p, r.q)) / dist(r.p, r.q); 
}

ld disttoseg(pt p, line r) {
    if (sgn((r.q-r.p)*(p-r.p)) < 0) return dist(r.p,p);
    if (sgn((r.p-r.q)*(p-r.q)) < 0) return dist(r.q,p);
    return disttoline(p, r);
}

bool in_tri(pt p, pt a, pt b, pt c) {
    bool b1 = sgn(sarea2(p, a, b)) < 0;
    bool b2 = sgn(sarea2(p, b, c)) < 0;
    bool b3 = sgn(sarea2(p, c, a)) < 0;
    return (b1 == b2) && (b2 == b3);
}

mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
pt random_pt(int min_c, int max_c) {
    uniform_int_distribution<int> dist(min_c, max_c);
    return {dist(rng), dist(rng)};
}

vector<pt> generate_random_polygon(int n, int min_c, int max_c) {
    vector<pt> pts(n);
    for (int i = 0; i < n; ++i) pts[i] = random_pt(min_c, max_c);
    ld cx = 0, cy = 0;
    for (auto p : pts) { cx += p.x; cy += p.y; }
    cx /= n; cy /= n;
    sort(pts.begin(), pts.end(), [cx,cy](const pt& a, const pt& b) {
        return atan2(a.y-cy, a.x-cx) < atan2(b.y-cy, b.x-cx);
    });
    return pts;
}