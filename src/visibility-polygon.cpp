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
    recorder.set_live_debug(true);

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

        edge_triang[ii(p1i,p2i)] = i;
        edge_triang[ii(p2i,p3i)] = i;
        edge_triang[ii(p3i,p1i)] = i;
    }

    vector<vector<tuple<int,int,int>>> adj(triang.size());
    for (int u = 0; u < adj.size(); u++){
        auto [a, b, c] = triang[u];
        if (edge_triang.count(ii(b,a))) adj[u].push_back({edge_triang[ii(b,a)], b, a});
        if (edge_triang.count(ii(c,b))) adj[u].push_back({edge_triang[ii(c,b)], c, b});
        if (edge_triang.count(ii(a,c))) adj[u].push_back({edge_triang[ii(a,c)], a, c});
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

    int start_sz = pts.size();

    auto draw_pt = [&](pt p, glm::vec4 color) {
        recorder.record_circle(p, 0.003*(mx-mn), color);
    };

    auto draw_base = [&](int u = -1) -> void {
        recorder.record_polygon(poly, {.6,.6,.6,.5}, BLACK, BLACK);
        for (auto [p1,p2,p3] : triang) {
            recorder.record_polygon(vector<pt>{pts[p1],pts[p2],pts[p3]},TRANSPARENT,{0,0,0,.6},TRANSPARENT);
        }

        int i = s1;
        vector<pt> region;
        dbg("Defining region");
        do {
            region.push_back(pts[i]);
            i = nxt[i];
        } while(i != s1);
        recorder.record_polygon(region,{0,.5,0,.5},{0,.5,0,.5},TRANSPARENT);
        dbg("Defined region");

        if (u != -1){
            recorder.record_polygon(
                    vector<pt>{pts[triang[u][0]],pts[triang[u][1]],pts[triang[u][2]]}
                    ,{0,0,.5,.5},BLUE,TRANSPARENT);
        }

        for (int i = start_sz; i < pts.size(); i++) draw_pt(pts[i], RED);
        draw_pt(guard, BLUE);
    };

    draw_base();
    recorder.commit_step();

    auto intersect = [&](pt a, pt b, pt c, pt d) -> pt {
        auto param = [&](ld t) -> pt { return a*(1-t) + b*t; };
        pt p1 = param(-1e9), p2 = param(1e9);
        return inter(line(p1,p2), line(c,d));
    };

    vector<bool> vis(adj.size(), false);

    auto dfs = [&](auto &&dfs, int u) -> void {
        dbg(u);
        vis[u] = true;
        draw_base(u);
        recorder.commit_step();

        auto current_adj = adj[u];

        for (auto [v, p1, p2] : current_adj){
            if (vis[v]) continue;

            int p3 = triang[v][0];
            if (p3 == p1 || p3 == p2) p3 = triang[v][1];
            if (p3 == p1 || p3 == p2) p3 = triang[v][2];

            int s1_area = sgn(sarea2(guard, pts[p1], pts[p3])); 
            int s2_area = sgn(sarea2(guard, pts[p2], pts[p3])); 

            if (s1_area <= 0 && s2_area >= 0){ 
                nxt[p2] = p3;
                nxt[p3] = p1;
                dfs(dfs, v);
            } 
            else if (s1_area > 0) { // Split pela esquerda
                dbg("start split left");
                pts.push_back(intersect(guard, pts[p1], pts[p2], pts[p3]));
                int p4 = pts.size() - 1;

                nxt[p2] = p4;
                nxt.push_back(p1); 

                draw_base(u);
                recorder.record_line(guard, pts[p4], RED);
                recorder.commit_step();

                int newv = adj.size();
                adj.push_back({});
                vis.push_back(true); 

                // Garantindo CCW
                triang[v] = {p1, p2, p4}; 
                triang.push_back({p1, p4, p3}); 

                auto old_adj_v = adj[v];
                adj[v].clear();
                int other = -1; 

                for (auto [nei, l, r] : old_adj_v) {
                    if (l == p2 && r == p1) { 
                        adj[v].push_back({nei, l, r});
                    } else if (l == p1 && r == p3) { 
                        adj[newv].push_back({nei, l, r});
                        for (auto& [nx, nl, nr] : adj[nei]) {
                            if (nx == v && nl == p3 && nr == p1) nx = newv;
                        }
                    } else if (l == p3 && r == p2) { 
                        other = nei;
                    }
                }

                adj[v].push_back({newv, p1, p4});
                adj[newv].push_back({v, p4, p1});

                if (other != -1) {
                    int p5 = triang[other][0];
                    if (p5 == p2 || p5 == p3) p5 = triang[other][1];
                    if (p5 == p2 || p5 == p3) p5 = triang[other][2];

                    // p3 eu sei que ta pra fora do cone de visao
                    // mas se o p5 tiver pra fora do cone (direita de p2)
                    // entao eu geraria splits infinitos
                    // pra isso acho um p6 e divido o quadrilatero do meio

                    if (sgn(sarea2(guard,pts[p2],pts[p5])) < 0) {
                        pts.push_back(intersect(guard,pts[p2], pts[p3],pts[p5]));
                        int p6 = pts.size()-1;
                        nxt.push_back(0); // not visited yet, but will be eventually

                        // triangulo p3,p4,p6 (newv2)
                        int newv2 = adj.size();
                        adj.push_back({});
                        vis.push_back(false);
                        triang.push_back({p3,p4,p6});

                        // triangulo p2,p5,p6 (newv3)
                        int newv3 = adj.size();
                        adj.push_back({});
                        vis.push_back(true);
                        triang.push_back({p2,p5,p6});

                        // triangulo p2,p6,p4 (other)
                        triang[other] = {p2,p6,p4};

                        auto old_adj_other = adj[other];
                        adj[other].clear();

                        adj[other].push_back({newv2,p4,p6});
                        adj[newv2].push_back({other,p6,p4});

                        adj[other].push_back({v,p2,p4});
                        adj[v].push_back({other,p4,p2});

                        adj[newv2].push_back({newv,p4,p3});
                        adj[newv].push_back({newv2,p3,p4});

                        adj[other].push_back({newv3,p6,p2});
                        adj[newv3].push_back({other, p2, p6});

                        int other2 = -1;
                        for (auto [nei, l, r] : old_adj_other){
                            if (l == p5 && r == p2) {
                                adj[newv3].push_back({nei,l,r});
                                for (auto &[nx,nl,nr] : adj[nei]) {
                                    if (nx == other && nl == p2 && nr == p5) nx = newv3;
                                }
                            } else if (l == p3 && r == p5){
                                other2 = nei;
                            }
                        }
                        if (other2 != -1){
                            int p7 = triang[other2][0];
                            if (p7 == p3 || p7 == p5) p7 = triang[other2][1];
                            if (p7 == p3 || p7 == p5) p7 = triang[other2][2];

                            triang[other2] = {p3,p6,p7};
                            triang.push_back({p6,p5,p7});

                            int newv4 = adj.size();
                            adj.push_back({});
                            vis.push_back(false);
                            auto old_adj_other2 = adj[other2];
                            adj[other2].clear();

                            adj[other2].push_back({newv2,p6,p3});
                            adj[newv2].push_back({other2,p3,p6});

                            adj[newv4].push_back({newv3,p5,p6});
                            adj[newv3].push_back({newv4,p6,p5});

                            adj[other2].push_back({newv4,p7,p6});
                            adj[newv4].push_back({other2,p6,p7});

                            for (auto [nei,l,r] : old_adj_other2) {
                                if (l == p3 && r == p7) {
                                    adj[other2].push_back({nei,l,r});
                                } else if (l == p7 && r == p5) {
                                    adj[newv4].push_back({nei,l,r});
                                }
                            }
                        }

                    } else {
                        int newv2 = adj.size();
                        adj.push_back({});
                        vis.push_back(false); 

                        // Garantindo CCW
                        triang[other] = {p4, p2, p5}; 
                        triang.push_back({p3, p4, p5}); 

                        auto old_adj_other = adj[other];
                        adj[other].clear();

                        for (auto [nei, l, r] : old_adj_other) {
                            if (l == p5 && r == p2) { 
                                adj[other].push_back({nei, l, r});
                            } else if (l == p3 && r == p5) { 
                                adj[newv2].push_back({nei, l, r});
                                for (auto& [nx, nl, nr] : adj[nei]) {
                                    if (nx == other && nl == p5 && nr == p3) nx = newv2;
                                }
                            }
                        }

                        adj[other].push_back({newv2, p4, p5});
                        adj[newv2].push_back({other, p5, p4});

                        adj[other].push_back({v, p2, p4});
                        adj[v].push_back({other, p4, p2});

                        adj[newv2].push_back({newv, p4, p3});
                        adj[newv].push_back({newv2, p3, p4});
                    }
                }
                dbg("end split left");
                dfs(dfs, v);
            } 
            else { // Split pela direita
                dbg("start split right");
                pts.push_back(intersect(guard, pts[p2], pts[p1], pts[p3]));
                int p4 = pts.size() - 1;

                // CORREÇÃO CRUCIAL AQUI: a fronteira também flui de p2 -> p4 -> p1
                nxt[p2] = p4;
                nxt.push_back(p1); 

                draw_base(u);
                recorder.record_line(guard, pts[p4], RED);
                recorder.commit_step();

                int newv = adj.size();
                adj.push_back({});
                vis.push_back(true); 

                // Garantindo CCW
                triang[v] = {p1, p2, p4}; 
                triang.push_back({p4, p2, p3}); 

                auto old_adj_v = adj[v];
                adj[v].clear();
                int other = -1;

                for (auto [nei, l, r] : old_adj_v) {
                    if (l == p2 && r == p1) { 
                        adj[v].push_back({nei, l, r});
                    } else if (l == p3 && r == p2) { 
                        adj[newv].push_back({nei, l, r});
                        for (auto& [nx, nl, nr] : adj[nei]) {
                            if (nx == v && nl == p2 && nr == p3) nx = newv;
                        }
                    } else if (l == p1 && r == p3) { 
                        other = nei;
                    }
                }

                adj[v].push_back({newv, p4, p2});
                adj[newv].push_back({v, p2, p4});

                if (other != -1) {
                    int p5 = triang[other][0];
                    if (p5 == p1 || p5 == p3) p5 = triang[other][1];
                    if (p5 == p1 || p5 == p3) p5 = triang[other][2];

                    // p3 eu sei que ta pra fora do cone de visao
                    // mas se o p5 tiver pra fora do cone (esquerda de p1)
                    // entao eu geraria splits infinitos
                    // pra isso acho um p6 e divido o quadrilatero do meio
                    if (sgn(sarea2(guard, pts[p1], pts[p5])) > 0) { // ccw
                        pts.push_back(intersect(guard,pts[p1], pts[p3],pts[p5]));
                        int p6 = pts.size()-1;
                        nxt.push_back(0); // not visited yet, but will be eventually

                        // triangulo p4,p3,p6 (newv2)
                        int newv2 = adj.size();
                        adj.push_back({});
                        vis.push_back(false);
                        triang.push_back({p4,p3,p6});

                        // triangulo p5,p1,p6 (newv3)
                        int newv3 = adj.size();
                        adj.push_back({});
                        vis.push_back(true);
                        triang.push_back({p5,p1,p6});

                        // triangulo p1,p4,p6 (other)
                        triang[other] = {p1,p4,p6};

                        auto old_adj_other = adj[other];
                        adj[other].clear();

                        adj[other].push_back({newv2,p6,p4});
                        adj[newv2].push_back({other,p4,p6});

                        adj[other].push_back({v,p4,p1});
                        adj[v].push_back({other,p1,p4});

                        adj[newv2].push_back({newv,p3,p4});
                        adj[newv].push_back({newv2,p4,p3});

                        adj[other].push_back({newv3,p1,p6});
                        adj[newv3].push_back({other,p6,p1});

                        int other2 = -1;
                        for (auto [nei, l, r] : old_adj_other){
                            if (l == p1 && r == p5) {
                                adj[newv3].push_back({nei,l,r});
                                for (auto &[nx,nl,nr] : adj[nei]) {
                                    if (nx == other && nl == p5 && nr == p1) nx = newv3;
                                }
                            } else if (l == p5 && r == p3){
                                other2 = nei;
                            }
                        }
                        if (other2 != -1){
                            int p7 = triang[other2][0];
                            if (p7 == p3 || p7 == p5) p7 = triang[other2][1];
                            if (p7 == p3 || p7 == p5) p7 = triang[other2][2];

                            triang[other2] = {p3,p7,p6};
                            triang.push_back({p5,p6,p7});

                            int newv4 = adj.size();
                            adj.push_back({});
                            vis.push_back(false);
                            auto old_adj_other2 = adj[other2];
                            adj[other2].clear();

                            adj[other2].push_back({newv2,p3,p6});
                            adj[newv2].push_back({other2,p6,p3});

                            adj[newv4].push_back({newv3,p6,p5});
                            adj[newv3].push_back({newv4,p5,p6});

                            adj[other2].push_back({newv4,p6,p7});
                            adj[newv4].push_back({other2,p7,p6});

                            for (auto [nei,l,r] : old_adj_other2) {
                                if (l == p7 && r == p3) {
                                    adj[other2].push_back({nei,l,r});
                                } else if (l == p5 && r == p7) {
                                    adj[newv4].push_back({nei,l,r});
                                }
                            }
                        }
                    }
                    else {
                        int newv2 = adj.size();
                        adj.push_back({});
                        vis.push_back(false); 

                        // Garantindo CCW
                        triang[other] = {p1, p4, p5}; 
                        triang.push_back({p4, p3, p5}); 

                        auto old_adj_other = adj[other];
                        adj[other].clear();

                        for (auto [nei, l, r] : old_adj_other) {
                            if (l == p1 && r == p5) { 
                                adj[other].push_back({nei, l, r});
                            } else if (l == p5 && r == p3) { 
                                adj[newv2].push_back({nei, l, r});
                                for (auto& [nx, nl, nr] : adj[nei]) {
                                    if (nx == other && nl == p3 && nr == p5) nx = newv2;
                                }
                            }
                        }

                        adj[other].push_back({newv2, p5, p4});
                        adj[newv2].push_back({other, p4, p5});

                        adj[other].push_back({v, p4, p1});
                        adj[v].push_back({other, p1, p4});

                        adj[newv2].push_back({newv, p3, p4});
                        adj[newv].push_back({newv2, p4, p3});
                    }
                }
                dbg("end split right");
            }
            dfs(dfs, v);
        }
    }; 
    dfs(dfs, start);

    recorder.record_polygon(poly, {.8,.8,.8,.5}, BLACK, BLACK);

    int i = s1;
    vector<pt> region;
    do {
        region.push_back(pts[i]);
        i = nxt[i];
    } while(i != s1);
    recorder.record_polygon(region,{0,0,.5,.5},{0,0,.5,.5},TRANSPARENT);

    draw_pt(guard, BLUE);
    recorder.commit_step();

    recorder.run(50);
    return 0;
}
