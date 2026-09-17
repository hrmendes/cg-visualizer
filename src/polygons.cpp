#include "geom/geom.hpp"
#include "utils/AlgorithmRecorder.hpp"
#include "geom/triangulation.hpp"
#include "utils/generators/geometry.hpp"

int main() {
    int mn = -1000, mx = 1000;
    AlgorithmRecorder recorder(mn, mx, mn, mx, 1000, 1000);

    recorder.commit_step();

    int runs = 40;
    for (int i = 1; i <= runs; i++){
        auto spiral = random_spiral_polygon<ld>(i*10, mn, mx);
        recorder.record_polygon(spiral, SKY_BLUE, BLUE, DARK_BLUE);
        recorder.commit_step();
    }

    recorder.run();
    return 0;
}
