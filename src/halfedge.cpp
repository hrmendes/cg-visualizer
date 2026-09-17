#include "geom/geom.hpp"
#include "utils/AlgorithmRecorder.hpp"
#include "geom/triangulation.hpp"
#include "utils/generators/geometry.hpp"

template<class T>
struct HalfEdge {
    pt<T> source;
    HalfEdge *next, *prev, *twin;
    HalfEdge (pt<T> p, HalfEdge *nxt = nullptr, HalfEdge *pre = nullptr, HalfEdge *tw = nullptr) {
        source = p;
        next = nxt;
        prev = pre;
        twin = tw;
    }
};

using T = int;

int main() {
    int mn = -1000, mx = 1000;
    int width = mx-mn;
    AlgorithmRecorder recorder(mn, mx, mn, mx, 1000, 1000);

    int n = 10;

    auto poly = random_simple_polygon(n,mn,mx);
    // auto poly = regular_polygon<T>(n,mn,mx);
    auto triangulation = triangulate(poly);

    recorder.record_polygon(poly, {0.8,0.8,0.8,0.4});
    recorder.commit_step();

    recorder.record_polygon(poly, {0.8,0.8,0.8,0.4});
    for (auto [p1,p2,p3] : triangulation){
        recorder.record_line(p1,p2,BLUE);
        recorder.record_line(p2,p3,BLUE);
        recorder.record_line(p3,p1,BLUE);
    }
    recorder.commit_step();

    map<pair<pt<T>, pt<T>>, HalfEdge<T>*> mp;
    auto link = [&](const pt<T>& u, const pt<T>& v, HalfEdge<T>* he) {
        auto it = mp.find({v, u});
        if (it != mp.end()) {
            he->twin = it->second;
            it->second->twin = he;
        } else {
            mp[{u, v}] = he;
        }
    };

    vector<HalfEdge<T>*> halfedges;
    halfedges.reserve(3*(n-2)/2 + n);

    map<pt<T>, HalfEdge<T>*> pt_out_edge;
    for (auto [p1, p2, p3] : triangulation) {
        auto* e1 = new HalfEdge<T>(p1);
        auto* e2 = new HalfEdge<T>(p2);
        auto* e3 = new HalfEdge<T>(p3);

        e1->next = e2; e2->next = e3; e3->next = e1;
        e1->prev = e3; e2->prev = e1; e3->prev = e2;

        link(p1, p2, e1);
        link(p2, p3, e2);
        link(p3, p1, e3);

        halfedges.push_back(e1);
        halfedges.push_back(e2);
        halfedges.push_back(e3);

        if (!pt_out_edge.count(p1)) pt_out_edge[p1] = e1;
        if (!pt_out_edge.count(p2)) pt_out_edge[p2] = e2;
        if (!pt_out_edge.count(p3)) pt_out_edge[p3] = e3;
    }

    map<int, HalfEdge<T>*> face_edge;
    map<HalfEdge<T>*, bool> vis;
    int faces = 0;

    vector<pt<ld>> face_centroid;
    for (auto &e : halfedges){
        if (vis[e]) continue;
        face_edge[faces] = e;
        auto v = e;
        ld cx = 0;
        ld cy = 0;
        int len = 0;
        do {
            len++;
            cx += v->source.x;
            cy += v->source.y;
            vis[v] = true;
            v = v->next;
        } while(v != e);
        cx /= len, cy /= len;
        face_centroid.emplace_back(cx,cy);
        faces++;
    }

    auto draw_text = [&](pt<ld> center, string s, glm::vec4 color) -> void {
        ld font_sz = 0.04*width;
        //        center.x -= (s.length()/2.0)*(font_sz/2);
        recorder.record_text(s, center, font_sz, color);
    };

    auto draw_half_edge = [&](HalfEdge<T>* e, glm::vec4 color) -> void {
        auto p = e->source;
        auto q = e->next->source;
        ld len = dist(p,q);
        ld ang = atan2(q.y-p.y, q.x-p.x);
        pt<float> start{
            p.x + 0.1f*len*cosf(ang),
                p.y + 0.1f*len*sinf(ang)
        };
        pt<float> end{
            q.x - 0.1f*len*cosf(ang),
                q.y - 0.1f*len*sinf(ang)
        };
        ld angp = ang + M_PI/2.0;
        ld offset = 0.008*width;
        start.x += offset*cos(angp);
        start.y += offset*sin(angp);
        end.x += offset*cos(angp);
        end.y += offset*sin(angp);
        recorder.record_line(start,end,color);
        recorder.draw_arrow_head(start,end,0.02*width,color);
    };

    auto draw_base = [&]() -> void {
        recorder.record_polygon(poly, {0.8,0.8,0.8,0.4});
        for (auto [p1,p2,p3] : triangulation){
            recorder.record_line(p1,p2,BLUE);
            recorder.record_line(p2,p3,BLUE);
            recorder.record_line(p3,p1,BLUE);
        }
        for (auto &e : halfedges) draw_half_edge(e, BLACK);

        for (int face = 0; face < faces; face++){
            draw_text(face_centroid[face], to_string(face), BLACK);
        }
    };

    draw_base();
    recorder.commit_step();

    // face delimiting
    for (int face = 0; face < faces; face++){
        draw_base();
        draw_text(face_centroid[face], to_string(face), RED);
        auto e = face_edge[face];
        auto v = e;
        do {
            draw_half_edge(v, RED);
            v = v->next;
        } while(v != e);
        recorder.commit_step();
    }

    // point walking
    for (auto p : poly){
        draw_base();
        recorder.record_circle(p, 0.005*width, RED, RED);
        auto e = pt_out_edge[p];

        // ccw run
        auto v = e;
        do {
            draw_half_edge(v, RED);
            v = v->prev;
            draw_half_edge(v, GREEN);
            v = v->twin;
        } while(v && v != e);

        // cw run
        v = e;
        do {
            draw_half_edge(v, RED);
            v = v->twin;
            if (!v) break;
            draw_half_edge(v, SEAGREEN);
            v = v->next;
        } while(v && v != e);

        recorder.commit_step();
    }

    auto merge_face = [&](HalfEdge<T>* e) -> bool {
        if (!e || !e->twin) return false; // border edge

        HalfEdge<T>* t = e->twin;

        // "antenna" cant merge 2 faces
        if (e->next == t || t->next == e) return false;

        HalfEdge<T>* prev_e = e->prev;
        HalfEdge<T>* next_e = e->next;
        HalfEdge<T>* prev_t = t->prev;
        HalfEdge<T>* next_t = t->next;

        prev_e->next = next_t;
        next_e->prev = prev_t;
        prev_t->next = next_e;
        next_t->prev = prev_e;

        if (pt_out_edge[e->source] == e) pt_out_edge[e->source] = next_t;
        if (pt_out_edge[t->source] == t) pt_out_edge[t->source] = next_e;

        delete e; delete t;
        return true;
    };

    // 'a' and 'b' must belong to the same face cycle and cannot share the same source.
    auto split_face = [&](HalfEdge<T>* a, HalfEdge<T>* b) -> bool {
        if (!a || !b || a == b || a->source == b->source) return false;

        HalfEdge<T>* v = a->next;
        while (v && v != a && v != b) v = v->next;
        if (v != b) return false; // Not in the same face

        auto* e = new HalfEdge<T>(a->source);
        auto* t = new HalfEdge<T>(b->source);

        e->twin = t;
        t->twin = e;

        HalfEdge<T>* prev_a = a->prev;
        HalfEdge<T>* prev_b = b->prev;

        prev_a->next = e;
        e->prev = prev_a;
        e->next = b;
        b->prev = e;

        prev_b->next = t;
        t->prev = prev_b;
        t->next = a;
        a->prev = t;

        return true;
    };

    recorder.run();

    for (auto e : halfedges) delete e;

    return 0;
}
