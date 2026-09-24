#include "geom/geom.hpp"
#include "utils/AlgorithmRecorder.hpp"
#include "utils/generators/geometry.hpp"

template<class T>
struct HalfEdge {
    pt<T> source;
    HalfEdge *next = nullptr, *prev = nullptr, *twin = nullptr;
    int vec_idx = -1;
    HalfEdge(pt<T> p, HalfEdge *nxt = nullptr, HalfEdge *pre = nullptr, HalfEdge *tw = nullptr) {
        source = p;
        next = nxt;
        prev = pre;
        twin = tw;
    }
};

using T = int;

int main() {
    int mn = -1000, mx = 1000;
    int width = mx - mn;
    AlgorithmRecorder recorder(mn, mx, mn, mx, 1000, 1000);

    int n = 25;

    auto poly = random_simple_polygon(n, mn, mx);

    recorder.record_polygon(poly, {0.8, 0.8, 0.8, 0.4});
    recorder.commit_step();

    vector<HalfEdge<T>*> halfedges;
    halfedges.reserve(6*n);

    auto add_he = [&](HalfEdge<T>* e) {
        e->vec_idx = halfedges.size();
        halfedges.push_back(e);
    };

    auto remove_he = [&](HalfEdge<T>* e) {
        int idx = e->vec_idx;
        HalfEdge<T>* last = halfedges.back();
        halfedges[idx] = last;
        last->vec_idx = idx;
        halfedges.pop_back();
        e->vec_idx = -1;
    };

    map<pt<T>, HalfEdge<T>*> pt_out_edge;
    vector<HalfEdge<T>*> initial_cycle(n);
    for (int i = 0; i < n; i++) {
        initial_cycle[i] = new HalfEdge<T>(poly[i]);
        add_he(initial_cycle[i]);
        pt_out_edge[poly[i]] = initial_cycle[i];
    }
    for (int i = 0; i < n; i++) {
        initial_cycle[i]->next = initial_cycle[(i+1)%n];
        initial_cycle[(i+1)%n]->prev = initial_cycle[i];
    }

    auto draw_text = [&](pt<ld> center, string s, glm::vec4 color) -> void {
        ld font_sz = 0.04 * width;
        recorder.record_text(s, center, font_sz, color);
    };

    auto draw_half_edge = [&](HalfEdge<T>* e, glm::vec4 color) -> void {
        auto p = e->source;
        auto q = e->next->source;
        recorder.record_line(p, q, BLUE);
        ld len = dist(p, q);
        ld ang = atan2(q.y - p.y, q.x - p.x);
        pt<float> start{
            p.x + 0.1f * len * cosf(ang),
            p.y + 0.1f * len * sinf(ang)
        };
        pt<float> end{
            q.x - 0.1f * len * cosf(ang),
            q.y - 0.1f * len * sinf(ang)
        };
        ld angp = ang + M_PI / 2.0;
        ld offset = 0.008 * width;
        start.x += offset * cos(angp);
        start.y += offset * sin(angp);
        end.x += offset * cos(angp);
        end.y += offset * sin(angp);
        recorder.record_line(start, end, color);
        recorder.draw_arrow_head(start, end, 0.02 * width, color);
    };

    auto draw_base = [&]() -> void {
        recorder.record_polygon(poly, {0.8, 0.8, 0.8, 0.4});
        for (auto& e : halfedges) draw_half_edge(e, BLACK);
    };

    auto draw_all_faces = [&]() -> void {
        recorder.record_polygon(poly, {0.8, 0.8, 0.8, 0.4});
        map<HalfEdge<T>*, bool> visited;
        int face = 0;
        for (auto e : halfedges) {
            if (!e || visited[e]) continue;
            auto v = e;
            ld cx = 0, cy = 0;
            int len = 0;
            do {
                visited[v] = true;
                cx += v->source.x;
                cy += v->source.y;
                len++;
                draw_half_edge(v, BLACK);
                v = v->next;
            } while (v && v != e);
            cx /= len; cy /= len;
            draw_text(pt<ld>{cx, cy}, to_string(face++), RED);
        }
        recorder.commit_step();
    };

    draw_all_faces();

    // out and in edges for each point
    for (auto p : poly) {
        draw_base();
        recorder.record_circle(p, 0.005 * width, RED, RED);
        auto e = pt_out_edge[p];

        auto v = e;
        do {
            draw_half_edge(v, RED);
            v = v->prev;
            draw_half_edge(v, GREEN);
            v = v->twin;
        } while (v && v != e);

        v = e;
        do {
            draw_half_edge(v, RED);
            v = v->twin;
            if (!v) break;
            draw_half_edge(v, SEAGREEN);
            v = v->next;
        } while (v && v != e);

        recorder.commit_step();
    }

    auto merge_face = [&](HalfEdge<T>* e) -> bool {
        if (!e || !e->twin) return false;

        HalfEdge<T>* t = e->twin;

        if (e->next == t || t->next == e) return false;

        HalfEdge<T>* prev_e = e->prev;
        HalfEdge<T>* next_e = e->next;
        HalfEdge<T>* prev_t = t->prev;
        HalfEdge<T>* next_t = t->next;

        prev_e->next = next_t;
        next_t->prev = prev_e;
        prev_t->next = next_e;
        next_e->prev = prev_t;

        if (pt_out_edge[e->source] == e) pt_out_edge[e->source] = next_t;
        if (pt_out_edge[t->source] == t) pt_out_edge[t->source] = next_e;

        remove_he(e);
        remove_he(t);

        delete e;
        delete t;
        return true;
    };

    auto in_cone = [&](HalfEdge<T>* e, pt<T> v) -> bool {
        pt<T> u = e->source;
        pt<T> pre = e->prev->source;
        pt<T> nxt = e->next->source;

        if (ccw(pre, u, nxt)) {
            return ccw(u, nxt, v) && ccw(u, v, pre);
        }
        return !(ccw(u, pre, v) && ccw(u, v, nxt));
    };

    auto is_diagonal = [&](HalfEdge<T>* a, HalfEdge<T>* b) -> bool {
        if (!a || !b || a == b || a->source == b->source) return false;
        if (a->next == b || b->next == a) return false;

        auto v = a->next;
        while (v && v != a && v != b) v = v->next;
        if (v != b) return false;

        if (!in_cone(a, b->source) || !in_cone(b, a->source)) return false;

        pt<T> p1 = a->source;
        pt<T> p2 = b->source;

        v = a;
        do {
            pt<T> q1 = v->source;
            pt<T> q2 = v->next->source;
            if (q1 != p1 && q1 != p2 && q2 != p1 && q2 != p2) {
                if (interseg(line(p1, p2), line(q1, q2))) return false;
            }
            v = v->next;
        } while (v != a);

        return true;
    };

    auto split_face = [&](HalfEdge<T>* a, HalfEdge<T>* b) -> pair<HalfEdge<T>*, HalfEdge<T>*> {
        if (!is_diagonal(a, b)) return {nullptr, nullptr};

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

        add_he(e);
        add_he(t);

        return {e, t};
    };

    // Random Splits
    int limit = 2*n; // limit attempts
    while (limit--) {
        if (halfedges.size() < 2) break;

        uniform_int_distribution<int> dist_he(0, (int)halfedges.size() - 1);
        int idx_a = dist_he(rng);
        int idx_b = dist_he(rng);
        while (idx_b == idx_a) idx_b = dist_he(rng);

        HalfEdge<T>* a = halfedges[idx_a];
        HalfEdge<T>* b = halfedges[idx_b];

        draw_base();
        recorder.record_circle(a->source, 0.0075 * width, GREEN, GREEN);
        recorder.record_circle(b->source, 0.0075 * width, GREEN, GREEN);
        draw_half_edge(a, GREEN);
        draw_half_edge(b, GREEN);
        recorder.commit_step();

        auto [e, t] = split_face(a, b);
        if (e && t)  draw_all_faces();
    }

    // Merges
    while(1){
        HalfEdge<T>* target = nullptr;
        for (auto* e : halfedges) {
            if (!e || !e->twin) continue;
            if (e->next == e->twin || e->twin->next == e) continue;

            HalfEdge<T>* cur = e->next;
            while (cur && cur != e && cur != e->twin) cur = cur->next;
            if (cur == e->twin) continue;

            target = e;
            break;
        }
        if (!target) break;
        merge_face(target);
        draw_all_faces();
    }
    recorder.run();

    for (auto* e : halfedges) delete e;

    return 0;
}
