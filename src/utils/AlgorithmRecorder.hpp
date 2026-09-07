#ifndef ALGORITHM_RECORDER_HPP
#define ALGORITHM_RECORDER_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include "../geom/geom.hpp"
#include "vulkan_renderer/Visualizer.hpp"
#include "colors.hpp"
#include "./generators/primitives.hpp"
#include "./generators/geometry.hpp"

class AlgorithmRecorder {
public:
    enum CommandType { 
        DRAW_POINT, 
        DRAW_LINE, 
        DRAW_POLYGON, 
        DRAW_HIGHLIGHT_POINT, 
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

    AlgorithmRecorder(
        float grid_minx, float grid_maxx, 
        float grid_miny, float grid_maxy, 
        int screenWidth = 800, int screenHeight = 800
    ) : grid_minx(grid_minx), grid_maxx(grid_maxx), grid_miny(grid_miny), grid_maxy(grid_maxy) {
        proj = glm::ortho(grid_minx, grid_maxx, grid_maxy, grid_miny, -1.0f, 1.0f);
        vis.init(screenWidth, screenHeight);
    }

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

    void record_point(pt p, glm::vec4 color = BLACK) {
        current_frame_commands.push_back({DRAW_POINT, {p}, color});
    }

    void record_line(pt a, pt b, glm::vec4 color = BLACK) {
        current_frame_commands.push_back({DRAW_LINE, {a, b}, color});
    }

    void record_polygon(
        const vector<pt>& poly, 
        glm::vec4 fill_color = LIGHT_GRAY, 
        glm::vec4 line_color = BLACK, 
        glm::vec4 point_color = BLACK
    ) {
        for (int i = 0; i < (int)poly.size(); i++){
            current_frame_commands.push_back({DRAW_POINT, {poly[i]}, point_color});
            current_frame_commands.push_back({DRAW_LINE, {poly[i], poly[(i+1)%poly.size()]}, line_color});
        }
        current_frame_commands.push_back({DRAW_POLYGON, vector<pt>(poly.begin(),poly.end()), fill_color});
    }

    void record_highlight_point(pt p, glm::vec4 color = RED) {
        current_frame_commands.push_back({DRAW_HIGHLIGHT_POINT, {p}, color});
    }

    void record_circle(pt center, float radius, glm::vec4 fill_color = LIGHT_GRAY, glm::vec4 border_color = BLACK) {
        current_frame_commands.push_back({DRAW_CIRCLE, {center}, fill_color, border_color, radius, ""});
    }

    void record_rectangle(pt bottom_left, pt top_right, glm::vec4 fill_color = LIGHT_GRAY, glm::vec4 border_color = BLACK) {
        current_frame_commands.push_back({DRAW_RECTANGLE, {bottom_left, top_right}, fill_color, border_color, 0.0f, ""});
    }

    void record_text(string text, pt position, glm::vec4 color = BLACK) {
        // TODO: deal with text rendering
        current_frame_commands.push_back({
            DRAW_TEXT, {position}, color, TRANSPARENT, 0.0f, text
        });
    }

    void record_log(string log_msg) {
        current_frame_commands.push_back({LOG, {}, TRANSPARENT, TRANSPARENT, 0, log_msg});
    }

    void record_weighted_graph(const vector<vector<pair<int, int>>> &adj, bool directed = false, GraphLayoutType layout_type = CIRCULAR) {
        int n = sz(adj);
        if (n == 0) return;
        vector<pt> pos = compute_layout(n, adj, layout_type);

        for (int u = 0; u < n; u++) {
            for (auto& edge : adj[u]) {
                int v = edge.first;
                int weight = edge.second;
                record_line(pos[u], pos[v], {0.4f, 0.4f, 0.4f, 0.8f});
                
                if (directed) {
                    draw_arrow_head(pos[u], pos[v], {0.4f, 0.4f, 0.4f, 0.8f});
                }

                pt mid((pos[u].x + pos[v].x) / 2, (pos[u].y + pos[v].y) / 2);
                record_text(to_string(weight), mid, BLACK);
            }
        }

        for (int i = 0; i < n; i++) {
            record_circle(pos[i], 6.0f, {0.8f, 0.9f, 1.0f, 1.0f}, BLACK);
            record_text(to_string(i), pos[i], BLACK);
        }
    }

    void record_unweighted_graph(const vector<vector<int>> &adj, bool directed = false, GraphLayoutType layout_type = CIRCULAR) {
        int n = sz(adj);
        if (n == 0) return;
        
        vector<vector<pair<int, int>>> weighted_adj(n);
        for (int u = 0; u < n; u++) {
            for (int v : adj[u]) {
                weighted_adj[u].push_back({v, 1});
            }
        }
        
        vector<pt> pos = compute_layout(n, weighted_adj, layout_type);

        for (int u = 0; u < n; u++) {
            for (int v : adj[u]) {
                record_line(pos[u], pos[v], {0.4f, 0.4f, 0.4f, 0.8f});
                if (directed) {
                    draw_arrow_head(pos[u], pos[v], {0.4f, 0.4f, 0.4f, 0.8f});
                }
            }
        }

        for (int i = 0; i < n; i++) {
            record_circle(pos[i], 6.0f, {0.8f, 0.9f, 1.0f, 1.0f}, BLACK);
            record_text(to_string(i), pos[i], BLACK);
        }
    }

    void record_tree(const vector<vector<int>> &adj, int root = 0) {
        int n = sz(adj);
        if (n == 0) return;

        vector<pt> pos(n);
        vector<int> dep(n, 0);
        vector<int> subtree_width(n, 0);

        auto compute_tree_metrics = [&](auto&& self, int u, int p, int d) -> int {
            dep[u] = d;
            int width = 0;
            for (int v : adj[u]) if (v != p) {
                width += self(self, v, u, d + 1);
            }
            subtree_width[u] = max(1, width);
            return subtree_width[u];
        };
        compute_tree_metrics(compute_tree_metrics, root, -1, 0);

        auto assign_positions = [&](auto& self, int u, int p, float x_low, float x_high) -> void {
            float x_mid = (x_low + x_high)/2;
            float y_pos = 30*dep[u] - 60;
            pos[u] = pt(x_mid, y_pos);

            float x = x_low;
            for (int v : adj[u]) if (v != p) {
                float slice = (x_high - x_low) * ((ld)subtree_width[v] / subtree_width[u]);
                self(self, v, u, x, x + slice);
                x += slice;
            }
        };
        assign_positions(assign_positions, root, -1, -80.0f, 80.0f);

        auto draw_edges = [&](auto& self, int u, int p) -> void {
            for (int v : adj[u]) if (v != p) {
                record_line(pos[u], pos[v], BLACK);
                self(self, v, u);
            }
        };
        draw_edges(draw_edges, root, -1);

        for (int i = 0; i < n; i++) {
            record_circle(pos[i], 6.0f, {0.9f, 0.8f, 1.0f, 1.0f}, BLACK);
            record_text(to_string(i), pos[i], BLACK);
        }
    }


    void commit_step() {
        timeline.push_back({current_frame_commands});
        current_frame_commands.clear();
    }

    void clear() {
        timeline.clear();
        current_frame_commands.clear();
        current_frame = 0;
    }

    void run() {
        bool space_prev = false;
        bool enter_prev = false;
        bool autoplay = false;
        double last_time = glfwGetTime();

        while (vis.is_running()) {
            vis.clear_buffers();
            // draw_grid(20.0f);

            bool space = vis.is_key_pressed(GLFW_KEY_SPACE);
            bool enter = vis.is_key_pressed(GLFW_KEY_ENTER);

            if (!autoplay && space && !space_prev) {
                if (!timeline.empty()) {
                    // TODO: debug multiple outputs, why is that happening
                    current_frame = (current_frame + 1) % timeline.size();
                    autoplay = false;
                    cout << "[Passo Manual] Frame " << current_frame + 1 
                            << " / " << timeline.size() << endl;
                }
            }
            space_prev = space;

            if (enter && !enter_prev) {
                autoplay = !autoplay;
                cout << (autoplay ? "[Play] Rodando automaticamente..." : "[Pause]") << endl;
            }
            enter_prev = enter;

            if (autoplay && !timeline.empty()) {
                double cur_time = glfwGetTime();
                if (cur_time - last_time > 0.8) {
                    current_frame = (current_frame + 1) % timeline.size();
                    last_time = cur_time;
                }
            } else {
                last_time = glfwGetTime();
            }

            if (!timeline.empty()) {
                for (const auto& cmd : timeline[current_frame].commands) {
                    if (cmd.type == DRAW_POINT) {
                        vis.draw_point(cmd.points[0], cmd.color);
                    } else if (cmd.type == DRAW_LINE) {
                        vis.draw_line(cmd.points[0], cmd.points[1], cmd.color);
                    } else if (cmd.type == DRAW_POLYGON) {
                        vis.draw_polygon(cmd.points, cmd.color);
                    } else if (cmd.type == DRAW_HIGHLIGHT_POINT) {
                        vis.draw_point(cmd.points[0], cmd.color);
                        float pulse = (sin((float)glfwGetTime() * 8.0f) + 1.0f) * 0.5f;
                        float r = 4.0f + (pulse * 3.0f);
                        int segments = 20;
                        for (int s = 0; s < segments; s++) {
                            float a1 = 2*M_PI * s/segments;
                            float a2 = 2*M_PI * (s+1)/segments;
                            pt p1(cmd.points[0].x + r*cos(a1), cmd.points[0].y + r*sin(a1));
                            pt p2(cmd.points[0].x + r*cos(a2), cmd.points[0].y + r*sin(a2));
                            vis.draw_line(p1, p2, RED);
                        }
                    } else if (cmd.type == DRAW_CIRCLE) {
                        int segments = 20;
                        pt center = cmd.points[0];
                        float r = cmd.radius;

                        vector<pt> circle_poly;
                        for (int s = 0; s < segments; s++) {
                            float ang = 2*M_PI * s/segments;
                            circle_poly.push_back(pt(center.x + r*cos(ang), center.y + r*sin(ang)));
                        }
                        vis.draw_polygon(circle_poly, cmd.color);
                        
                        for (int s = 0; s < segments; s++) {
                            vis.draw_line(circle_poly[s], circle_poly[(s+1)%segments], cmd.secondary_color);
                        }
                    } else if (cmd.type == DRAW_RECTANGLE) {
                        pt bl = cmd.points[0];
                        pt tr = cmd.points[1];
                        vector<pt> rect = { bl, pt(tr.x, bl.y), tr, pt(bl.x, tr.y) };
                        vis.draw_polygon(rect, cmd.color);
                        for (int s = 0; s < 4; s++) {
                            vis.draw_line(rect[s], rect[(s + 1) % 4], cmd.secondary_color);
                        }
                    } else if (cmd.type == DRAW_TEXT) {
                        // TODO: deal with text rendering
                    } else if (cmd.type == LOG) {
                        cout << "[LOG]: " << cmd.text << endl;
                    }
                }
            }

            vis.render_frame(proj);
        }
    }

private:
    vector<Command> current_frame_commands;
    Visualizer vis;
    glm::mat4 proj;
    float grid_minx, grid_maxx, grid_miny, grid_maxy;

    void draw_grid(float step = 20.0f, float axis_thickness = 1.0f) {
        glm::vec4 grid_color = {0.20f, 0.20f, 0.20f, 0.4f};
        for (float x = grid_minx; x <= grid_maxx; x += step) {
            if (x == 0.0f) continue;
            vis.draw_line(pt(x, grid_miny), pt(x, grid_maxy), grid_color);
        }
        for (float y = grid_miny; y <= grid_maxy; y += step) {
            if (y == 0.0f) continue;
            vis.draw_line(pt(grid_minx, y), pt(grid_maxx, y), grid_color);
        }

        float half_thick = axis_thickness / 2.0f;
        for (float offset = -half_thick; offset <= half_thick; offset += 0.1f) {
            vis.draw_line(pt(grid_minx, offset), pt(grid_maxx, offset), {1,0,0,1});
        }
        for (float offset = -half_thick; offset <= half_thick; offset += 0.1f) {
            vis.draw_line(pt(offset, grid_miny), pt(offset, grid_maxy), {0,1,0,1});
        }
    }

    vector<pt> compute_layout(int n, const vector<vector<pair<int, int>>>& adj, GraphLayoutType layout_type) {
        vector<pt> pos(n);
        if (layout_type == CIRCULAR) {
            float r = 70.0f;
            for (int i = 0; i < n; i++) {
                float ang = (2*M_PI * i) / n;
                pos[i] = pt(r*cos(ang), r*sin(ang));
            }
        } else if (layout_type == BIPARTITE) {
            vector<int> color(n, -1), set0, set1;

            auto dfs = [&](auto& self, int u, int c) -> void {
                color[u] = c;
                if (c == 0) set0.push_back(u);
                else set1.push_back(u);
                for (auto& [v,w] : adj[u]) {
                    if (color[v] == -1) self(self, v, 1 - c);
                }
            };

            for (int i = 0; i < n; i++) {
                if (color[i] == -1) dfs(dfs, i, 0);
            }

            float x_left = -60.0f, x_right = 60.0f;
            for (int i = 0; i < (int)set0.size(); i++) {
                float y = (i - set0.size()/2.0f) * 30.0f;
                pos[set0[i]] = pt(x_left, y);
            }
            for (int i = 0; i < (int)set1.size(); i++) {
                float y = (i - set1.size()/2.0f) * 30.0f;
                pos[set1[i]] = pt(x_right, y);
            }
        }
        else if (layout_type == FLOW_NETWORK) {
            // // Source = 0, Sink = n - 1.
            // pos[0] = pt(-80.0f, 0.0f);
            // pos[n-1] = pt(80.0f, 0.0f);

            // vector<int> color(n, -1);
            // vector<int> set0, set1;
            // color[0] = 0; // Source começa no grupo 0

            // auto dfs = [&](auto& self, int u, int c) -> void {
            //     if (u != 0 && u != n - 1) {
            //         if (c == 0) set0.push_back(u);
            //         else set1.push_back(u);
            //     }
            //     for (auto& edge : adj[u]) {
            //         int v = edge.first;
            //         if (v == 0 || v == n - 1) continue;
            //         if (color[v] == -1) {
            //             color[v] = 1 - c;
            //             self(self, v, 1 - c);
            //         }
            //     }
            // };

            // for (int i = 1; i < n - 1; i++) {
            //     if (color[i] == -1) {
            //         color[i] = 0;
            //         dfs(dfs, i, 0);
            //     }
            // }

            // // Posiciona o grupo 0 na coluna intermediária esquerda (-30.0f) e o grupo 1 na direita (+30.0f)
            // for (size_t i = 0; i < set0.size(); i++) {
            //     float y = (float)((int)i - (int)set0.size() / 2.0f) * 30.0f;
            //     pos[set0[i]] = pt(-30.0f, y);
            // }
            // for (size_t i = 0; i < set1.size(); i++) {
            //     float y = (float)((int)i - (int)set1.size() / 2.0f) * 30.0f;
            //     pos[set1[i]] = pt(30.0f, y);
            // }
        }
        else {
            for (pt &p : pos) p = random_pt(-50,50);
            for (int iter = 0; iter < 40; iter++) {
                vector<pt> disp(n);
                float k = sqrt(10000.0f / n);
                for (int i = 0; i < n; i++) {
                    for (int j = 0; j < n; j++) {
                        if (i == j) continue;
                        float dx = pos[i].x - pos[j].x;
                        float dy = pos[i].y - pos[j].y;
                        float dist = sqrt(dx*dx + dy*dy) + 0.01f;
                        float rep = (k * k) / dist;
                        disp[i].x += (dx / dist) * rep;
                        disp[i].y += (dy / dist) * rep;
                    }
                }
                for (int u = 0; u < n; u++) {
                    for (auto& [v,w] : adj[u]) {
                        float dx = pos[u].x - pos[v].x;
                        float dy = pos[u].y - pos[v].y;
                        float dist = sqrt(dx*dx + dy*dy) + 0.01f;
                        float attr = (dist * dist) / k;
                        disp[u].x -= (dx / dist) * attr;
                        disp[u].y -= (dx / dist) * attr;
                        disp[v].x += (dx / dist) * attr;
                        disp[v].y += (dx / dist) * attr;
                    }
                }
                for (int i = 0; i < n; i++) {
                    pos[i].x += disp[i].x * 0.2f;
                    pos[i].y += disp[i].y * 0.2f;
                }
            }
        }
        return pos;
    }

    void draw_arrow_head(pt from, pt to, glm::vec4 color) {
        double angle = atan2(to.y-from.y, to.x - from.x);
        double len = 8.0;
        pt tip = to - pt(10*cos(angle), 10*sin(angle));
        pt left(tip.x - len*cos(angle - M_PI/6), tip.y - len * sin(angle - M_PI/6));
        pt right(tip.x -len*cos(angle + M_PI/6), tip.y - len * sin(angle + M_PI/6));
        record_polygon({tip, left, right}, color, color, color);
    }
};

#endif