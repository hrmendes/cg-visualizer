#include "rng.hpp"
#include <chrono>

std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());