#include "geom/geom.hpp"
#include "geom/basics.hpp"
#include "utils/AlgorithmRecorder.hpp"
#include "geom/triangulation.hpp"
#include "utils/generators/geometry.hpp"

int main() {
    int mn = -200, mx = 200;
    AlgorithmRecorder recorder(mn,mx,mn,mx);

    recorder.commit_step(); // empty start

    // Circles
    recorder.record_circle(random_pt(mn,mx), random_int(10,20));
    recorder.commit_step();
    recorder.record_circle(random_pt(mn,mx), random_int(10,20), CYAN, GREEN);
    recorder.commit_step();

    // Higlhigting points
    recorder.record_highlight(random_pt(mn,mx), 4);
    recorder.commit_step();
    recorder.record_highlight(random_pt(mn,mx), 4, ORANGE);
    recorder.commit_step();

    // Rectangles
    recorder.record_rectangle(random_pt(mn,mx), random_pt(mn,mx));
    recorder.commit_step();
    recorder.record_rectangle(random_pt(mn,mx), random_pt(mn,mx), TURQUOISE, SALMON);
    recorder.commit_step();

    // Points, lines and polygons
    recorder.record_line(random_pt(mn,mx), random_pt(mn,mx));
    recorder.record_line(random_pt(mn,mx), random_pt(mn,mx), RED);
    recorder.record_line(random_pt(mn,mx), random_pt(mn,mx), random_color());
    recorder.record_line(random_pt(mn,mx), random_pt(mn,mx), random_color());
    recorder.record_line(random_pt(mn,mx), random_pt(mn,mx), random_color());
    recorder.record_line(random_pt(mn,mx), random_pt(mn,mx), random_color());
    recorder.record_line(random_pt(mn,mx), random_pt(mn,mx), random_color());
    recorder.record_point(random_pt(mn,mx));
    recorder.record_point(random_pt(mn,mx), DARK_GREEN);
    recorder.commit_step();

    recorder.record_polygon(random_simple_polygon(10,mn,mx));
    recorder.record_polygon(random_simple_polygon(10,mn,mx), PINK, SKY_BLUE);
    recorder.commit_step();

    // TODO: graphs and text

    recorder.run();
    return 0;
}