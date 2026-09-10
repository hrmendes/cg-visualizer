#pragma once

#include "rng.hpp"
#include "glm/glm.hpp"

float random_float(float mn, float mx){
    std::uniform_real_distribution<float> dist(mn,mx);
    return dist(rng);
}

int random_int(int mn, int mx){
    std::uniform_int_distribution<int> dist(mn,mx);
    return dist(rng);
}

glm::vec4 random_color(){
    return {
        random_float(0,1),
        random_float(0,1),
        random_float(0,1),
        1
    };
}