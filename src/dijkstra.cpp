#include "utils/AlgorithmRecorder.hpp"
#include "utils/generators/graph.hpp"
#include <bits/stdc++.h>

using namespace std;

#define ii pair<int,int>

int main() {
    int n = 6, m = 8;
    bool directed = true;
    bool bezier = true;
    AlgorithmRecorder::GraphLayoutType layout_type = AlgorithmRecorder::GraphLayoutType::FORCE_DIRECTED;

    auto adj = random_weighted_graph(n, m, directed, 1, 20);

    AlgorithmRecorder recorder(-100, 100, -100, 100);
    auto [node_radius, pos] = recorder.compute_graph_layout(adj, layout_type);


    int start = 0;
    const int inf = 1e9;
    vector<int> dist(n, inf);
    priority_queue<ii, vector<ii>, greater<ii>> pq;
    dist[start] = 0;
    pq.push({dist[start], start});


    vector<bool> discovered(n);
    auto draw_base = [&]() -> void {
        recorder.record_weighted_graph(adj, pos, node_radius, directed, bezier);
        for (int u = 0; u < n; u++){
            if (discovered[u]) {
                recorder.draw_graph_node(pos[u], node_radius, to_string(u), SEAGREEN);
            }
            pt p = pos[u];
            p.x += 0.7*node_radius;
            auto c = discovered[u] ? BLUE : DARK_GREEN;
            string d = dist[u] == inf ? "∞" : to_string(dist[u]);
            recorder.record_text("("+d+")", pos[u], 0.5*node_radius, c);
        }
    };
    draw_base(); 
    recorder.commit_step();

    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (d > dist[u]) continue;

        discovered[u] = true;

        draw_base();
        recorder.record_highlight(pos[u], node_radius, RED);
        recorder.commit_step();

        for (auto [v, w] : adj[u]) {
            if (dist[v] > dist[u] + w) {
                dist[v] = dist[u] + w;
                pq.push({dist[v], v});
                
                draw_base();
                recorder.record_highlight(pos[u], node_radius, RED);
                recorder.record_highlight(pos[v], node_radius, GREEN);
                recorder.commit_step();
            }
        }
    }

    recorder.run(500);
    return 0;
}