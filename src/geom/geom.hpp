#pragma once
#include <bits/stdc++.h>
using namespace std;

#define sz(x) ((int)x.size())
#define sq(x) ((x)*(x))

using ld = long double;
const ld DINF = 2e18;
const ld pi = acosl(-1.0);
const ld eps = 1e-9;


// -1 for negative, 0 for zero, 1 for positive
template<class T>
int sgn(T x);

template<class T = float>
struct pt {
    T x, y;
    pt(T x_ = 0, T y_ = 0) : x(x_), y(y_) {}
    bool operator<(const pt p) const {
        if (sgn(x-p.x) != 0) return sgn(x-p.x) < 0;
        return sgn(y-p.y) < 0;
    }
    bool operator==(const pt p) const { 
        return sgn(x-p.x) == 0 && sgn(y-p.y) == 0; 
    }
    pt operator+(pt p) const {return pt(x+p.x, y+p.y);}
    pt operator-(pt p) const {return pt(x-p.x, y-p.y);}
    pt operator*(T c) const {return pt(x*c, y*c);}
    pt operator/(T c) const {return pt(x/c, y/c);}
    T operator*(pt p) const {return x*p.x + y*p.y;}
    T operator^(pt p) const {return x*p.y - y*p.x;}
};

template<class T = float>
struct line {
    pt<T> p, q;
    line() {}
    line(pt<T> p_, pt<T> q_) : p(p_), q(q_) {}
};



// -1 for negative, 0 for zero, 1 for positive
template<class T>
int sgn(T x) {
    if constexpr (is_floating_point_v<T>) 
        return (x > eps) - (x < -eps);
    return (x > 0) - (x < 0);
}

template<class T>
ld dist(pt<T> p, pt<T> q) { 
    return hypot((ld)(p.y - q.y), (ld)(p.x - q.x)); 
}

template<class T>
T dist2(pt<T> p, pt<T> q) {return sq(p.x-q.x) + sq(p.y-q.y);}

template<class T>
ld norm(pt<T> v) { return dist(pt(0, 0), v); }

// Angle with +x axis in [0, 2*pi)
template<class T>
ld angle(pt<T> v) { 
    ld ang = atan2((ld)v.y, (ld)v.x); 
    return ang < 0 ? ang + 2*pi : ang; 
}

// 2x signed area of triangle p-q-r (>0 if ccw)
template<class T>
T sarea2(pt<T> p, pt<T> q, pt<T> r) { 
    return (q-p)^(r-q); 
}

// True if p, q, r are collinear
template<class T>
bool col(pt<T> p,pt<T> q,pt<T> r) {
    return sgn(sarea2(p,q,r))==0;
}

// True if r is strictly to the left of line<T> p->q
template<class T>
bool ccw(pt<T> p,pt<T> q,pt<T> r) {return sgn(sarea2(p,q,r))>0;}

template<class T>
bool isvertical(line<T> r) {return sgn(r.p.x-r.q.x) == 0;}

// True if point p lies on segment r
template<class T>
bool isinseg(pt<T> p, line<T> r) {
    pt<T> a = r.p - p, b = r.q - p;
    return sgn(a ^ b) == 0 && sgn(a * b) <= 0;
}

// True if line<T> segments intersect
template<class T>
bool interseg(line<T> r, line<T> s) {
    if (isinseg(r.p, s) || isinseg(r.q, s) || 
            isinseg(s.p, r) || isinseg(s.q, r)) return 1;
    return ccw(r.p, r.q, s.p) != ccw(r.p, r.q, s.q) &&
                 ccw(s.p, s.q, r.p) != ccw(s.p, s.q, r.q);
}

template<class T>
ld disttoline(pt<T> p, line<T> r) { 
    return (ld)abs(sarea2(p, r.p, r.q)) / dist(r.p, r.q); 
}

template<class T>
ld disttoseg(pt<T> p, line<T> r) {
    if (sgn((r.q-r.p)*(p-r.p)) < 0) return dist(r.p,p);
    if (sgn((r.p-r.q)*(p-r.q)) < 0) return dist(r.q,p);
    return disttoline(p, r);
}

template<class T>
bool in_tri(pt<T> p, pt<T> a, pt<T> b, pt<T> c) {
    bool b1 = sgn(sarea2(p, a, b)) < 0;
    bool b2 = sgn(sarea2(p, b, c)) < 0;
    bool b3 = sgn(sarea2(p, c, a)) < 0;
    return (b1 == b2) && (b2 == b3);
}

template<class T>
vector<pt<T>> get_circle_polygon(pt<T> center, ld radius, int segments = 20){
    vector<pt<T>> circle(segments);
    for (int s = 0; s < segments; s++) {
        ld ang = 2*M_PI * s/segments;
        circle[s] = pt<T>(center.x + radius*cos(ang), center.y + radius*sin(ang));
    }
    return circle;
}

template<class T>
T spolarea2(const vector<pt<T>> &v) {
    T ans = 0;
    for (int i = 0; i < sz(v); i++) {
        ans += (v[i] ^ v[(i+1)%sz(v)]);
    }

    return ans;
}

template<class T>
int inpol(const vector<pt<T>> &v, pt<T> p) {
    int qt = 0;
    for (int i = 0; i < sz(v); i++) {
        if (p == v[i]) return 2;
        int j = (i+1)%sz(v);
        if (sgn(p.y-v[i].y)==0 && sgn(p.y-v[j].y)==0) {
            if (sgn((v[i]-p)*(v[j]-p)) <= 0) return 2;
            continue;
        }
        bool lower = sgn(v[i].y - p.y) < 0;
        if (lower == (sgn(v[j].y - p.y) < 0)) continue;
        auto t = (p-v[i])^(v[j]-v[i]);
        if (sgn(t) == 0) return 2;
        if (lower == (sgn(t) > 0)) qt += lower ? 1 : -1;
    }
    return qt != 0;
}
