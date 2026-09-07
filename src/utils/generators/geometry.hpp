#pragma once

#include "rng.hpp"
#include "../../geom/geom.hpp"

pt random_pt(ld min_c, ld max_c);

vector<pt> random_simple_polygon(int n, ld min_c, ld max_c);