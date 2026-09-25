#include "utils/AlgorithmRecorder.hpp"
#include "utils/generators/graph.hpp"
#include <bits/stdc++.h>

using namespace std;

#define ii pair<int,int>

using T = int;

int main() {
    cout << "How many vertices?\n";
    int n; cin >> n; n-=5;

    cout << "Include collinear in hull? (0/1)\n";
    bool include_collinear; cin >> include_collinear;
    int mn = -100, mx = 100;
    vector<pt<T>> pts(n);
    for (auto &p : pts) p = random_pt(9*mn/10,9*mx/10);

    AlgorithmRecorder recorder(mn,mx,mn,mx);

    pts.push_back({-90,-80});
    pts.push_back({-90,-70});
    pts.push_back({-90,-90});
    pts.push_back({-80,-90});
    pts.push_back({-70,-90});
    n += 5;

    sort(pts.begin(),pts.end());
    pts.erase(unique(pts.begin(),pts.end()), pts.end());

    sort(pts.begin()+1, pts.end(), [&] (pt<T> a, pt<T> b) -> bool {
        if (!col(pts[0], a, b)) return ccw(pts[0],a,b);
        return dist2(pts[0],a) < dist2(pts[0],b);
    });
    int j = n-1;
    while(j>=0 && col(pts[0],pts[n-1],pts[j])) j--;

    reverse(pts.begin()+j+1,pts.end());

    auto check = [&](pt<T> a, pt<T> b, pt<T> c) -> bool {
        if (include_collinear)
            return ccw(c,b,a);
        return !ccw(a,b,c);
    };

    auto draw_pt = [&](pt<T> p, glm::vec4 color){
        recorder.record_circle(p, 0.003*(mx-mn), color);
    };

    vector<pt<T>> hull;
    auto draw_base = [&]() -> void {
        for (auto p : pts) draw_pt(p, BLACK);
        for (auto p : hull) draw_pt(p, BLUE);
        for (int i = 0; i+1 < hull.size(); i++){
            recorder.record_line(hull[i], hull[i+1], BLUE);
        }
    };
    draw_base();

    for (int i = 0; i < n; i++){
        while(hull.size()>=2 && check(hull[hull.size()-2], hull[hull.size()-1], pts[i])){
            draw_base();
            recorder.record_line(hull[hull.size()-2], hull[hull.size()-1], RED);
            recorder.record_line(hull[hull.size()-1], pts[i], RED);
            draw_pt(hull[hull.size()-2], RED);
            draw_pt(hull[hull.size()-1], RED);
            draw_pt(pts[i], RED);
            recorder.commit_step();
            hull.pop_back();
        }
        hull.push_back(pts[i]);
        draw_base();
        recorder.commit_step();
    }

    for (auto p : pts) draw_pt(p, BLACK);
    for (int i = 0; i < hull.size(); i++){
        draw_pt(hull[i], DARK_GREEN);
        recorder.record_line(hull[i], hull[(i+1)%hull.size()], DARK_GREEN);
    }

    recorder.commit_step();

    recorder.run(500);
    return 0;
}
