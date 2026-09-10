#include <bits/stdc++.h>
using namespace std;

#include "utils/AlgorithmRecorder.hpp"
#include "utils/generators/geometry.hpp"

int main() {
    cout << "How many segments?\n";
    int n; cin >> n;

    AlgorithmRecorder recorder(-100, 100, -100, 100);
    vector<line<float>> segments(n);
    
    for (int i = 0; i < n; i++) {
        segments[i] = random_line<float>(-90,90);
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

    for (int i = 0; i < n; i++) {
        for (int j = i+1; j < n; j++) {
            line r = segments[i], s = segments[j];
            if (!interseg(r,s)) continue;
            pt p = inter(r,s);
            recorder.record_circle(p, 1.0, RED);
        }
    }

    recorder.commit_step();
    recorder.run();

    return 0;
}
