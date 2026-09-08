#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include "../geom/geom.hpp"
#include "../vulkan_renderer/Visualizer.hpp"
#include "colors.hpp"
#include "./generators/primitives.hpp"
#include "./generators/geometry.hpp"
#include <bits/stdc++.h>

using namespace std;

class AlgorithmRecorder {
public:
    enum CommandType { 
        DRAW_POINT, 
        DRAW_LINE, 
        DRAW_POLYGON, 
        DRAW_HIGHLIGHT, 
        DRAW_CIRCLE, 
        DRAW_RECTANGLE, 
        DRAW_TEXT, 
        LOG 
    };

    enum GraphLayoutType { 
        CIRCULAR, 
        FORCE_DIRECTED,
        BIPARTITE,
        FLOW_NETWORK
    };

    struct Command {
        CommandType type;
        vector<pt> points;
        glm::vec4 color;
        glm::vec4 secondary_color = TRANSPARENT;
        float radius = 0.0f;
        string text = "";
    };

    struct Frame {
        vector<Command> commands;
    };

    vector<Frame> timeline;
    size_t current_frame = 0;

    AlgorithmRecorder(
        float grid_minx, float grid_maxx, 
        float grid_miny, float grid_maxy, 
        int screenWidth = 800, int screenHeight = 800
    );

    void record_point(pt p, glm::vec4 color = BLACK);
    void record_line(pt a, pt b, glm::vec4 color = BLACK);
    void record_polygon(
        const vector<pt>& poly, 
        glm::vec4 fill_color = LIGHT_GRAY, 
        glm::vec4 line_color = BLACK, 
        glm::vec4 point_color = BLACK
    );
    void record_highlight_point(pt p, glm::vec4 color = RED);
    void record_circle(pt center, float radius, glm::vec4 fill_color = LIGHT_GRAY, glm::vec4 border_color = BLACK);
    void record_rectangle(pt bottom_left, pt top_right, glm::vec4 fill_color = LIGHT_GRAY, glm::vec4 border_color = BLACK);
    void record_text(string text, pt position, glm::vec4 color = BLACK);
    void record_log(string log_msg);
    void record_weighted_graph(const vector<vector<pair<int, int>>> &adj, bool directed = false, GraphLayoutType layout_type = CIRCULAR);
    vector<pt> record_unweighted_graph(const vector<vector<int>> &adj, bool directed = false, GraphLayoutType layout_type = CIRCULAR);
    void record_tree(const vector<vector<int>> &adj, int root = 0);
    
    void commit_step();
    void clear();
    void run(int frametime_ms = 500);

private:
    vector<Command> current_frame_commands;
    Visualizer vis;
    glm::mat4 proj;
    float grid_minx, grid_maxx, grid_miny, grid_maxy;

    void draw_grid(float step = 20.0f);
    void draw_axis();
    vector<pt> compute_layout(int n, const vector<vector<pair<int, int>>>& adj, GraphLayoutType layout_type);
    void draw_arrow_head(pt from, pt to, ld node_radius);
    pt draw_edge(const vector<pt> &pos, int u, int v, const ld node_radius, bool directed, bool bezier);
};