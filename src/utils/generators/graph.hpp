#pragma once

#include "rng.hpp"
#include <bits/stdc++.h>
using namespace std;

struct FlowEdge {
    int to;
    int capacity;
    int flow;
    int rev_index;
};

#define ll long long

vector<vector<pair<int, int>>> random_weighted_graph(int n, int m, bool directed, int min_w = 1, int max_w = 20);

vector<vector<int>> random_graph(int n, int m, bool directed);

vector<vector<pair<int,int>>> random_weighted_dag(int n, int m, int min_w = 1, int max_w = 20);

vector<vector<int>> random_dag(int n, int m);

vector<vector<pair<int,int>>> random_weighted_graph_with_cycles(int n, int m, bool directed, int min_w = 1, int max_w = 20);

vector<vector<int>> random_graph_with_cycles(int n, int m, bool directed);

vector<vector<int>> random_functional_graph(int n);

vector<vector<int>> random_bipartite_graph(int n, int m);

vector<vector<int>> random_grid_graph(int rows, int cols);

vector<vector<FlowEdge>> random_flow_network(int n, int m, int min_cap = 1, int max_cap = 30);

vector<vector<FlowEdge>> random_bipartite_flow_network(int n,int m, int min_cap = 1, int max_cap = 30);