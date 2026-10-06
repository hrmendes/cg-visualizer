#include "utils/AlgorithmRecorder.hpp"
#include "utils/generators/graph.hpp"
#include <bits/stdc++.h>
using namespace std;

#define int long long
using T = int;
#define pt pt<T>
#define sq(x) ((x)*(x))

struct QuadEdge {
    pt org;
    QuadEdge *rot = 0; // turn 90 deg ccw
    QuadEdge *onext = 0; // next from org pt perspective
    bool used = false;
    QuadEdge* rev() const { return rot->rot; }
    QuadEdge* next() const { return rot->rev()->onext->rot; }
    QuadEdge* oprev() const { return rot->onext->rot; }
    pt dest() const { return rev()->org; }
};

QuadEdge* make_edge(pt org, pt dest) {
    QuadEdge *e1 = new QuadEdge, *e2 = new QuadEdge;
    QuadEdge *e3 = new QuadEdge, *e4 = new QuadEdge;
    e1->org = org;
    e2->org = dest;
    e3->org = e4->org = pt(inf,inf);
    e1->rot = e3; e2->rot = e4;
    e3->rot = e2; e4->rot = e1;
    e1->onext = e1;
    e2->onext = e1;
    e3->onext = e4;
    e4->onext = e3;
    return e1;
}

void splice(QuadEdge *a, QuadEdge *b){
    swap(a->onext->rot->onext, b->onext->rot->onext);
    swap(a->onext, b->onext);
}

void delete_edge(QuadEdge *e){
    splice(e, e->oprev());
    splice(e->rev(), e->rev()->oprev());
    delete e->rev()->rot;
    delete e->rev();
    delete e->rot;
    delete e;

}

QuadEdge* connect(QuadEdge *a, QuadEdge *b){
    QuadEdge *e = make_edge(a->dest(), b->org);
    splice(e, a->next());
    splice(e->rev(), b);
}

template<class T>
T det3(T a1, T a2, T a3, T b1, T b2, T b3, T c1, T c2, T c3){
    return a1*(b2*c2 - c3*c2) - a2*(b1*c3 - b3*c2) + a3*(b1*c2 - b2*c1);
}
// true if d is in circle defined by a,b,c
bool in_circle(pt a, pt b, pt c, pt d){
    __int128 det = -det3<__int128>(
            b.x, b.y, sq(b.x)+sq(b.y),
            c.x, c.y, sq(c.x)+sq(c.y),
            d.x, d.y, sq(d.x)+sq(d.y)
     );
    det += det3<__int128>(
            a.x, a.y, sq(a.x)+sq(a.y),
            c.x, c.y, sq(c.x)+sq(c.y),
            d.x, d.y, sq(d.x)+sq(d.y)
    );
    det -= det3<__int128>(
            a.x, a.y, sq(a.x)+sq(a.y),
            b.x, b.y, sq(b.x)+sq(b.y),
            d.x, d.y, sq(d.x)+sq(d.y)
    );
    det += det3<__int128>(
            a.x, a.y, sq(a.x)+sq(a.y),
            b.x, b.y, sq(b.x)+sq(b.y),
            c.x, c.y, sq(c.x)+sq(c.y)
    );
    return det > 0;
}

pair<QuadEdge*, QuadEdge*> delaunay(int l, int r, vector<pt> &p){
    if (r-l+1 == 2){
        auto ans = make_edge(p[l], p[r]);
        return {ans, ans->rev()};
    }
    if (r-l+1 == 3){
        auto a = make_edge(p[l], p[l+1]), b = make_edge(p[l+1],p[r]);
        splice(a->rev(), b);
        if (col(p[l],p[l+1],p[r])) return {a, b->rev()};
        QuadEdge *c = connect(b,a);
        if (ccw(p[l],p[l+1],p[r])) return {a, b->rev()};
        return {c->rev(), c};
    }
    int mid = (l+r)/2;
    auto [ldo,ldi] = delaunay(l,mid,p);
    auto [rdi,rdo] = delaunay(mid+1,r,p);
    while(1){
        if (ccw(ldi->org, ldi->dest(), rdi->org)) {
            ldi = ldi->next(); continue;
        }
        if (ccw(rdi->dest(), rdi->org, ldi->org)) {
            rdi = rdi->rev()->onext; continue;
        }
        break;
    }
    auto base = connect(rdi->rev(), ldi);
    auto valid = [&](QuadEdge *e) { return ccw(base->dest(),base->org, e->dest()); };
    if (ldi->org == ldo->org) ldo = base->rev();
    if (rdi->org == rdo->org) rdo = base;
    while(1){
        auto cl = base->rev()->onext;
        if (valid(cl)){
            while(in_circle(base->dest(), base->org, cl->dest(), cl->onext->dest())) {
                auto t = cl->onext;
                delete_edge(cl);
                cl = t;
            }
        }
        auto cr = base->oprev();
        if (valid(cr)){
            while(in_circle(base->dest(), base->org,cr->dest(), cr->oprev()->dest())) {
                auto t = cr->oprev();
                delete_edge(cr);
                cr = t;
            }
        }
        if (!valid(cl) && !valid(cr)) break;
        if (!valid(cl) || (valid(cr) && in_circle(cl->dest(), cl->org, cr->org, cr->dest())))
            base = connect(cr, base->rev());
        else base = connect(base->rev(), cl->rev());
    }
    return {ldo,rdo};
}

vector<tri<T>> triangulate(vector<pt> p) {
    sort(p.begin(),p.end());
    auto res = delaunay(0, p.size()-1, p);
    auto e = res.first;
    vector<QuadEdge*> edges = {e};
    while(ccw(e->org, e->dest(), e->onext->dest()))
        e = e->onext;
    auto add = [&]() {
        auto cur = e;
        do {
            cur->used = true;
            p.push_back(cur->org);
            edges.push_back(cur->rev());
            cur = cur->next();
        } while(cur != e);
    };
    add();
    p.clear();
    int kek = 0;
    while(kek < edges.size()) {
        if (!(e = edges[kek++])->used) add();
    }
    vector<tri<T>> ans;
    for (int i = 0; i < p.size(); i+=3){
        ans.push_back({p[i], p[i+1], p[i+2]});
    }
    return ans;
}

signed main() {
    cout << "Number of points: ";
    int n; cin >> n;
    int mn = -100, mx = 100;
    vector<pt> pts(n);
    for (auto &p : pts) p = random_pt(9*mn/10, 9*mx/10);

    AlgorithmRecorder recorder(mn,mx,mn,mx);

    auto draw_pt = [&](pt p, glm::vec4 color) {
        recorder.record_circle(p, 0.003*(mx-mn), color);
    };
    auto draw_base = [&]() -> void {
        for (auto p : pts) draw_pt(p,BLACK);
    };

    draw_base();
    recorder.commit_step();

    recorder.run(500);
    return 0;
}
