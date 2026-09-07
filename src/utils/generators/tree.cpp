#include "tree.hpp"
#include "rng.hpp"

vector<vector<pair<int,int>>> random_weighted_tree(int n, int min_w, int max_w){
    vector<vector<pair<int, int>>> adj(n);
    uniform_int_distribution<int> dist(min_w, max_w);
    auto adj2 = random_tree(n);
    for (int u = 0; u < n; u++){
        for (int v : adj2[u]){
            adj[u].push_back({v,dist(rng)});
        }
    }
    return adj;
}

vector<vector<int>> random_tree(int n) {
    vector<vector<int>> adj(n);
    if (n <= 1) return adj;
    if (n == 2) {
        adj[0].push_back(1);
        adj[1].push_back(0);
        return adj;
    }

    vector<int> prufer(n-2);
    uniform_int_distribution<int> node_dist(0, n-1);
    for (int &x : prufer) x = node_dist(rng);

    vector<int> freq(n, 1);
    for (int v : prufer) freq[v]++;

    for (int i = 0; i < n-2; i++) {
        int u = -1;
        for (int v = 0; v < n; v++) {
            if (freq[v] == 1) {
                u = v;
                break;
            }
        }

        int v = prufer[i];
        adj[u].push_back(v);
        adj[v].push_back(u);

        freq[u]--;
        freq[v]--;
    }

    int u = -1, v = -1;
    for (int i = 0; i < n; i++) {
        if (freq[i] == 1) {
            if (u == -1) u = i;
            else { v = i; break; }
        }
    }
    if (u != -1 && v != -1) {
        adj[u].push_back(v);
        adj[v].push_back(u);
    }

    return adj;
}