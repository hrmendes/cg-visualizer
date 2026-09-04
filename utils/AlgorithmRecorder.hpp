#ifndef ALGORITHM_RECORDER_HPP
#define ALGORITHM_RECORDER_HPP

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include "../geom/geom.hpp"
#include "vulkan_renderer/Visualizer.hpp"
#include "colors.hpp"

struct ptf {
    float x, y;
    ptf(float x = 0, float y = 0) : x(x), y(y) {}
};

class AlgorithmRecorder {
public:
    enum CommandType { DRAW_POINT, DRAW_LINE, DRAW_POLYGON, LOG };

    AlgorithmRecorder(
        float grid_minx, float grid_maxx, 
        float grid_miny, float grid_maxy, 
        int screenWidth = 800, int screenHeight = 800
    ) : grid_minx(grid_minx), grid_maxx(grid_maxx), grid_miny(grid_miny), grid_maxy(grid_maxy) {
        proj = glm::ortho(grid_minx, grid_maxx, grid_maxy, grid_miny,-1.0f, 1.0f);
        vis.init(screenWidth,screenHeight);
    }

    struct Command {
        CommandType type;
        vector<pt> points;
        glm::vec4 color;
        string log = "";
    };

    struct Frame {
        vector<Command> commands;
        vector<string> logs;
    };

    vector<Frame> timeline;
    size_t current_frame = 0;

    void record_point(pt p, glm::vec4 color) {
        current_frame_commands.push_back({DRAW_POINT, {p}, color});
    }

    void record_line(pt a, pt b, glm::vec4 color) {
        current_frame_commands.push_back({DRAW_LINE, {a, b}, color});
    }

    void record_polygon(
        const vector<pt>& poly, 
        glm::vec4 fill_color = LIGHT_GRAY, 
        glm::vec4 line_color = BLACK, 
        glm::vec4 point_color = BLACK
    ) {
        for (int i = 0; i < poly.size(); i++){
            current_frame_commands.push_back({DRAW_POINT, {poly[i]}, point_color});
            current_frame_commands.push_back({DRAW_LINE, {poly[i], poly[(i+1)%poly.size()]}, line_color});
        }
        current_frame_commands.push_back({DRAW_POLYGON, poly, fill_color});
    }

    void record_log(string log){
        current_frame_commands.push_back({LOG, {}, {}, log});
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

    void run(){
        bool space_prev = false;
        bool enter_prev = false;
        bool autoplay = false;
        double last_time = glfwGetTime();

        while (vis.is_running()) {
            vis.clear_buffers();
            draw_grid(20.0f);

            bool space = vis.is_key_pressed(GLFW_KEY_SPACE);
            bool enter = vis.is_key_pressed(GLFW_KEY_ENTER);

            if (!autoplay && space && !space_prev) {
                if (!timeline.empty()) {
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
                const auto& frame = timeline[current_frame];
                for (const auto& cmd : frame.commands) {
                    if (cmd.type == AlgorithmRecorder::DRAW_POINT) {
                        vis.draw_point(cmd.points[0], cmd.color);
                    } else if (cmd.type == AlgorithmRecorder::DRAW_LINE) {
                        vis.draw_line(cmd.points[0], cmd.points[1], cmd.color);
                    } else if (cmd.type == AlgorithmRecorder::DRAW_POLYGON) {
                        vis.draw_polygon(cmd.points, cmd.color);
                    } else if (cmd.type == AlgorithmRecorder::LOG) {
                        cout << "[LOG]: " << cmd.log << endl;
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
            vis.draw_line(ptf(x, grid_minx), ptf(x, grid_maxx), grid_color);
        }
        for (float y = grid_miny; y <= grid_maxy; y += step) {
            if (y == 0.0f) continue;
            vis.draw_line(ptf(grid_miny, y), ptf(grid_maxy, y), grid_color);
        }

        float half_thick = axis_thickness / 2.0f;
        for (float offset = -half_thick; offset <= half_thick; offset += 0.1f) {
            vis.draw_line(ptf(grid_minx, offset), ptf(grid_maxx, offset), {1,0,0,1});
        }
        for (float offset = -half_thick; offset <= half_thick; offset += 0.1f) {
            vis.draw_line(ptf(offset, grid_minx), ptf(offset, grid_maxx), {0,1,0,1});
        }
    }

};

#endif