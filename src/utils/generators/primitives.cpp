#include "primitives.hpp"

float random_float(float mn, float mx){
    std::uniform_real_distribution<float> dist(mn,mx);
    return dist(rng);
}

int random_int(int mn, int mx){
    std::uniform_int_distribution<int> dist(mn,mx);
    return dist(rng);
}