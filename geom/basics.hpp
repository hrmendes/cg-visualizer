#include "geom.hpp"
#include "../utils/AlgorithmRecorder.hpp"

#define all(x) x.begin(),x.end()

ld polarea(const vector<pt> &v, AlgorithmRecorder *rec = nullptr) {
    if (rec){
        rec->record_polygon(v,LIGHT_GRAY, BLACK, RED);
        rec->commit_step();
    }

    T ans = 0;
    for (int i = 0; i < sz(v); i++) {
        ans += (v[i] ^ v[(i+1)%sz(v)]);

        if (rec) {
            rec->record_polygon(v, {0.7,0.7,0.7,1}, {0,0,0,1}, {1,0,0,1});
            rec->record_polygon({pt(0,0), v[i], v[(i+1)%sz(v)]}, {0.5,0.5,1,1});
            for (int j = 0; j < i; j++) {
                rec->record_polygon({pt(0,0), v[j], v[j+1]}, {1,0.5,0.5,.5});
            }
            rec->commit_step();
        }
    }

    return abs((ld)ans / 2.0);
}

int inpol(const vector<pt> &v, pt p) {
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
