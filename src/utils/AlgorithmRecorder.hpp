#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include "../geom/geom.hpp"
#include "./generators/primitives.hpp"
#include "./generators/geometry.hpp"

#include "../vulkan_renderer/Visualizer.hpp"
#include "colors.hpp"

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
        vector<pt<float>> points;
        glm::vec4 color;
        glm::vec4 secondary_color = TRANSPARENT;
        float radius = 0.0f;
        string text = "";

        Command(
            CommandType type_,
            vector<pt<float>> points_,
            glm::vec4 color_,
            glm::vec4 secondary_color_ = TRANSPARENT,
            float radius_ = 0.0f,
            string text_ = ""
        )
            : type(type_),
            points(move(points_)),
            color(color_),
            secondary_color(secondary_color_),
            radius(radius_),
            text(move(text_))
        {}
    };


    struct Frame {
        vector<Command> commands;
    };


    AlgorithmRecorder(
        float grid_minx, float grid_maxx, 
        float grid_miny, float grid_maxy, 
        int screenWidth = 1000, int screenHeight = 1000
    ) : grid_minx(grid_minx), grid_maxx(grid_maxx), grid_miny(grid_miny), grid_maxy(grid_maxy) {
        proj = glm::ortho(grid_minx, grid_maxx, grid_maxy, grid_miny, -1.0f, 1.0f);
        vis.init(screenWidth, screenHeight);
    }

    template<class T>
    void record_point(pt<T> p, glm::vec4 color = BLACK) {
        current_frame_commands.emplace_back(
            DRAW_POINT,
            vector<pt<float>>{to_float(p)},
            color
        );
    }

    template<class T>
    void record_line(pt<T> a, pt<T> b, glm::vec4 color = BLACK) {
        current_frame_commands.emplace_back(
            DRAW_LINE,
            vector<pt<float>>{to_float(a), to_float(b)},
            color
        );
    }

    template<class T>
    void record_polygon(
        const vector<pt<T>>& poly, 
        glm::vec4 fill_color = LIGHT_GRAY, 
        glm::vec4 line_color = BLACK, 
        glm::vec4 point_color = BLACK
    ) {
        vector<pt<float>> p2(sz(poly));
        for (int i = 0; i < (int)poly.size(); i++) p2[i] = to_float(poly[i]);

        for (int i = 0; i < (int)poly.size(); i++){
            current_frame_commands.emplace_back(DRAW_POINT, vector<pt<float>>{p2[i]}, point_color);
            current_frame_commands.emplace_back(DRAW_LINE, vector<pt<float>>{p2[i], p2[(i+1)%poly.size()]}, line_color);
        }
        current_frame_commands.emplace_back(DRAW_POLYGON, p2, fill_color);
    }

    template<class T>
    void record_highlight(const pt<T> &p, float base_radius, glm::vec4 color = RED) {
        current_frame_commands.emplace_back(DRAW_HIGHLIGHT, vector<pt<float>>{to_float(p)}, color, TRANSPARENT, base_radius);
    }

    template<class T>
    void record_circle(const pt<T> &center, float radius, glm::vec4 fill_color = LIGHT_GRAY, glm::vec4 border_color = BLACK) {
        current_frame_commands.emplace_back(DRAW_CIRCLE, vector<pt<float>>{to_float(center)}, fill_color, border_color, radius, "");
    }

    template<class T>
    void record_rectangle(const pt<T> &bottom_left, const pt<T> &top_right, glm::vec4 fill_color = LIGHT_GRAY, glm::vec4 border_color = BLACK) {
        current_frame_commands.emplace_back(DRAW_RECTANGLE, vector<pt<float>>{to_float(bottom_left), to_float(top_right)}, fill_color, border_color, 0.0f, "");
    }

    template<class T>
    void record_text(const string &text, const pt<T> &position, float font_size, glm::vec4 color = BLACK) {
        current_frame_commands.emplace_back(
            DRAW_TEXT, vector<pt<float>>{to_float(position)}, color, TRANSPARENT, font_size, text
        );
    }

    void record_log(string log_msg) {
        current_frame_commands.emplace_back(LOG, vector<pt<float>>{}, TRANSPARENT, TRANSPARENT, 0, log_msg);
    }


    // Graph drawing (for both general graphs and trees)

    void record_weighted_graph(const vector<vector<pair<int, int>>> &adj, const vector<pt<float>>& layout, const float node_radius, bool directed, bool bezier) {
        int n = sz(adj);

        for (int u = 0; u < n; u++) {
            for (auto [v,w] : adj[u]) {
                pt mid = draw_edge(layout, u, v, node_radius, directed, bezier || (u==v));
                auto ws = to_string(w);
                mid.x -= ws.size()*(node_radius/4.0);
                mid.y -= node_radius/4;
                record_text(ws, mid, 0.8*node_radius, BLACK);
            }
        }

        for (int i = 0; i < n; i++) {
            draw_graph_node(layout[i], node_radius, to_string(i));
        }
    }

    void record_unweighted_graph(const vector<vector<int>> &adj, const vector<pt<float>>& layout, const float node_radius, bool directed, bool bezier) {
        int n = sz(adj);

        for (int u = 0; u < n; u++) {
            for (int v : adj[u]) {
                draw_edge(layout, u, v, node_radius, directed, bezier || (u==v));
            }
        }

        for (int i = 0; i < n; i++) {
            draw_graph_node(layout[i], node_radius, to_string(i));
        }
    }

    // Tree layout

    pair<float, vector<pt<float>>> compute_tree_layout(const vector<vector<pair<int,int>>> &adj, int root){
        int n = sz(adj);
        vector<vector<int>> adj2(n);
        for (int u = 0; u < n; u++){
            for (auto [v,w] : adj[u]) {
                adj2[u].push_back(v);
            }
        }
        return compute_tree_layout(adj2,root);
    }

    pair<float, vector<pt<float>>> compute_tree_layout(const vector<vector<int>> &adj, int root){
        int n = sz(adj);
        if (n == 0) return {};

        float width = grid_maxx - grid_minx;
        float height = grid_maxy - grid_miny;
        const float node_radius = min(width/(3*n), height/(3*n));

        vector<pt<float>> pos(n);
        vector<int> dep(n), subtree_width(n);

        auto get_tree_width = [&](auto&& self, int u, int p) -> int {
            int width = 0;
            for (int v : adj[u]) if (v != p) {
                dep[v] = dep[u] + 1;
                width += self(self, v, u);
            }
            return subtree_width[u] = max(1, width);
        };
        get_tree_width(get_tree_width, root, -1);

        auto assign_pos = [&](auto& self, int u, int p, float minx, float maxx) -> void {
            pos[u].x = (minx + maxx)/2;
            pos[u].y = grid_maxy - 2*node_radius - 3*node_radius*dep[u];
            float x = minx;
            for (int v : adj[u]) if (v != p) {
                float slice = (maxx - minx) * ((ld)subtree_width[v] / subtree_width[u]);
                self(self, v, u, x, x + slice);
                x += slice;
            }
        };
        assign_pos(assign_pos, root, -1, 0.8*grid_minx, 0.8*grid_maxx);

        return {node_radius, pos};
    }


    // Graph layout

    pair<float, vector<pt<float>>> compute_graph_layout(const vector<vector<pair<int,int>>>& adj, GraphLayoutType layout_type) {
        int n = sz(adj);
        vector<vector<int>> adj2(n);
        for (int u = 0; u < n; u++){
            for (auto [v,w] : adj[u]) {
                adj2[u].push_back(v);
            }
        }
        return compute_graph_layout(adj2,layout_type);
    }

    pair<float, vector<pt<float>>> compute_graph_layout(const vector<vector<int>>& adj, GraphLayoutType layout_type) {
        int n = sz(adj);
        if (n == 0) return {};

        float width = grid_maxx - grid_minx;
        float height = grid_maxy - grid_miny;
        const float node_radius = min(width/(3*n), height/(3*n));

        vector<pt<float>> pos(n);

        auto compute_circular_layout = [&]() -> void {
            float r = 0.35 * (grid_maxy - grid_miny);

            for (int i = 0; i < (int)pos.size(); i++) {
                float ang = (2 * M_PI * i) / pos.size();
                pos[i] = pt(r * cos(ang), r * sin(ang));
            }
        };

        auto compute_bipartite_layout = [&]() -> void {
            vector<int> color(n, -1), set0, set1;
            auto dfs = [&](auto& dfs, int u, int c) -> void {
                color[u] = c;
                if (c == 0) set0.push_back(u);
                else set1.push_back(u);
                for (int v : adj[u]) {
                    if (color[v] == -1) {
                        dfs(dfs, v, 1 - c);
                    }
                }
            };

            for (int i = 0; i < n; i++) {
                if (color[i] == -1) {
                    dfs(dfs, i, 0);
                }
            }

            float x_left = 0.6*grid_minx;
            float x_right = 0.6*grid_maxx;

            float step0 = 0.9 * (grid_maxy - grid_miny) / set0.size();
            float y = step0 * set0.size() / 2.0;

            for (int i = 0; i < (int)set0.size(); i++) {
                pos[set0[i]] = pt(x_left, y);
                y -= step0;
            }

            float step1 = 0.9 * (grid_maxy - grid_miny) / set1.size();
            y = step1 * set1.size() / 2.0;

            for (int i = 0; i < (int)set1.size(); i++) {
                pos[set1[i]] = pt(x_right, y);
                y -= step1;
            }
        };
        
        auto compute_flow_network_layout = [&]() -> void {
            int source = 0;
            int sink = n - 1;

            // bfs p dividir em camadas
            vector<int> layer(n, -1);
            queue<int> q;
            layer[source] = 0;
            q.push(source);
            while (!q.empty()) {
                int u = q.front(); q.pop();
                for (int v : adj[u]) {
                    if (layer[v] == -1) {
                        layer[v] = layer[u] + 1;
                        q.push(v);
                    }
                }
            }

            int max_layer = *max_element(layer.begin(), layer.end());
            layer[sink] = max_layer; // force it

            vector<vector<int>> layers(max_layer + 1);

            for (int i = 0; i < n; i++) {
                if (i == source || i == sink) continue;
                if (layer[i] == -1)  // desconectado?
                    layer[i] = max_layer / 2;

                layers[layer[i]].push_back(i);
            }

            float center_y = (grid_miny + grid_maxy) / 2.0f;
            float left_x = grid_minx + 2.0f * node_radius;
            float right_x = grid_maxx - 2.0f * node_radius;

            pos[source] = pt(left_x, center_y);
            pos[sink] = pt(right_x, center_y);

            for (int l = 1; l < max_layer; l++) {
                if (layers[l].empty()) continue;

                float x = left_x + (right_x-left_x) * ((float)l / max_layer);

                float step = 0.8f * (grid_maxy - grid_miny) / layers[l].size();
                float y = center_y + step * (layers[l].size() - 1) / 2.0f;

                for (int u : layers[l]) {
                    pos[u] = pt(x, y);
                    y -= step;
                }
            }
        };

        auto compute_force_directed_layout = [&]() -> void {
            float area = width * height;
            
            for (auto &[x,y] : pos) {
                x = random_float(grid_minx, grid_maxx);
                y = random_float(grid_miny, grid_maxy);
            }
            float k = sqrt(area/((float)n));
            float t = max(width,height)/5.0;

            for (int iter = 0; iter < 2000; iter++) {
                vector<pt<float>> disp(n);

                // Repulsão
                for (int i = 0; i < n; i++) {
                    for (int j = 0; j < n; j++) {
                        if (i == j) continue;
                        float dx = pos[i].x - pos[j].x;
                        float dy = pos[i].y - pos[j].y;
                        float dist = sqrt(dx*dx + dy*dy);
                                            
                        if (dist < 1e-3f) { 
                            dx = random_float(-0.1f, 0.1f); 
                            dy = random_float(-0.1f, 0.1f); 
                            if (dx == 0 && dy == 0) dx = 0.1f;
                            dist = sqrt(dx*dx + dy*dy); 
                        }

                        float rep = 2 * (k * k) / dist;
                        disp[i].x += (dx / dist) * rep;
                        disp[i].y += (dy / dist) * rep;
                    }
                }

                // Atração por arestas
                for (int u = 0; u < n; u++) {
                    for (int v : adj[u]) {
                        if (u >= v) continue;
                        float dx = pos[u].x - pos[v].x;
                        float dy = pos[u].y - pos[v].y;
                        float dist = sqrt(dx*dx + dy*dy);

                        if (dist < 1e-3f) { 
                            dx = random_float(-0.1f, 0.1f); 
                            dy = random_float(-0.1f, 0.1f); 
                            if (dx == 0 && dy == 0) dx = 0.1f;
                            dist = sqrt(dx*dx + dy*dy); 
                        }

                        float attr = (dist * dist * 15) / k;
                        disp[u].x -= (dx / dist) * attr;
                        disp[u].y -= (dy / dist) * attr;
                        disp[v].x += (dx / dist) * attr;
                        disp[v].y += (dy / dist) * attr;
                    }
                }

                // Gravidade central p desgrudar da parede
                float center_x = (grid_minx + grid_maxx) / 2.0;
                float center_y = (grid_miny + grid_maxy) / 2.0;

                for (int i = 0; i < n; i++) {
                    float dx = center_x - pos[i].x;
                    float dy = center_y - pos[i].y;
                    float dist = sqrt(dx*dx + dy*dy);
                    if (dist > 1e-3) {
                        float attr = (dist * dist * 6  ) / k;
                        // Puxa suavemente para o centro da tela
                        disp[i].x += (dx / dist) * attr;
                        disp[i].y += (dy / dist) * attr;
                    }
                }

                // Repulsão entre nós e o meio das arestas, tentando evitar colinearidade
                for (int i = 0; i < n; i++) {
                    for (int u = 0; u < n; u++) {
                        for (int v : adj[u]) {
                            if (u >= v || i == u || i == v) continue;
                            
                            float mx = (pos[u].x + pos[v].x) / 2.0f;
                            float my = (pos[u].y + pos[v].y) / 2.0f;
                            
                            float dx = pos[i].x - mx;
                            float dy = pos[i].y - my;
                            float dist = sqrt(dx*dx + dy*dy);
                            
                            if (dist > 1e-3f && dist < k) { 
                                float rep = (k * k * 2.0) / dist;
                                disp[i].x += (dx / dist) * rep;
                                disp[i].y += (dy / dist) * rep;
                                float fx = (dx / dist) * rep;
                                float fy = (dy / dist) * rep;
                                disp[u].x -= fx / 2.0f;
                                disp[u].y -= fy / 2.0f;
                                disp[v].x -= fx / 2.0f;
                                disp[v].y -= fy / 2.0f;
                            }
                        }
                    }
                }

                // noise p deixar mais natural e sair de minimos locais
                for (int i = 0; i < n; i++) {
                    float noise_x = (random_float(-1.0f, 1.0f)) * (t * 0.05f);
                    float noise_y = (random_float(-1.0f, 1.0f)) * (t * 0.05f);
                    
                    disp[i].x += noise_x;
                    disp[i].y += noise_y;
                }
                
                for (int i = 0; i < n; i++) {
                    float disp_len = sqrt(disp[i].x * disp[i].x + disp[i].y * disp[i].y);
                    if (disp_len < 1e-5) continue;

                    float capped_len = min(disp_len, t);
                    pos[i].x += (disp[i].x / disp_len) * capped_len;
                    pos[i].y += (disp[i].y / disp_len) * capped_len;

                    pos[i].x = clamp(pos[i].x, grid_minx + 10.0f, grid_maxx - 10.0f);
                    pos[i].y = clamp(pos[i].y, grid_miny + 10.0f, grid_maxy - 10.0f);
                }

                t *= 0.999; // esfria
            }
        };

        if (layout_type == CIRCULAR) {
            compute_circular_layout();
        }
        else if (layout_type == BIPARTITE) {
            compute_bipartite_layout();
        }
        else if (layout_type == FLOW_NETWORK) {
            compute_flow_network_layout();
        }
        else if (layout_type == FORCE_DIRECTED) {
            compute_force_directed_layout();
        }

        return {node_radius, pos};
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

    void run(int frametime_ms = 500) {
        bool space_prev = false;
        bool enter_prev = false;
        bool r_prev = false;
        bool home_prev = false;
        bool end_prev = false;
        bool plus_prev = false;
        bool minus_prev = false;

        bool autoplay = false;
        double last_time = glfwGetTime();
        size_t last_frame = current_frame-1;

        auto handle_input = [&]() -> void {
            bool space = vis.is_key_pressed(GLFW_KEY_SPACE);
            bool shift = vis.is_key_pressed(GLFW_KEY_LEFT_SHIFT);
            bool enter = vis.is_key_pressed(GLFW_KEY_ENTER);
            bool r = vis.is_key_pressed(GLFW_KEY_R);
            bool home = vis.is_key_pressed(GLFW_KEY_HOME);
            bool end = vis.is_key_pressed(GLFW_KEY_END);
            bool esc = vis.is_key_pressed(GLFW_KEY_ESCAPE);
            bool plus = vis.is_key_pressed(GLFW_KEY_EQUAL);
            bool minus = vis.is_key_pressed(GLFW_KEY_MINUS);

            if (shift && space && !space_prev) {
                if (!timeline.empty()) {
                    current_frame = (current_frame + timeline.size() - 1) % timeline.size();
                }
            }
            else if (!shift && space && !space_prev) {
                if (!timeline.empty()) {
                    current_frame = (current_frame + 1) % timeline.size();
                }
            }

            if (enter && !enter_prev) {
                autoplay = !autoplay;
            }

            if (r && !r_prev) {
                current_frame = 0;
                autoplay = false;
            }

            if (home && !home_prev) {
                current_frame = 0;
                autoplay = false;
            }

            if (end && !end_prev) {
                if (!timeline.empty()) {
                    current_frame = timeline.size() - 1;
                }
                autoplay = false;
            }

            if (plus && !plus_prev) {
                frametime_ms = max(50, frametime_ms - 50);
                cout << "[Speed] " << frametime_ms << " ms" << endl;
            }

            if (minus && !minus_prev) {
                frametime_ms = min(5000, frametime_ms + 50);
                cout << "[Speed] " << frametime_ms << " ms" << endl;
            }

            if (esc) vis.stop();

            space_prev = space;
            enter_prev = enter;
            r_prev = r;
            home_prev = home;
            end_prev = end;
            plus_prev = plus;
            minus_prev = minus;
        };

        auto update_playback = [&]() -> void {
            if (autoplay && !timeline.empty()) {
                double cur_time = glfwGetTime();
                if (cur_time - last_time > frametime_ms / 1000.0) {
                    current_frame = (current_frame + 1) % timeline.size();
                    last_time = cur_time;
                }
            } else {
                last_time = glfwGetTime();
            }
        };

        auto render_command = [&](const Command& cmd) -> void {
            if (cmd.type == DRAW_POINT) {
                vis.draw_point(cmd.points[0], cmd.color);
            }
            else if (cmd.type == DRAW_LINE) {
                vis.draw_line(cmd.points[0], cmd.points[1], cmd.color);
            }
            else if (cmd.type == DRAW_POLYGON) {
                vis.draw_polygon(cmd.points, cmd.color);
            }
            else if (cmd.type == DRAW_HIGHLIGHT) {
                float pulse = (sin(8*glfwGetTime()) + 1)*0.5;
                float r = cmd.radius + (pulse * cmd.radius * 0.5);
                auto circle = get_circle_polygon(cmd.points[0], r);
                for (int i = 0; i < (int)circle.size(); i++){
                    pt p1 = circle[i];
                    pt p2 = circle[(i+1)%circle.size()];
                    vis.draw_line(p1, p2, cmd.color);
                }
            }
            else if (cmd.type == DRAW_CIRCLE) {
                auto circle = get_circle_polygon(cmd.points[0], cmd.radius);
                vis.draw_polygon(circle, cmd.color);
                for (int i = 0; i < (int)circle.size(); i++){
                    pt p1 = circle[i];
                    pt p2 = circle[(i+1)%circle.size()];
                    vis.draw_line(p1, p2, cmd.secondary_color);
                }
            }
            else if (cmd.type == DRAW_RECTANGLE) {
                pt bl = cmd.points[0];
                pt tr = cmd.points[1];
                vector<pt<float>> rect = { bl, pt(tr.x, bl.y), tr, pt(bl.x, tr.y) };
                vis.draw_polygon(rect, cmd.color);
                for (int s = 0; s < 4; s++) {
                    vis.draw_line(rect[s], rect[(s+1) % 4], cmd.secondary_color);
                }
            }
            else if (cmd.type == DRAW_TEXT) {
                vis.draw_text(cmd.text, cmd.points[0], cmd.radius, cmd.color);
            }
            else if (cmd.type == LOG) {
                if (last_frame != current_frame)
                    cout << "[LOG]: " << cmd.text << endl;
            }
        };

        auto render_current_frame = [&]() -> void {
            if (timeline.empty()) return;

            for (const auto& cmd : timeline[current_frame].commands) {
                render_command(cmd);
            }
        };

        auto render_hud = [&]() -> void {
            const float x = grid_minx;
            const float y = grid_miny;

            glm::vec4 hud_color = {0.02f, 0.02f, 0.02f, 0.92f};
            glm::vec4 border_color = {0.25f, 0.25f, 0.25f, 1.0f};
            glm::vec4 text_color = {1.0f, 1.0f, 1.0f, 1.0f};

            pt bl(x, y);
            pt tr(grid_maxx, y + 9.0f);

            // ccw
            vector<pt<float>> rect = {bl, pt(tr.x, bl.y), tr, pt(bl.x, tr.y)};

            vis.draw_polygon(rect, hud_color);

            for (int s = 0; s < 4; s++) {
                vis.draw_line(rect[s], rect[(s + 1) % 4], border_color);
            }

            if (autoplay) {
                vector<pt<float>> play = {
                    pt(x + 2.0f, y + 2.5f),
                    pt(x + 2.0f, y + 6.5f),
                    pt(x + 5.0f, y + 4.5f)
                };
                vis.draw_polygon(play, text_color);
            } else {
                vector<pt<float>> pause_left = {
                    pt(x + 2.0f, y + 2.5f),
                    pt(x + 3.0f, y + 2.5f),
                    pt(x + 3.0f, y + 6.5f),
                    pt(x + 2.0f, y + 6.5f)
                };

                vector<pt<float>> pause_right = {
                    pt(x + 4.0f, y + 2.5f),
                    pt(x + 5.0f, y + 2.5f),
                    pt(x + 5.0f, y + 6.5f),
                    pt(x + 4.0f, y + 6.5f)
                };

                vis.draw_polygon(pause_left, text_color);
                vis.draw_polygon(pause_right, text_color);
            }

            string frame = timeline.empty()
                ? "- / 0"
                : to_string(current_frame+1) + " / " + to_string(timeline.size());

            string speed = to_string(frametime_ms) + " ms";
            string caption = frame + "   " + speed;
            string commands = "   Space: next    Shift+Space: previous    Enter: play/pause    +/-: speed    R: restart    Esc: exit";
            vis.draw_text(caption, pt(x + 8.0f, y + 4.25f), 3.0f, text_color);
            vis.draw_text(commands, pt(x + 30.0f, y + 4.0f), 2.5f, text_color);
        };

        while (vis.is_running()) {
            handle_input();
            update_playback();
            vis.clear_buffers();
            render_current_frame();
            render_hud();
            vis.render_frame(proj);
            last_frame = current_frame;
        }
    }


    // Helper methods

    void draw_grid(float step) {
        glm::vec4 grid_color = {0.20f, 0.20f, 0.20f, 0.4f};
        for (float x = grid_minx; x <= grid_maxx; x += step) {
            if (x == 0.0f) continue;
            vis.draw_line(pt(x, grid_miny), pt(x, grid_maxy), grid_color);
        }
        for (float y = grid_miny; y <= grid_maxy; y += step) {
            if (y == 0.0f) continue;
            vis.draw_line(pt(grid_minx, y), pt(grid_maxx, y), grid_color);
        }
    }

    void draw_axis(){
        float half = 0.4f;
        for (float offset = -half; offset <= half; offset += 0.1f) {
            vis.draw_line(pt(grid_minx, offset), pt(grid_maxx, offset), RED);
        }
        for (float offset = -half; offset <= half; offset += 0.1f) {
            vis.draw_line(pt(offset, grid_miny), pt(offset, grid_maxy), GREEN);
        }
    }

    void draw_arrow_head(pt<float> from, pt<float> to, float node_radius, glm::vec4 color) {
        float angle = atan2(to.y-from.y, to.x - from.x);
        float len = node_radius/2;;
        pt tip = to;
        pt<float> left(tip.x - len*cos(angle - M_PI/10), tip.y - len * sin(angle - M_PI/10));
        pt<float> right(tip.x -len*cos(angle + M_PI/10), tip.y - len * sin(angle + M_PI/10));
        vector<pt<float>> tri = {tip, left, right};
        record_polygon(tri, color, color, TRANSPARENT);
    }

    pt<float> draw_edge(const vector<pt<float>> &pos, int u, int v, const float node_radius, bool directed, bool bezier, glm::vec4 color = BLACK){
        if (!bezier){
            ld ang = atan2(pos[v].y - pos[u].y, pos[v].x - pos[u].x);
            pt start = pos[u];
            start.x += node_radius*cos(ang);
            start.y += node_radius*sin(ang);
            pt end = pos[v];
            end.x -= node_radius*cos(ang);
            end.y -= node_radius*sin(ang);
            
            record_line(start, end, color);
            if (directed) draw_arrow_head(start, end, node_radius, color);
            return (pos[u]+pos[v])/2.0;
        }

        ld ang = atan2(pos[v].y - pos[u].y, pos[v].x - pos[u].x) + M_PI_2;
        ld d = dist(pos[u], pos[v]);
        pt mid = (pos[u] + pos[v])/2.0;

        pt ctrl = mid;
        ctrl.x += cos(ang) * 0.3 * d;
        ctrl.y += sin(ang) * 0.3 * d;

        for (int i = 0; i < (int)pos.size(); i++) {
            if (i == u || i == v) continue;
            
            ld dx = ctrl.x - pos[i].x;
            ld dy = ctrl.y - pos[i].y;
            ld d = sqrt(dx*dx + dy*dy);
            
            ld safe_dist = node_radius * 4.0f; 
            if (d < safe_dist && d > 1e-3f) {
                ld push = (safe_dist - d) * 0.8f;
                ctrl.x += (dx / d) * push;
                ctrl.y += (dy / d) * push;
            }
        }

        ld ang_start = atan2(ctrl.y - pos[u].y, ctrl.x - pos[u].x);
        pt start = pos[u];
        start.x += node_radius * cos(ang_start);
        start.y += node_radius * sin(ang_start);

        ld ang_end = atan2(pos[v].y - ctrl.y, pos[v].x - ctrl.x);
        pt end = pos[v];
        end.x -= node_radius * cos(ang_end);
        end.y -= node_radius * sin(ang_end);

        auto f = [&](ld t) -> pt<float> {
            return start*(1-t)*(1-t) + ctrl*2*(1-t)*t + end*t*t;
        };

        const int segments = 20;
        vector<pt<float>> curve(segments);
        curve[0] = start;
        curve[segments-1] = end;

        for (int i = 1; i < segments-1; i++){
            curve[i] = f(i / (segments-1.0));
        }

        for (int i = 0; i < segments-1; i++){
            record_line(curve[i], curve[i+1], color);
        }
        if (directed) draw_arrow_head(curve[segments-2], curve[segments-1], node_radius, color);
        
        return f(0.5); // mid point to draw weight
    }

    void draw_graph_node(pt<float> pos, float radius, string label, glm::vec4 color = {0.8, 0.9, 1.0, 1.0}) {
        record_circle(pos, radius, color, BLACK);
        pt mid = pos;
        mid.x -= label.size()*(radius/4.0);
        mid.y -= radius/4;
        record_text(label, mid, radius, BLACK);
    }

private:
    vector<Command> current_frame_commands;
    vector<Frame> timeline;
    size_t current_frame = 0;

    Visualizer vis;
    glm::mat4 proj;

    float grid_minx;
    float grid_maxx;
    float grid_miny;
    float grid_maxy;


    template<class T>
    static pt<float> to_float(const pt<T>& p) {
        return pt<float>(static_cast<float>(p.x), static_cast<float>(p.y));
    }
};
