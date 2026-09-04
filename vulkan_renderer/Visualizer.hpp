#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "VkBootstrap.h"
#include <GLFW/glfw3.h>

struct Vertex {
    glm::vec2 pos;
    glm::vec4 color;
};

class Visualizer {
public:
    void init(int width = 800, int height = 800);
    void cleanup();
    bool is_running();
    void clear_buffers();
    void render_frame(glm::mat4 proj);

    ~Visualizer() { cleanup(); }

    template<typename Pt>
    void draw_point(Pt p, glm::vec4 color) {
        points.push_back({glm::vec2((float)p.x, (float)p.y), color});
    }

    template<typename Pt>
    void draw_line(Pt a, Pt b, glm::vec4 color) {
        lines.push_back({glm::vec2((float)a.x, (float)a.y), color});
        lines.push_back({glm::vec2((float)b.x, (float)b.y), color});
    }

    template<typename Pt>
    void draw_polygon(const std::vector<Pt>& poly, glm::vec4 color) {
        if (poly.size() < 3) return;
        for (auto t : triangulate(poly)) {
            auto [p1,p2,p3] = t;
            triangles.push_back({glm::vec2(p1.x, p1.y), color});
            triangles.push_back({glm::vec2(p2.x, p2.y), color});
            triangles.push_back({glm::vec2(p3.x, p3.y), color});
        }
    }

    bool is_key_pressed(int key);
private:
    std::vector<Vertex> points;
    std::vector<Vertex> lines;
    std::vector<Vertex> triangles;

    struct VulkanState* vkState = nullptr; 

    void init_pipelines();
    void init_render_resources();
};