#include "arrays.hpp"

std::vector<int> random_array(int n, int mn, int mx){
    std::vector<int> ans(n);
    std::uniform_int_distribution<int> dist(mn,mx);
    for (int &x : ans) x = dist(rng);
    return ans;
}