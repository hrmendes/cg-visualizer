#pragma once

#include "rng.hpp"
#include <bits/stdc++.h>
using namespace std;

#define ll long long

vector<vector<int>> random_graph(int n, int m, bool directed) {
    vector<vector<int>> adj(n);
    if (n <= 1 || m <= 0) return adj;

    ll max_edges = (ll)n * (n - 1) / (directed ? 1 : 2);
    m = min((ll)m, max_edges);

    int edges_added = 0;
    uniform_int_distribution<int> node_dist(0, n - 1);
    
    if (m >= max_edges / 2 && max_edges < 500000) {
        vector<pair<int, int>> all_edges;
        for (int u = 0; u < n; u++) {
            for (int v = (directed ? 0 : u + 1); v < n; v++) {
                if (u != v) all_edges.push_back({u, v});
            }
        }
        shuffle(all_edges.begin(), all_edges.end(), rng);
        for (int i = 0; i < m; i++) {
            adj[all_edges[i].first].push_back(all_edges[i].second);
            if (!directed) adj[all_edges[i].second].push_back(all_edges[i].first);
        }
        return adj;
    }

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

vector<vector<pair<int, int>>> random_weighted_graph(int n, int m, bool directed, int min_w, int max_w) {
    vector<vector<pair<int, int>>> adj(n);
    auto adj2 = random_graph(n, m, directed);
    uniform_int_distribution<int> dist(min_w, max_w);
    for (int u = 0; u < n; u++) {
        for (int v : adj2[u]) {
            if (directed) adj[u].push_back({v, dist(rng)});
            else if (u < v) {
                int w = dist(rng);
                adj[u].push_back({v, w});
                adj[v].push_back({u, w});
            }
        }
    }
    return adj;
}

vector<vector<int>> random_dag(int n, int m) {
    vector<vector<int>> adj(n);
    if (n <= 1) return adj;
    
    ll max_edges = (ll)n * (n - 1) / 2;
    m = min((ll)m, max_edges);

    int edges_added = 0;
    uniform_int_distribution<int> node_dist(0, n - 1);

    while (edges_added < m) {
        int u = node_dist(rng);
        int v = node_dist(rng);
        if (u == v) continue;
        if (u > v) swap(u, v);
        
        if (find(adj[u].begin(), adj[u].end(), v) == adj[u].end()) {
            adj[u].push_back(v);
            edges_added++;
        }
    }
    return adj;
}

vector<vector<pair<int,int>>> random_weighted_dag(int n, int m, int min_w, int max_w) {
    vector<vector<pair<int, int>>> adj(n);
    uniform_int_distribution<int> dist(min_w, max_w);
    auto adj2 = random_dag(n, m);
    for (int u = 0; u < n; u++){
        for (int v : adj2[u]){
            adj[u].push_back({v, dist(rng)});
        }
    }
    return adj;
}

vector<vector<int>> random_graph_with_cycles(int n, int m, bool directed) {
    vector<vector<int>> adj(n);
    if (n <= 1 || m <= 0) return adj;

    if (directed) {
        for (int i = 0; i < n; i++) {
            adj[i].push_back((i + 1) % n);
        }
    } else {
        for (int i = 0; i < n; i++) {
            int j = (i + 1) % n;
            adj[i].push_back(j);
            adj[j].push_back(i);
        }
    }

    ll max_edges = (ll)n * (n - 1) / (directed ? 1 : 2);
    m = min((ll)m, max_edges);

    int edges_added = n;
    uniform_int_distribution<int> node_dist(0, n - 1);

    while (edges_added < m) {
        int u = node_dist(rng);
        int v = node_dist(rng);

        if (u == v) continue;

        if (find(adj[u].begin(), adj[u].end(), v) == adj[u].end()) {
            adj[u].push_back(v);

            if (!directed)
                adj[v].push_back(u);

            edges_added++;
        }
    }

    return adj;
}

vector<vector<pair<int,int>>> random_weighted_graph_with_cycles(int n, int m, bool directed, int min_w, int max_w) {
    vector<vector<pair<int, int>>> adj(n);
    uniform_int_distribution<int> dist(min_w, max_w);
    auto adj2 = random_graph_with_cycles(n, m, directed);
    for (int u = 0; u < n; u++){
        for (int v : adj2[u]){
            adj[u].push_back({v, dist(rng)});
        }
    }
    return adj;
}

vector<vector<int>> random_functional_graph(int n) {
    vector<vector<int>> adj(n);
    if (n <= 0) return adj;
    uniform_int_distribution<int> node_dist(0, n - 1);
    for (int i = 0; i < n; i++) {
        adj[i].push_back(node_dist(rng));
    }
    return adj;
}

vector<vector<int>> random_bipartite_graph(int n, int m) {
    if (n <= 2) return vector<vector<int>>(n);
    uniform_int_distribution<int> dist(1, n - 1);
    int n1 = dist(rng);
    int n2 = n - n1;
    vector<vector<int>> adj(n);
    
    ll max_possible = (ll)n1 * n2;
    m = min((ll)m, max_possible);

    uniform_int_distribution<int> u_dist(0, n1 - 1);
    uniform_int_distribution<int> v_dist(n1, n - 1);
    
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

vector<vector<pair<int, int>>> random_flow_network(int n, int m, int min_cap, int max_cap, bool bipartite) {
    vector<vector<pair<int, int>>> adj(n);
    if (n < 2 || m <= 0) return adj;

    uniform_int_distribution<int> cap_dist(
        min_cap,
        max_cap
    );

    auto has_edge = [&](int u, int v) {
        for (auto [to, cap] : adj[u]) {
            if (to == v) return true;
        }
        return false;
    };

    auto add_edge = [&](int u, int v) {
        adj[u].push_back({v, cap_dist(rng)});
    };

    int source = 0, sink = n - 1;

    if (bipartite) {
        vector<int> left, right;

        for (int i = 1; i < n - 1; i++) {
            if (i&1) right.push_back(i);
            else left.push_back(i);
        }

        if (left.empty() || right.empty()) return adj;

        int mandatory_edges = left.size() + right.size();
        m = max(m, mandatory_edges);

        for (int u : left) {
            add_edge(source, u);
        }

        for (int v : right) {
            add_edge(v, sink);
        }

        int edges_added = mandatory_edges;
        while (edges_added < m) {
            int u = left[rng() % left.size()];
            int v = right[rng() % right.size()];
            if (has_edge(u, v)) continue;
            add_edge(u, v);
            edges_added++;
        }
        return adj;
    }

    vector<int> middle;

    for (int i = 1; i < n - 1; i++) middle.push_back(i);

    if (middle.empty()) {
        add_edge(source, sink);
        return adj;
    }

    shuffle(middle.begin(), middle.end(), rng);

    add_edge(source, middle[0]);

    for (int i = 0; i+1 < (int)middle.size(); i++) {
        add_edge(middle[i], middle[i + 1]);
    }

    add_edge(middle.back(), sink);

    int edges_added = middle.size() + 1;

    m = max(m, edges_added);

    while (edges_added < m) {
        int u = rng() % (n - 1);
        int v = 1 + rng() % (n - 1);

        if (u == v) continue;
        if (u == sink) continue;
        if (v == source) continue;
        if (has_edge(u, v)) continue;

        add_edge(u, v);
        edges_added++;
    }
    return adj;
}
