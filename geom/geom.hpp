#pragma once
#include <bits/stdc++.h>
using namespace std;

#define sz(x) ((int)x.size())

using ld = long double;
using T = long long; // change to ld if necessary

extern const ld DINF;
extern const ld pi;
extern const ld eps;

#define sq(x) ((x)*(x))
// -1 for negative, 0 for zero, 1 for positive
int sgn(T x);

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

struct line {
    pt p, q;
    line() {}
    line(pt p_, pt q_) : p(p_), q(q_) {}
};

ld dist(pt p, pt q);

T dist2(pt p, pt q);

ld norm(pt v);

// Angle with +x axis in [0, 2*pi)
ld angle(pt v);

// 2x signed area of triangle p-q-r (>0 if ccw)
T sarea2(pt p, pt q, pt r);

// True if p, q, r are collinear
bool col(pt p,pt q,pt r);

// True if r is strictly to the left of line p->q
bool ccw(pt p,pt q,pt r);

bool isvertical(line r);

// True if point p lies on segment r
bool isinseg(pt p, line r);

// True if line segments intersect
bool interseg(line r, line s);

ld disttoline(pt p, line r);

ld disttoseg(pt p, line r);

struct tri { pt a, b, c; };

bool in_tri(pt p, pt a, pt b, pt c);

extern mt19937 rng;
pt random_pt(int min_c, int max_c);

vector<pt> generate_random_polygon(int n, int min_c, int max_c);