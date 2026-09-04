#pragma once
#include "geom.hpp"
#include "../utils/AlgorithmRecorder.hpp"

// O(n^2) version of ears clipping triangulation
vector<tri> triangulate(vector<pt> poly, AlgorithmRecorder *rec = nullptr); 

// O(n^3) version of ears clipping triangulation
vector<tri> triangulate_deprecated(vector<pt> poly);