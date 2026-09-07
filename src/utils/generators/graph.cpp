#include "graph.hpp"
#include "rng.hpp"
#include <bits/stdc++.h>
using namespace std;

#define ll long long

vector<vector<pair<int, int>>> random_weighted_graph(int n, int m, bool directed, int min_w, int max_w) {
    vector<vector<pair<int, int>>> adj(n);
    uniform_int_distribution<int> dist(min_w, max_w);
    auto adj2 = random_graph(n,m,directed);
    for (int u = 0; u < n; u++){
        for (int v : adj2[u]){
            adj[u].push_back({v,dist(rng)});
        }
    }
    return adj;
}

vector<vector<int>> random_graph(int n, int m, bool directed) {
    vector<vector<int>> adj(n);
    uniform_int_distribution<int> node_dist(0, n-1);
    
    int edges_added = 0;
    while (edges_added < m) {
        int u = node_dist(rng);
        int v = node_dist(rng);
        if (u == v) continue;
        
        if (find(adj[u].begin(), adj[u].end(), v) == adj[u].end()) {
            adj[u].push_back(v);
            if (!directed) adj[v].push_back(u);
            edges_added++;
        }
    }
    return adj;
}

vector<vector<pair<int,int>>> random_weighted_dag(int n, int m, int min_w, int max_w) {
    vector<vector<pair<int, int>>> adj(n);
    uniform_int_distribution<int> dist(min_w, max_w);
    auto adj2 = random_dag(n,m);
    for (int u = 0; u < n; u++){
        for (int v : adj2[u]){
            adj[u].push_back({v,dist(rng)});
        }
    }
    return adj;
}

vector<vector<int>> random_dag(int n, int m) {
    vector<vector<int>> adj(n);
    if (n <= 1) return adj;
    
    ll max_edges = (ll)n*(n-1)/2;
    assert(m <= max_edges);

    int edges_added = 0;
    uniform_int_distribution<int> node_dist(0, n-1);

    while (edges_added < m) {
        int u = node_dist(rng);
        int v = node_dist(rng);
        if (u == v) continue;
        if (u > v) swap(u,v);
        
        if (find(adj[u].begin(), adj[u].end(), v) == adj[u].end()) {
            adj[u].push_back(v);
            edges_added++;
        }
    }
    return adj;
}

vector<vector<pair<int,int>>> random_weighted_graph_with_cycles(int n, int m, bool directed, int min_w, int max_w) {
    vector<vector<pair<int, int>>> adj(n);
    uniform_int_distribution<int> dist(min_w, max_w);
    auto adj2 = random_graph_with_cycles(n,m,directed);
    for (int u = 0; u < n; u++){
        for (int v : adj2[u]){
            adj[u].push_back({v,dist(rng)});
        }
    }
    return adj;
}

vector<vector<int>> random_graph_with_cycles(int n, int m, bool directed) {
    vector<vector<int>> adj(n);
    if (n <= 0) return adj;

    for (int i = 0; i < n; i++) {
        int nxt = (i + 1) % n;
        adj[i].push_back(nxt);
        if (!directed) adj[nxt].push_back(i);
    }

    int edges_added = n;
    uniform_int_distribution<int> node_dist(0, n - 1);

    while (edges_added < m) {
        int u = node_dist(rng);
        int v = node_dist(rng);
        if (u == v) continue;

        if (find(adj[u].begin(), adj[u].end(), v) == adj[u].end()) {
            adj[u].push_back(v);
            if (!directed) adj[v].push_back(u);
            edges_added++;
        }
    }
    return adj;
}

vector<vector<int>> random_functional_graph(int n) {
    vector<vector<int>> adj(n);
    uniform_int_distribution<int> node_dist(0, n - 1);
    for (int i = 0; i < n; i++) {
        adj[i].push_back(node_dist(rng));
    }
    return adj;
}

vector<vector<int>> random_bipartite_graph(int n, int m) {
    uniform_int_distribution<int> dist(1, n - 1);
    int n1 = dist(rng);
    int n2 = n-n1;
    vector<vector<int>> adj(n);
    uniform_int_distribution<int> u_dist(0, n1 - 1);
    uniform_int_distribution<int> v_dist(n1, n - 1);
    
    ll max_possible = (ll)n1 * n2;
    assert(m <= max_possible);

    int edges_added = 0;
    while (edges_added < m) {
        int u = u_dist(rng);
        int v = v_dist(rng);
        
        if (find(adj[u].begin(), adj[u].end(), v) == adj[u].end()) {
            adj[u].push_back(v);
            adj[v].push_back(u);
            edges_added++;
        }
    }
    return adj;
}

vector<vector<int>> random_grid_graph(int rows, int cols) {
    int n = rows * cols;
    vector<vector<int>> adj(n);
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int u = r * cols + c;
            for (int i = 0; i < 4; i++) {
                int nr = r + dr[i];
                int nc = c + dc[i];
                if (nr >= 0 && nr < rows && nc >= 0 && nc < cols) {
                    int v = nr * cols + nc;
                    adj[u].push_back(v);
                }
            }
        }
    }
    return adj;
}

vector<vector<FlowEdge>> random_flow_network(int n, int m, int min_cap, int max_cap) {
    vector<vector<FlowEdge>> adj(n);
    if (n < 2) return adj;

    uniform_int_distribution<int> node_dist(0, n - 1);
    uniform_int_distribution<int> cap_dist(min_cap, max_cap);

    auto add_edge = [&](int u, int v, int cap) {
        FlowEdge a{v, cap, 0, (int)adj[v].size()};
        FlowEdge b{u, 0, 0, (int)adj[u].size()};
        adj[u].push_back(a);
        adj[v].push_back(b);
    };

    int cur = 0;
    while (cur != n-1) {
        int next = (cur == 0 && n > 2) ? uniform_int_distribution<int>(1, n-2)(rng) : n - 1;
        if (next == cur) next = n - 1;
        add_edge(cur, next, cap_dist(rng));
        cur = next;
    }

    int edges_added = 0;
    while (edges_added < m) {
        int u = node_dist(rng);
        int v = node_dist(rng);
        if (u == v || u == n - 1 || v == 0) continue;

        bool exists = false;
        for (auto& edge : adj[u]) {
            if (edge.to == v) { exists = true; break; }
        }

        if (!exists) {
            add_edge(u, v, cap_dist(rng));
            edges_added++;
        }
    }
    return adj;
}

vector<vector<FlowEdge>> random_bipartite_flow_network(int n, int m, int min_cap, int max_cap) {
    assert(n >= 4);
    uniform_int_distribution<int> dist(1, n-3);
    int n1 = dist(rng);
    int n2 = n-n1-2;

    vector<vector<FlowEdge>> adj(n);

    int source = 0;
    int sink = n - 1;
    int u_start = 1;
    int v_start = 1 + n1;

    auto add_edge = [&](int u, int v, int cap) {
        FlowEdge a{v, cap, 0, (int)adj[v].size()};
        FlowEdge b{u, 0, 0, (int)adj[u].size()};
        adj[u].push_back(a);
        adj[v].push_back(b);
    };

    uniform_int_distribution<int> cap_dist(min_cap, max_cap);
    for (int i = 0; i < n1; i++) {
        add_edge(source, u_start + i, cap_dist(rng));
    }

    for (int j = 0; j < n2; j++) {
        add_edge(v_start + j, sink, cap_dist(rng));
    }

    uniform_int_distribution<int> u_dist(0, n1 - 1);
    uniform_int_distribution<int> v_dist(0, n2 - 1);

    int edges_added = 0;
    ll max_possible = (ll)n1 * n2;
    assert(m <= max_possible);

    while (edges_added < m) {
        int u = u_start + u_dist(rng);
        int v = v_start + v_dist(rng);

        bool exists = false;
        for (auto& edge : adj[u]) {
            if (edge.to == v) { exists = true; break; }
        }

        if (!exists) {
            add_edge(u, v, cap_dist(rng));
            edges_added++;
        }
    }

    return adj;
}