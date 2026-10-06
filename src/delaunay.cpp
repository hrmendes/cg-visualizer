#include "utils/AlgorithmRecorder.hpp"
#include "utils/generators/graph.hpp"
#include <bits/stdc++.h>
using namespace std;

#define int long long
using T = int;
#define pt pt<T>
#define sq(x) ((x)*(x))
#define vi vector<int>
#define vvi vector<vi>
#define all(x) x.begin(),x.end()

typedef struct QuadEdge* Q;
struct QuadEdge {
	int id;
	pt o;
	Q rot, nxt;
	bool used;
	QuadEdge(int id_ = -1, pt o_ = pt(inf, inf)) :
		id(id_), o(o_), rot(0), nxt(0), used(0) {}
	Q rev() const { return rot->rot; }
	Q next() const { return nxt; }
	Q prev() const { return rot->next()->rot; }
	pt dest() const { return rev()->o; }
};
Q edge(pt from, pt to, int id_from, int id_to) {
	Q e1 = new QuadEdge(id_from, from);
	Q e2 = new QuadEdge(id_to, to);
	Q e3 = new QuadEdge;
	Q e4 = new QuadEdge;
	tie(e1->rot, e2->rot, e3->rot, e4->rot) = {e3, e4, e2, e1};
	tie(e1->nxt, e2->nxt, e3->nxt, e4->nxt) = {e1, e2, e4, e3};
	return e1;
}
void splice(Q a, Q b) {
	swap(a->nxt->rot->nxt, b->nxt->rot->nxt);
	swap(a->nxt, b->nxt);
}
void del_edge(Q& e, Q ne) {
	splice(e, e->prev());
	splice(e->rev(), e->rev()->prev());
	delete e->rev()->rot, delete e->rev();
	delete e->rot; delete e;
	e = ne;
}
Q conn(Q a, Q b) {
	Q e = edge(a->dest(), b->o, a->rev()->id, b->id);
	splice(e, a->rev()->prev());
	splice(e->rev(), b);
	return e;
}
bool in_c(pt a, pt b, pt c, pt p) {
	__int128 p2 = p*p, A = a*a - p2, B = b*b - p2, C = c*c - p2;
	return sarea2(p, a, b) * C + sarea2(p, b, c) * A + sarea2(p, c, a) * B > 0;
}
pair<Q, Q> build_tr(vector<pt>& p, int l, int r) {
	if (r-l+1 <= 3) {
		Q a = edge(p[l], p[l+1], l, l+1), b = edge(p[l+1], p[r], l+1, r);
		if (r-l+1 == 2) return {a, a->rev()};
		splice(a->rev(), b);
		auto ar = sarea2(p[l], p[l+1], p[r]);
		Q c = ar ? conn(b, a) : 0;
		if (ar >= 0) return {a, b->rev()};
		return {c->rev(), c};
	}
	int m = (l+r)/2;
	auto [la, ra] = build_tr(p, l, m);
	auto [lb, rb] = build_tr(p, m+1, r);
	while (true) {
		if (ccw(lb->o, ra->o, ra->dest())) ra = ra->rev()->prev();
		else if (ccw(lb->o, ra->o, lb->dest())) lb = lb->rev()->next();
		else break;
	}
	Q b = conn(lb->rev(), ra);
	auto valid = [&](Q e) { return ccw(e->dest(), b->dest(), b->o); };
	if (ra->o == la->o) la = b->rev();
	if (lb->o == rb->o) rb = b;
	while (true) {
		Q L = b->rev()->next();
		if (valid(L)) while (in_c(b->dest(), b->o, L->dest(), L->next()->dest()))
			del_edge(L, L->next());
		Q R = b->prev();
		if (valid(R)) while (in_c(b->dest(), b->o, R->dest(), R->prev()->dest()))
			del_edge(R, R->prev());
		if (!valid(L) and !valid(R)) break;
		if (!valid(L) or (valid(R) and in_c(L->dest(), L->o, R->o, R->dest())))
			b = conn(R, b->rev());
		else b = conn(b->rev(), L->rev());
	}
	return {la, rb};
}
vvi delaunay(vector<pt> v) {
	int n = v.size();
	auto tmp = v;
	vi ids(n);
	iota(all(ids), 0);
	sort(all(ids), [&](int l, int r) { return v[l] < v[r]; });
	for (int i = 0; i < n; i++) v[i] = tmp[ids[i]];
	assert(unique(all(v)) == v.end());
	vvi adj(n);
	bool col = true;
	for (int i = 2; i < n; i++) 
        if (sarea2(v[i], v[i-1], v[i-2])) col = false;
	if (col) { // degenerate case where all pts are in a line
		for (int i = 1; i < n; i++) {
			adj[ids[i-1]].push_back(ids[i]);
            adj[ids[i]].push_back(ids[i-1]);
        }
		return adj;
	}
	Q e = build_tr(v, 0, n-1).first;
	vector<Q> edg = {e};
	for (int i = 0; i < edg.size(); e = edg[i++]) {
		for (Q at = e; !at->used; at = at->next()) {
			at->used = true;
			adj[ids[at->id]].push_back(ids[at->rev()->id]);
			edg.push_back(at->rev());
		}
	}
	return adj;
}

signed main() {
    cout << "Number of points: ";
    int n; cin >> n;
    int mn = -100, mx = 100;
    vector<pt> poly = random_simple_polygon(n, 9*mn/10, 9*mx/10);

    AlgorithmRecorder recorder(mn,mx,mn,mx);

    auto adj = delaunay(poly);

    auto draw_pt = [&](pt p, glm::vec4 color) {
        recorder.record_circle(p, 0.003*(mx-mn), color);
    };
    auto draw_base = [&]() -> void {
        recorder.record_polygon(poly, {.5,.5,.5,.5}, BLACK, BLACK);
        for (auto p : poly) draw_pt(p,BLACK);
        for (int i = 0; i < adj.size(); i++){
            for (int j : adj[i])
            recorder.record_line(poly[i], poly[j], RED);
        }
    };

    draw_base();
    recorder.commit_step();

    recorder.run(500);
    return 0;
}
