#include "utils/AlgorithmRecorder.hpp"
#include "utils/generators/graph.hpp"
#include <bits/stdc++.h>
using namespace std;

#define int long long
#define pt pt<ld>
#define sq(x) ((x)*(x))
#define vi vector<int>
#define vvi vector<vi>
#define all(x) x.begin(),x.end()
#define ii pair<int,int>
#define dbg(v) cout << "Line(" << __LINE__ << ") -> " << #v << " = " << (v) << endl

signed main() {
    cout << "Number of points: ";
    int n; cin >> n;
    int mn = -100, mx = 100;
    vector<pt> poly = random_angular_polygon<ld>(n, 9*mn/10, 9*mx/10);
    pt guard;
    do {
        guard = random_pt<ld>(9*mn/10, 9*mx/10);
    } while(!inpol(poly, guard));

    AlgorithmRecorder recorder(mn,mx,mn,mx);

//    recorder.set_live_debug(true);
    auto pts = poly;

    vector<int> st;

    auto draw_pt = [&](pt p, glm::vec4 color) {
        recorder.record_circle(p, 0.005*(mx-mn), color);
    };

    auto draw_base = [&](int u = -1) -> void {
        recorder.record_polygon(poly, {.6,.6,.6,.5}, BLACK, BLACK);

        for (int i : st) draw_pt(pts[i], RED);
        for (int i = 0; i+1 < st.size(); i++) 
            recorder.record_line(pts[st[i]], pts[st[i+1]], RED);

        draw_pt(guard, BLUE);
    };

    pt start_pt(1e18,1e18);
    int minx_i = 0;
    for (int i = 0; i < n; i++){
        if ((poly[i].y > guard.y && poly[(i+1)%n].y < guard.y) || (poly[i].y<guard.y && poly[(i+1)%n].y>guard.y)){
            pt intersection = inter(line(pt(-1e9,guard.y),pt(1e9,guard.y)), line(poly[i],poly[(i+1)%n]));
            draw_base();
            recorder.record_line(pt(-1e9,guard.y),pt(1e9,guard.y),RED);
            recorder.record_line(poly[i],poly[(i+1)%n],RED);
            recorder.commit_step();
            if (intersection.x > guard.x && start_pt.x > intersection.x) {
                start_pt = intersection;

                minx_i = i;
            }
        }
    }
    
    draw_base();
    draw_pt(start_pt, SEAGREEN);
    recorder.commit_step();

    vector<int> nxt(poly.size()), pre(poly.size());
    iota(all(nxt), 1);
    nxt.back() = 0;
    iota(all(pre), -1);
    pre[0] = poly.size()-1;

    pts = poly;
    vector<ld> ang(pts.size());
    for (int i = 0; i < ang.size(); i++){
        ang[i] = atan2(pts[i].y-guard.y, pts[i].x-guard.x)+M_PI;
    }

    auto intersect = [&](pt a, pt b, pt c, pt d) -> pt {
        auto param = [&](ld t) -> pt { return a*(1-t) + b*t; };
        pt p1 = param(-1e9), p2 = param(1e9);
        return inter(line(p1,p2), line(c,d));
    };

    poly.insert(poly.begin()+minx_i+1, start_pt);

    bool hiding = false;
    st.push_back(minx_i+1);
    ang[minx_i+1] = 0;
    for (int i = 1; i < poly.size(); i++){
        if (hiding) {
            if (ang[i] >= ang[st.back()]) {
                pts.push_back(intersect(guard,pts[st.back()], pts[pre[i]],pts[i]));
                int p = pts.size()-1;
                ang.push_back(atan2(pts[p].y-guard.y, pts[p].x-guard.x)+M_PI);
                nxt[pre[i]] = p;
                nxt.push_back(i); // nxt[p] = i;
                pre.push_back(pre[i]); //pre[p] = pre[i];
                pre[i] = p;
                st.push_back(p);
                st.push_back(i);
                hiding = false;
            }
            continue;
        }

        if (ang[i] >= ang[st.back()]) st.push_back(i);
        else {
            if (ccw(pts[pre[st.back()]], pts[st.back()], pts[i])) {
                // coming forward
                while(!st.empty() && ang[st.back()] > ang[i]) {
                    draw_base(); recorder.commit_step();
                    st.pop_back();
                }
                draw_base(); recorder.commit_step();
                cout << st.empty() << endl;
                cout << st.back() << ' ' << nxt.size() << endl;
                cout << i << ' ' << st.back() << ' ' << pts.size() << endl;
                pts.push_back(intersect(guard,pts[i], pts[st.back()],pts[nxt[st.back()]]));
                int p = pts.size()-1;
                ang.push_back(atan2(pts[p].y-guard.y, pts[p].x-guard.x)+M_PI);
                nxt[st.back()] = p;
                nxt.push_back(i); // nxt[p] = i;
                pre[i] = p;
                pre.push_back(st.back()); // pre[p] = st.back();
                st.push_back(p);
                st.push_back(i);
                draw_base(); recorder.commit_step();
            } else {
                // hiding in the back
                hiding = true;
            }
        }
        draw_base(); recorder.commit_step();
    }
    recorder.record_polygon(poly, {.8,.8,.8,.5}, BLACK, BLACK);
    vector<pt> region; region.reserve(st.size());
    for (int i : st) region.push_back(pts[i]);
    recorder.record_polygon(region,{0,0,.5,.5},{0,0,.5,.5},TRANSPARENT);
    draw_pt(guard, BLUE);
    recorder.commit_step();

    recorder.run(50);
    return 0;
}
