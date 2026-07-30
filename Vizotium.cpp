#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <cstdlib>
#include <functional>

// DO NOT reorder these include

#include "Camera.h"
#include "Surface.h"
#include "DrawHandel.h"

//---------------------


#include "Shapes.h"
#include "ShaderLoader.h"


using std::cerr;
using std::cout;



GLuint compile_shader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        cerr << log << '\n';
    }

    return shader;
}

GLuint create_program(const char* vs_src, const char* fs_src) {
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);

    GLuint prog = glCreateProgram();

    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);

    if (!success) {
        char log[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        cerr << log << '\n';
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return prog;
}

Camera camera{};
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

void update_MVP_n_send(GLuint mvp_location) {
    glm::mat4 mvp = camera.update_MVP();

    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
}

void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
    camera.aspect = float(width) / float(height);
}

GLFWwindow* make_window() {
    if (!glfwInit()) std::exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_SAMPLES, 8);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    GLFWwindow* window =
        glfwCreateWindow(1280, 720, "vizotium", nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        std::exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwTerminate();
        std::exit(EXIT_FAILURE);
    }
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

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

bool process_input(GLFWwindow* win, Camera& cam) {
    bool key_press = false;

    KEY_FUNC_IF(UP, cam.scale_inc(0.01f))
        KEY_FUNC_ELSE_IF(DOWN, cam.scale_inc(-0.01f))
  
            KEY_FUNC_ELSE_IF(D, cam.inc_yaw(2.f))
            KEY_FUNC_ELSE_IF(A, cam.inc_yaw(-2.f))
            KEY_FUNC_ELSE_IF(W, cam.inc_pitch(2.f))
            KEY_FUNC_ELSE_IF(S, cam.inc_pitch(-2.f))

            KEY_FUNC_ELSE_IF(8, cam.scale_inc(0.1f))
            KEY_FUNC_ELSE_IF(2, cam.scale_inc(-0.1f))

            KEY_FUNC_ELSE_IF(END, glfwSetWindowShouldClose(win, true))
            KEY_FUNC_ELSE_IF(SPACE, Time.stop_start())
            KEY_FUNC_ELSE_IF(SPACE, Time.stop_start())
            return key_press;
}

#define CLEAR_SCREEN std::cout << "\033[2J\033[1;1H"
float gauss(float x, float y) {
    float d = x * x + y * y;
    return std::exp(-d);
}

constexpr int YSZ = 200;
constexpr int XSZ = 200;

static Surface<XSZ, YSZ> sur(0.f, 0.f, 1.0, 1.0);

// Grid is a pure data class (no GL calls in its constructor), so like
// Surface it can be constructed in static memory before main() creates
// the GL context. It only needs the surface's vertex array and EBO
// layout, both of which are already available on `sur` above.

static Grid<XSZ / 10, 2, XSZ, YSZ> grid(sur.arr, sur.gl_ebo_arr(), glm::vec4(1));


GLuint is_gridLoc;
GLuint grid_clrLoc;

constexpr SetupState surface_setup = {
            false,
            false,// waiting for grid to shrink width 

            false,
            true,

            false,
            true,

            0

};

constexpr SetupState grid_setup = {
            true, // shared vbo_data
            false,

            false,
            true,

            false,
            true,
            0

};


int main() {
    mat_debug = false;
    ShaderReader<2000, 2> shader_reader("shaders.h");

    bool is_grid = false;
    //SHADER LOADING

    char* vertex_shader = shader_reader["surface"]["vertex"];
    char* fragment_shader = shader_reader["surface"]["fragment"];
    char* grid_vs = shader_reader["grid"]["vertex"];

    GLFWwindow* window = make_window();

    GLuint program = create_program(vertex_shader, fragment_shader);

    // UNIFORMS

    GLuint mvpLoc = glGetUniformLocation(program, "MVP");
    GLuint t_Loc = glGetUniformLocation(program, "t");
    GLuint f_Loc = glGetUniformLocation(program, "f");

    GLuint factr_Loc = glGetUniformLocation(program, "factr");

    GLuint xsz_Loc = glGetUniformLocation(program, "XSZ");
    GLuint ysz_Loc = glGetUniformLocation(program, "YSZ");

    is_gridLoc = glGetUniformLocation(program, "is_grid");
    grid_clrLoc = glGetUniformLocation(program, "grid_clr");

    glUseProgram(program);


    float freq = 2 * PI;  // freqency

    update_MVP_n_send(mvpLoc);
    //UPLOADING UNIFROMS
    glUniform1f(f_Loc, freq);
    glUniform3f(grid_clrLoc, 1,1,1);

    auto pre_surface = []() {
        glUniform1i(is_gridLoc,false);
        };

    auto post_surface = []() {};
    
    auto pre_grid = []() {
        glUniform1i(is_gridLoc,true);
        };

    auto post_grid = []() {};

    static GLDrawHandel        
        <surface_setup,pre_surface,post_surface>
        gl_surface{ sur.mesh_data() };
    static GLDrawHandel
        <surface_setup,pre_grid,post_grid> 
        gl_grid{ grid.mesh_data() };
    
    gl_grid.VBO = gl_surface.VBO;

    gl_grid.setup();
    gl_surface.setup();

    gl_surface.commit_vbo();

    glUseProgram(program);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);

    GLint samples;

    glGetIntegerv(GL_SAMPLES, &samples);

    std::cout << "MSAA samples = " << samples << '\n';
    
    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        bool inp = process_input(window, camera);

        glUniform1f(t_Loc, Time.time);
        Time.update();

        if (inp) {
            if (mat_debug) CLEAR_SCREEN;
            update_MVP_n_send(mvpLoc);
            cout << "[Yaw:] " << camera.yaw << " [Pitch:] " << camera.pitch;
            inp = false;
        }

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0, -1.0);

        gl_surface.draw();
        gl_grid.draw();

        glfwSwapBuffers(window);
    }
    
    glDeleteProgram(program);
    glfwTerminate();

    return 0;

}


