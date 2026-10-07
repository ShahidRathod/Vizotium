#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Camera.h"


#define RED "\x1b[31m"
#define RESET "\x1b[0m"

GLuint compile_shader(GLenum type, const char* src,GLint &success) {
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



void linkprogram(GLuint prog) {
    glLinkProgram(prog);

    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);

    if (!success) {
        char log[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        std::cerr << log << '\n';
    }
}

float winwidth = 1280;
float winheight = 720;

Camera camera;

extern glm::vec2 mapscale;
extern const int  grid_sz;
extern bool win_resized ;

void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
    winwidth = float(width);
    winheight = float(height);


    camera.aspect = winwidth / winheight;

    mapscale = glm::vec2{ height,width };
    mapscale = glm::normalize(mapscale);
    win_resized = true;
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
TimeObj Time{};

GLFWwindow* make_window() {

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

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);


    glViewport(0, 0, 1280, 720);

    return window;
}


#define KEY_FUNC_HLPR(key, func)                      \
    (glfwGetKey(win, GLFW_KEY_##key) == GLFW_PRESS) { \
        key_press = true;                             \
        func;                                         \
        cout << #key;                                 \
    }

#define KEY_FUNC_ELSE_IF(key, func) else if KEY_FUNC_HLPR (key, func)
#define KEY_FUNC_IF(key, func) if KEY_FUNC_HLPR (key, func)

#define CLEAR_SCREEN std::cout << "\033[2J\033[1;1H"

#define glerror(str)  std::cout <<  str << glGetError() << "\n";
