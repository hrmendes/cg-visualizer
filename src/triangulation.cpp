#include "geom/geom.hpp"
#include "geom/basics.hpp"
#include "utils/AlgorithmRecorder.hpp"
#include "geom/triangulation.hpp"
#include "utils/generators/geometry.hpp"

int main() {
    int mn = -200, mx = 200;
    cout << "how many vertices you wanna triangulate?\n";
    int n; cin >> n;
    vector<pt> polygon = random_simple_polygon(n,mn,mx);
    AlgorithmRecorder recorder(mn,mx,mn,mx, 1000,1000);
    // polarea(polygon, recorder); // algoritmo pra rodar
    triangulate(polygon, &recorder);
    recorder.run();
    return 0;
}