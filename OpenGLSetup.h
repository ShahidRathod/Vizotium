#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Camera.h"

#define RED "\x1b[31m"
#define RESET "\x1b[0m"

inline GLuint compile_shader(GLenum type, const char* src,GLint &success) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr <<RED<< log << '\n'<<RESET;
    }

    return shader;
}



inline void linkprogram(GLuint prog) {
    glLinkProgram(prog);

    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);

    if (!success) {
        char log[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        std::cerr << log << '\n';
    }
}

#include "glbuffers.h"
#include "ShaderLoader.h"

inline float winwidth = 1280;
inline float winheight = 720;

inline Camera camera;

extern glm::vec2 mapscale;
extern const int  grid_sz;
extern bool win_resized ;

extern bool inp;

inline void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
    winwidth = float(width);
    winheight = float(height);

    camera.mouse_x_centre = winwidth / 2;
    camera.mouse_y_centre = winheight / 2;

    camera.aspect = winwidth / winheight;

    mapscale = glm::vec2{ height,width };
    mapscale = glm::normalize(mapscale);
    win_resized = true;
}

inline void mouse_callback(GLFWwindow*, double x, double y) {
    camera.calibrate_eye(x, y);
    inp = true;
}


struct TimeObj {
    float time = 0.0;
    float inc = 0.01;
    bool status = true;
    float time_stmp = 0;
    void stop_start() {
        if (time_stmp >= inc * 10) {
            status = !status;
            time_stmp = 0;
        }

    }
    void update() {
        if (status) time += inc;
        time_stmp += inc;
    }

};
inline TimeObj Time{};

inline GLFWwindow* make_window() {

    if (!glfwInit()) std::exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_SAMPLES, 8);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window =
        glfwCreateWindow(winwidth, winheight, "vizotium", nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        std::exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwTerminate();
        std::exit(EXIT_FAILURE);
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);


    glViewport(0, 0, 1280, 720);

    return window;
}


#define KEY_FUNC_HLPR(key, func)                      \
    (glfwGetKey(win, GLFW_KEY_##key) == GLFW_PRESS) { \
        inp = true;                                 \
        func;                                         \
        cout << #key;                                 \
    }

#define KEY_FUNC_IF(key, func) if KEY_FUNC_HLPR (key, func)
#define KEY_FUNC_ELSE_IF(key, func) else KEY_FUNC_IF(key, func)


#define CLEAR_SCREEN std::cout << "\033[2J\033[1;1H"

#define glerror(str)  std::cout <<  str << glGetError() << "\n";




struct CellPerspective {

    
    float quad[4];
    glm::vec3 position;
    glm::vec3 origin;
    glm::vec3 relative;

    int xi, yi;

    int to_grid(float x, int sz) { return (int)((x + 0.5f) * sz); }
    float cell_pos(int i, int sz) { return (float)i / sz - 0.5f; }

    void get_edges(glm::vec3 & x, glm::vec3 & z) {
        z = glm::vec3(0.0f, quad[2] - quad[0], 1.0f);
        x = glm::vec3(1.0f, quad[1] - quad[0], 0.0f);

    }

    glm::vec3 normal() {

        glm::vec3 edge_x, edge_z;
        get_edges(edge_x,edge_z);
        return glm::normalize(glm::cross(edge_z, edge_x));

    }

    CellPerspective(glm::vec3 pos, float* cell_heights) {
        position = pos;

        xi = to_grid(pos.x, grid_sz);
        yi = to_grid(pos.z, grid_sz);

        for (int i = 0; i < 4; ++i)
            quad[i] = cell_heights[xi + (i % 2) + (yi + i / 2) * grid_sz];

        origin = glm::vec3(cell_pos(xi, grid_sz), quad[0], cell_pos(yi, grid_sz));

        relative = position - origin;


    }

   
};
