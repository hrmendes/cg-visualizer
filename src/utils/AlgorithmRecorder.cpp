#include "AlgorithmRecorder.hpp"

AlgorithmRecorder::AlgorithmRecorder(
    float grid_minx, float grid_maxx, 
    float grid_miny, float grid_maxy, 
    int screenWidth, int screenHeight
) : grid_minx(grid_minx), grid_maxx(grid_maxx), grid_miny(grid_miny), grid_maxy(grid_maxy) {
    proj = glm::ortho(grid_minx, grid_maxx, grid_maxy, grid_miny, -1.0f, 1.0f);
    vis.init(screenWidth, screenHeight);
}

void AlgorithmRecorder::record_point(pt p, glm::vec4 color) {
    current_frame_commands.push_back({DRAW_POINT, {p}, color});
}

void AlgorithmRecorder::record_line(pt a, pt b, glm::vec4 color) {
    current_frame_commands.push_back({DRAW_LINE, {a, b}, color});
}

void AlgorithmRecorder::record_polygon(
    const vector<pt>& poly, 
    glm::vec4 fill_color, 
    glm::vec4 line_color, 
    glm::vec4 point_color
) {
    for (int i = 0; i < (int)poly.size(); i++){
        current_frame_commands.push_back({DRAW_POINT, {poly[i]}, point_color});
        current_frame_commands.push_back({DRAW_LINE, {poly[i], poly[(i+1)%poly.size()]}, line_color});
    }
    current_frame_commands.push_back({DRAW_POLYGON, vector<pt>(poly.begin(),poly.end()), fill_color});
}

void AlgorithmRecorder::record_highlight_point(pt p, glm::vec4 color) {
    current_frame_commands.push_back({DRAW_HIGHLIGHT_POINT, {p}, color});
}

void AlgorithmRecorder::record_circle(pt center, float radius, glm::vec4 fill_color, glm::vec4 border_color) {
    current_frame_commands.push_back({DRAW_CIRCLE, {center}, fill_color, border_color, radius, ""});
}

void AlgorithmRecorder::record_rectangle(pt bottom_left, pt top_right, glm::vec4 fill_color, glm::vec4 border_color) {
    current_frame_commands.push_back({DRAW_RECTANGLE, {bottom_left, top_right}, fill_color, border_color, 0.0f, ""});
}

void AlgorithmRecorder::record_text(string text, pt position, glm::vec4 color) {
    // TODO: deal with text rendering
    current_frame_commands.push_back({
        DRAW_TEXT, {position}, color, TRANSPARENT, 0.0f, text
    });
}

void AlgorithmRecorder::record_log(string log_msg) {
    current_frame_commands.push_back({LOG, {}, TRANSPARENT, TRANSPARENT, 0, log_msg});
}

void AlgorithmRecorder::record_weighted_graph(const vector<vector<pair<int, int>>> &adj, bool directed, GraphLayoutType layout_type) {
    int n = sz(adj);
    if (n == 0) return;
    vector<pt> pos = compute_layout(n, adj, layout_type);

    for (int u = 0; u < n; u++) {
        for (auto& edge : adj[u]) {
            int v = edge.first;
            int weight = edge.second;

            // TODO: draw quadratic bezier for this
            record_line(pos[u], pos[v]);
            
            if (directed) draw_arrow_head(pos[u], pos[v]);

            pt mid((pos[u].x + pos[v].x) / 2, (pos[u].y + pos[v].y) / 2);
            record_text(to_string(weight), mid, BLACK);
        }
    }

    const float node_radius = min((grid_maxx-grid_minx)/(2*n), (grid_maxy-grid_miny)/(2*n));
    for (int i = 0; i < n; i++) {
        record_circle(pos[i], node_radius, {0.8f, 0.9f, 1.0f, 1.0f}, BLACK);
        record_text(to_string(i), pos[i], BLACK);
    }
}

vector<pt> AlgorithmRecorder::record_unweighted_graph(const vector<vector<int>> &adj, bool directed, GraphLayoutType layout_type) {
    int n = sz(adj);
    if (n == 0) return {};
    
    vector<vector<pair<int, int>>> weighted_adj(n);
    for (int u = 0; u < n; u++) {
        for (int v : adj[u]) {
            weighted_adj[u].push_back({v, 1});
        }
    }
    
    vector<pt> pos = compute_layout(n, weighted_adj, layout_type);

    for (int u = 0; u < n; u++) {
        for (int v : adj[u]) {
            // TODO: draw quadratic bezier for this
            record_line(pos[u], pos[v]);
            if (directed) draw_arrow_head(pos[u], pos[v]);
        }
    }

    const float node_radius = min((grid_maxx-grid_minx)/(2*n), (grid_maxy-grid_miny)/(2*n));
    for (int i = 0; i < n; i++) {
        record_circle(pos[i], node_radius, {0.8f, 0.9f, 1.0f, 1.0f}, BLACK);
        record_text(to_string(i), pos[i], BLACK);
    }
    return pos;
}

void AlgorithmRecorder::record_tree(const vector<vector<int>> &adj, int root) {
    int n = sz(adj);
    if (n == 0) return;

    const float node_radius = min((grid_maxx-grid_minx)/(3*n), (grid_maxy-grid_miny)/(3*n));

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

    auto assign_positions = [&](auto& self, int u, int p, float minx, float maxx) -> void {
        pos[u].x = (minx + maxx)/2;
        pos[u].y = grid_maxy - 2*node_radius - 3*node_radius*dep[u];

        float x = minx;
        for (int v : adj[u]) if (v != p) {
            float slice = (maxx - minx) * ((ld)subtree_width[v] / subtree_width[u]);
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
        record_circle(pos[i], node_radius, {0.8f, 0.9f, 1.0f, 1.0f}, BLACK);
        record_text(to_string(i), pos[i], BLACK);
    }
}

void AlgorithmRecorder::commit_step() {
    timeline.push_back({current_frame_commands});
    current_frame_commands.clear();
}

void AlgorithmRecorder::clear() {
    timeline.clear();
    current_frame_commands.clear();
    current_frame = 0;
}

void AlgorithmRecorder::run() {
    bool space_prev = false;
    bool enter_prev = false;
    bool autoplay = false;
    double last_time = glfwGetTime();

    while (vis.is_running()) {
        vis.clear_buffers();
        // draw_axis();
        // draw_grid();

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
            if (cur_time - last_time > 0.5) {
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

void AlgorithmRecorder::draw_grid(float step) {
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

void AlgorithmRecorder::draw_axis(){
    float half = 0.4f;
    for (float offset = -half; offset <= half; offset += 0.1f) {
        vis.draw_line(pt(grid_minx, offset), pt(grid_maxx, offset), RED);
    }
    for (float offset = -half; offset <= half; offset += 0.1f) {
        vis.draw_line(pt(offset, grid_miny), pt(offset, grid_maxy), GREEN);
    }
}

vector<pt> AlgorithmRecorder::compute_layout(int n, const vector<vector<pair<int, int>>>& adj, GraphLayoutType layout_type) {
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
    else if (layout_type == FORCE_DIRECTED) {
        ld width = grid_maxx - grid_minx;
        ld height = grid_maxy - grid_miny;
        ld area = width * height;
        
        for (auto &[x,y] : pos) {
            x = random_float(grid_minx, grid_maxx);
            y = random_float(grid_miny, grid_maxy);
        }
        ld k = sqrt(area/((ld)n));
        ld t = max(width,height)/5.0;


        for (int u = 0; u < n; u++) {
            for (auto [v,w] : adj[u]) {
                record_line(pos[u], pos[v]);
            }
        }

        const float node_radius = min((grid_maxx-grid_minx)/(3*n), (grid_maxy-grid_miny)/(3*n));
        for (int i = 0; i < n; i++) {
            record_circle(pos[i], node_radius, {0.8f, 0.9f, 1.0f, 1.0f}, BLACK);
            record_text(to_string(i), pos[i], BLACK);
        }
        commit_step();

        for (int iter = 0; iter < 5000; iter++) {
            vector<pt> disp(n);

            // Repulsão
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    if (i == j) continue;
                    ld dx = pos[i].x - pos[j].x;
                    ld dy = pos[i].y - pos[j].y;
                    ld dist = sqrt(dx*dx + dy*dy);
                                           
                    if (dist < 1e-3f) { 
                        dx = random_float(-0.1f, 0.1f); 
                        dy = random_float(-0.1f, 0.1f); 
                        if (dx == 0 && dy == 0) dx = 0.1f;
                        dist = sqrt(dx*dx + dy*dy); 
                    }

                    ld rep = 2 * (k * k) / dist;
                    disp[i].x += (dx / dist) * rep;
                    disp[i].y += (dy / dist) * rep;
                }
            }

            // Atração por arestas
            for (int u = 0; u < n; u++) {
                for (auto& [v,w] : adj[u]) {
                    if (u >= v) continue;
                    ld dx = pos[u].x - pos[v].x;
                    ld dy = pos[u].y - pos[v].y;
                    ld dist = sqrt(dx*dx + dy*dy);

                    if (dist < 1e-3f) { 
                        dx = random_float(-0.1f, 0.1f); 
                        dy = random_float(-0.1f, 0.1f); 
                        if (dx == 0 && dy == 0) dx = 0.1f;
                        dist = sqrt(dx*dx + dy*dy); 
                    }

                    ld attr = (dist * dist * 17) / k;
                    disp[u].x -= (dx / dist) * attr;
                    disp[u].y -= (dy / dist) * attr;
                    disp[v].x += (dx / dist) * attr;
                    disp[v].y += (dy / dist) * attr;
                }
            }

            // Gravidade central (substitui a repulsão das paredes)
            ld center_x = (grid_minx + grid_maxx) / 2.0;
            ld center_y = (grid_miny + grid_maxy) / 2.0;

            for (int i = 0; i < n; i++) {
                ld dx = center_x - pos[i].x;
                ld dy = center_y - pos[i].y;
                ld dist = sqrt(dx*dx + dy*dy);
                if (dist > 1e-3) {
                    ld attr = (dist * dist * 6  ) / k;
                    // Puxa suavemente para o centro da tela
                    disp[i].x += (dx / dist) * attr;
                    disp[i].y += (dy / dist) * attr;
                }
            }

            // Repulsão entre nós e o meio das arestas, tentando evitar colinearidade
            for (int i = 0; i < n; i++) {
                for (int u = 0; u < n; u++) {
                    for (auto& [v, w] : adj[u]) {
                        if (u >= v || i == u || i == v) continue;
                        
                        ld mx = (pos[u].x + pos[v].x) / 2.0f;
                        ld my = (pos[u].y + pos[v].y) / 2.0f;
                        
                        ld dx = pos[i].x - mx;
                        ld dy = pos[i].y - my;
                        ld dist = sqrt(dx*dx + dy*dy);
                        
                        if (dist > 1e-3f && dist < k) { 
                            ld rep = (k * k * 2.0) / dist;
                            disp[i].x += (dx / dist) * rep;
                            disp[i].y += (dy / dist) * rep;
                            ld fx = (dx / dist) * rep;
                            ld fy = (dy / dist) * rep;
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
                ld noise_x = (random_float(-1.0f, 1.0f)) * (t * 0.05f);
                ld noise_y = (random_float(-1.0f, 1.0f)) * (t * 0.05f);
                
                disp[i].x += noise_x;
                disp[i].y += noise_y;
            }
            
            for (int i = 0; i < n; i++) {
                ld disp_len = sqrt(disp[i].x * disp[i].x + disp[i].y * disp[i].y);
                if (disp_len < 1e-5) continue;

                ld capped_len = min(disp_len, t);
                pos[i].x += (T)((disp[i].x / disp_len) * capped_len);
                pos[i].y += (T)((disp[i].y / disp_len) * capped_len);

                pos[i].x = clamp(pos[i].x, (ld)grid_minx + 10.0f, (ld)grid_maxx - 10.0f);
                pos[i].y = clamp(pos[i].y, (ld)grid_miny + 10.0f, (ld)grid_maxy - 10.0f);
            }

            t *= 0.999; // esfria
        }

        for (int u = 0; u < n; u++) {
            for (auto [v,w] : adj[u]) {
                record_line(pos[u], pos[v]);
            }
        }

        for (int i = 0; i < n; i++) {
            record_circle(pos[i], node_radius, {0.8f, 0.9f, 1.0f, 1.0f}, BLACK);
            record_text(to_string(i), pos[i], BLACK);
        }
        commit_step();
    }
    return pos;
}

void AlgorithmRecorder::draw_arrow_head(pt from, pt to) {
    double angle = atan2(to.y-from.y, to.x - from.x);
    double len = 8.0;
    pt tip = to - pt(6*cos(angle), 6*sin(angle));
    pt left(tip.x - len*cos(angle - M_PI/10), tip.y - len * sin(angle - M_PI/10));
    pt right(tip.x -len*cos(angle + M_PI/10), tip.y - len * sin(angle + M_PI/10));
    record_polygon({tip, left, right}, BLACK, BLACK, TRANSPARENT);
}