#include "geom/geom.hpp"
#include "geom/basics.hpp"
#include "geom/triangulation.hpp"
#include "utils/generators/geometry.hpp"

int main() {
    int mn = -2000000, mx = 2000000;
    cout << "how many vertices you wanna triangulate?\n";
    int n; cin >> n;

    vector<pt> polygon = random_simple_polygon(n, mn, mx);

    auto start = chrono::steady_clock::now();
    triangulate(polygon);
    auto end = chrono::steady_clock::now();

    auto delta = chrono::duration_cast<chrono::milliseconds>(end - start);

    cout << "triangulate O(n^2): " << delta.count() << " ms\n";

    start = chrono::steady_clock::now();
    triangulate_deprecated(polygon);
    end = chrono::steady_clock::now();

    delta = chrono::duration_cast<chrono::milliseconds>(end - start);

    cout << "triangulate_deprecated O(n^3): " << delta.count() << " ms\n";

    return 0;
}
