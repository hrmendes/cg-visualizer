#include <bits/stdc++.h>
using namespace std;

#include "utils/AlgorithmRecorder.hpp"
#include "utils/generators/geometry.hpp"

int main() {
    AlgorithmRecorder recorder(-100, 100, -100, 100);

    vector<line<float>> segments = {
        line<float>(pt<float>(-80, -60), pt<float>(80, 60)),
        line<float>(pt<float>(-80, 60), pt<float>(80, -60)),
        line<float>(pt<float>(-70, 0), pt<float>(70, 0)),
        line<float>(pt<float>(0, -80), pt<float>(0, 80)),
        line<float>(pt<float>(-60, -70), pt<float>(60, 40))
    };

    for (int i = 0; i < 15; i++) {
        segments.push_back(random_line<float>(-90,90));
    }

    for (const auto& segment : segments) {
        recorder.record_line(
            segment.p,
            segment.q,
            BLACK
        );
        recorder.record_point(segment.p, BLACK);
        recorder.record_point(segment.q, BLACK);
    }

    for (int i = 0; i < (int)segments.size(); i++) {
        for (int j = i + 1; j < (int)segments.size(); j++) {
            line r = segments[i], s = segments[j];
            if (!interseg(r,s)) continue;
            pt p = inter(r,s);
            recorder.record_point(p, RED);
        }
    }

    recorder.commit_step();
    recorder.run();

    return 0;
}
