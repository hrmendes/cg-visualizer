#pragma once

#include <bits/stdc++.h>
using namespace std;

#include "rng.hpp"

vector<vector<int>> random_tree(int n) {
    vector<vector<int>> adj(n);
    if (n <= 1) return adj;

    vector<int> prufer(n - 2);
    uniform_int_distribution<int> node_dist(0, n-1);

    for (int& x : prufer) x = node_dist(rng);
    vector<int> deg(n, 1);
    for (int v : prufer) deg[v]++;

    priority_queue<int, vector<int>, greater<int>> leaves;
    for (int i = 0; i < n; i++) {
        if (deg[i] == 1) leaves.push(i);
    }

    for (int v : prufer) {
        int u = leaves.top(); leaves.pop();
        adj[u].push_back(v);
        adj[v].push_back(u);
        deg[u]--;
        deg[v]--;
        if (deg[v] == 1) leaves.push(v);
    }

    int u = leaves.top(); leaves.pop();
    int v = leaves.top();
    adj[u].push_back(v);
    adj[v].push_back(u);
    return adj;
}

vector<vector<pair<int, int>>> random_weighted_tree(int n, int min_w, int max_w) {
    auto adj2 = random_tree(n);
    uniform_int_distribution<int> dist(min_w, max_w);

    vector<vector<pair<int, int>>> adj(n);
    if (n <= 1) return adj;

    for (int u = 0; u < n; u++) {
        for (int v : adj2[u]) {
            if (u > v) continue;
            int w = dist(rng);
            adj[u].push_back({v, w});
            adj[v].push_back({u, w});
        }
    }
    return adj;
}
