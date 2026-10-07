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

    // currently O(N^2 ears clipping implementation)
    auto triangulation = triangulate(poly);

    auto pts = poly;
    sort(all(pts));
    pts.erase(unique(all(pts)), pts.end());

    map<pair<int,int>, int> edge_triang;

    vector<array<int,3>> triang(triangulation.size());
    for (int i = 0; i < triang.size(); i++){
        auto [p1,p2,p3] = triangulation[i];
        int p1i = lower_bound(all(pts), p1) - pts.begin();
        int p2i = lower_bound(all(pts), p2) - pts.begin();
        int p3i = lower_bound(all(pts), p3) - pts.begin();
        triang[i] = {p1i,p2i,p3i};
        edge_triang[ii(p1i,p2i)] = edge_triang[ii(p2i,p3i)] = edge_triang[ii(p3i,p1i)] = i;
        // points are in ccw order
    }

    vector<vector<tuple<int,int,int>>> adj(triang.size());

    for (int u = 0; u < adj.size(); u++){
        auto [p1,p2,p3] = triang[u];
        if (edge_triang.count(ii(p2,p1))){
            int v = edge_triang[ii(p2,p1)];
            adj[u].push_back({v,p2,p1});
        }
        if (edge_triang.count(ii(p3,p2))){
            int v = edge_triang[ii(p3,p2)];
            adj[u].push_back({v,p3,p2});
        }
        if (edge_triang.count(ii(p1,p3))){
            int v = edge_triang[ii(p1,p3)];
            adj[u].push_back({v,p1,p3});
        }
    }

    vector<int> nxt(pts.size());
    iota(all(nxt), 0);
    int start = 0;
    for (int u = 0; u < triang.size(); u++){
        auto [p1,p2,p3] = triang[u];
        if (sgn(sarea2(pts[p1], pts[p2], guard)) < 0) continue;
        if (sgn(sarea2(pts[p2], pts[p3], guard)) < 0) continue;
        if (sgn(sarea2(pts[p3], pts[p1], guard)) < 0) continue;
        start = u;
    }
    auto [s1,s2,s3] = triang[start];
    nxt[s1] = s2;
    nxt[s2] = s3;
    nxt[s3] = s1;
    

    auto draw_pt = [&](pt p, glm::vec4 color) {
        recorder.record_circle(p, 0.003*(mx-mn), color);
    };
    auto draw_base = [&]() -> void {
        recorder.record_polygon(poly, {.8,.8,.8,.5}, BLACK, BLACK);

        for (auto [p1,p2,p3] : triang) {
            recorder.record_polygon(vector<pt>{pts[p1],pts[p2],pts[p3]},TRANSPARENT,{0,0,0,.6},TRANSPARENT);
        }

        draw_pt(guard, BLUE);

        int i = s1;
        do {
            recorder.record_line(pts[i], pts[nxt[i]], SEAGREEN);
            i = nxt[i];
        } while(i != s1);
    };


    draw_base();
    recorder.commit_step();

    auto intersect = [&](pt a, pt b, pt c, pt d) -> pt {
        // returns the intersection pt of the line defined by a and b
        // and the segment between c and d
        // it is guaranteed such an intersection exists

        auto param = [&](ld t) -> pt {
            return a*(1-t) + b*t;
        };
        pt p1 = param(-1e9), p2 = param(1e9);
        // p1 and p2 make the infinite line
        return inter(line(p1,p2), line(c,d));
    };

    vector<bool> vis(adj.size());
    auto dfs = [&](auto &&dfs, int u) -> void {
        vis[u] = true;
        draw_base(); recorder.commit_step();
        for (auto [v,p1,p2] : adj[u]){
            if (vis[v]) continue;
            int p3 = triang[v][0];
            if (p3 == p1 || p3 == p2) p3 = triang[v][1];
            if (p3 == p1 || p3 == p2) p3 = triang[v][2];
            int s1 = sgn(sarea2(guard,pts[p1],pts[p3])); // 1 is the left point, must not be ccw
            int s2 = sgn(sarea2(guard,pts[p2],pts[p3])); // 2 is the right point, must not be cw
            if (s1 <= 0 && s2 >= 0){
                nxt[p2] = p3;
                nxt[p3] = p1;
                dfs(dfs,v);
            } else if (s1 > 0) {
                // impossible to have 2 reflex angles because they form a triangle
                // so if one wasnt satisfied, the other must have been
                pts.push_back(intersect(guard,pts[p1],pts[p2],pts[p3]));
                int p4 = pts.size()-1;
                draw_base();
                draw_pt(pts[p4], RED);
                recorder.record_line(guard, pts[p4], RED);
                recorder.commit_step();
            } else {
                pts.push_back(intersect(guard,pts[p2],pts[p1],pts[p3]));
                int p4 = pts.size()-1;
                draw_base();
                draw_pt(pts[p4], RED);
                recorder.record_line(guard, pts[p4], RED);
                recorder.commit_step();

            }

        }
    }; dfs(dfs, start);

    recorder.run(500);
    return 0;
}
