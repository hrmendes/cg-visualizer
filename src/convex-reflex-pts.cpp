#include <bits/stdc++.h>
using namespace std;

#include "utils/AlgorithmRecorder.hpp"
#include "utils/generators/geometry.hpp"

int main() {
    AlgorithmRecorder recorder(-100, 100, -100, 100);

    int n = 15;
    auto poly = random_simple_polygon(n, -80, 80);
    recorder.record_polygon(poly);

    for (int cur = 0; cur < n; cur++) {
        int pre = (cur+n-1)%n;
        int nxt = (cur+1)%n;
        
        int sign = sgn(sarea2(poly[pre], poly[cur], poly[nxt]));
        if (sign == -1) recorder.record_circle(poly[cur], 1.0f, RED);
        else if (sign == 1) recorder.record_circle(poly[cur], 1.0f, GREEN);
        else recorder.record_circle(poly[cur], 1.0f, BLUE);
    }

    recorder.commit_step();
    recorder.run();

    return 0;
}
