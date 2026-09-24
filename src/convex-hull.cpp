#include "utils/AlgorithmRecorder.hpp"
#include "utils/generators/graph.hpp"
#include <bits/stdc++.h>

using namespace std;

#define ii pair<int,int>

using T = int;

int main() {
    int n; cin >> n;
    int mn = -100, mx = 100;
    vector<pt<T>> pts(n);
    for (auto &p : pts) p = random_pt(mn,mx);

    AlgorithmRecorder recorder(mn,mx,mn,mx);

    pts.push_back({-90,-90});
    pts.push_back({-80,-90});
    pts.push_back({-70,-90});
    n += 3;

    sort(pts.begin(),pts.end());
    pts.erase(unique(pts.begin(),pts.end()), pts.end());

    bool include_collinear = false;
    auto check = [&](pt<T> a, pt<T> b, pt<T> c) -> bool {
        if (include_collinear)
            return ccw(c,b,a);
        return !ccw(a,b,c);
    };

    auto draw_pt = [&](pt<T> p, glm::vec4 color){
        recorder.record_circle(p, 0.003*(mx-mn), color);
    };

    vector<pt<T>> lower, upper;
    auto draw_base = [&]() -> void {
        for (auto p : pts) draw_pt(p, BLACK);
        for (auto p : lower) draw_pt(p, SEAGREEN);
        for (auto p : upper) draw_pt(p, BLUE);
        for (int i = 0; i+1 < lower.size(); i++){
            recorder.record_line(lower[i], lower[i+1], SEAGREEN);
        }
        for (int i = 0; i+1 < upper.size(); i++){
            recorder.record_line(upper[i], upper[i+1], BLUE);
        }
    };
    draw_base();

    for (int i = 0; i < n; i++){
        while(lower.size() >= 2 && check(lower[lower.size()-2], lower[lower.size()-1], pts[i])){
            draw_base();
            draw_pt(lower[lower.size()-2], RED);
            draw_pt(lower[lower.size()-1], RED);
            draw_pt(pts[i], RED);
            recorder.record_line(lower[lower.size()-2], lower[lower.size()-1], RED);
            recorder.record_line(lower[lower.size()-1], pts[i], RED);
            recorder.commit_step();
            lower.pop_back();
        }
        lower.push_back(pts[i]);
        draw_base();
        recorder.commit_step();
    }
    for (int i = n-1; i >= 0; i--){
        while(upper.size() >= 2 && check(upper[upper.size()-2], upper[upper.size()-1], pts[i])){
            draw_base();
            draw_pt(upper[upper.size()-2], RED);
            draw_pt(upper[upper.size()-1], RED);
            draw_pt(pts[i], RED);
            recorder.record_line(upper[upper.size()-2], upper[upper.size()-1], RED);
            recorder.record_line(upper[upper.size()-1], pts[i], RED);
            recorder.commit_step();
            upper.pop_back();
        }
        upper.push_back(pts[i]);
    }
    upper.pop_back(); lower.pop_back();
    auto hull = lower;
    hull.insert(hull.end(), upper.begin(),upper.end());

    recorder.record_polygon(hull, TRANSPARENT, BLACK, BLACK);
    recorder.commit_step();

    recorder.run(500);
    return 0;
}
