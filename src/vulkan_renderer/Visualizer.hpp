#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "VkBootstrap.h"
#include <GLFW/glfw3.h>
#include "../geom/triangulation.hpp"

struct Vertex {
    glm::vec2 pos;
    glm::vec4 color;
    glm::vec2 uv;
};

class Visualizer {
public:
    void init(int width = 800, int height = 800);
    void cleanup();
    bool is_running();
    void clear_buffers();
    void render_frame(glm::mat4 proj);

    ~Visualizer() { cleanup(); }

    void draw_point(pt<float> p, glm::vec4 color);
    void draw_line(pt<float> a, pt<float> b, glm::vec4 color);
    void draw_polygon(const std::vector<pt<float>>& poly, glm::vec4 color);
    void draw_text(const std::string& text, pt<float> pos, float font_size, glm::vec4 color);
    bool is_key_pressed(int key);
    void stop() { running = false; }
private:
    std::vector<Vertex> text_vertices;

    std::vector<Vertex> points;
    std::vector<Vertex> lines;
    std::vector<Vertex> triangles;

    struct VulkanState* vkState = nullptr; 

    void init_pipelines();
    void init_render_resources();
    void init_text_pipeline();

    bool running = true;
};