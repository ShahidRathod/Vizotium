#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <cstdlib>
#include <iostream>
#include "Surface.h"

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
        if (time_stmp >= inc * 50) {
            status = !status;
            time_stmp = 0;
        }
     
    }
    void update () {
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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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
        // KEY_FUNC_ELSE_IF(LEFT, cam.shift_x_by(-0.1f))
        // KEY_FUNC_ELSE_IF(RIGHT, cam.shift_x_by(0.1f))

        KEY_FUNC_ELSE_IF(D, cam.inc_yaw(2.f))
        KEY_FUNC_ELSE_IF(A, cam.inc_yaw(-2.f))
        KEY_FUNC_ELSE_IF(W, cam.inc_pitch(2.f))
        KEY_FUNC_ELSE_IF(S, cam.inc_pitch(-2.f))

        KEY_FUNC_ELSE_IF(8, cam.scale_inc(0.1f))
        KEY_FUNC_ELSE_IF(2, cam.scale_inc(-0.1f))

        KEY_FUNC_ELSE_IF(END, glfwSetWindowShouldClose(win, true))
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

/*
void update_arr() {
    static float t = 0;
    for (int i = 0; i < YSZ; i++) {
        for (int j = 0; j < XSZ; j++) {
            int indx = j + i * XSZ;
            Vertex& coord_xy = sur.arr[indx];
            coord_xy.Y = gauss(std::cos(5 * (t+coord_xy.X)), std::sin(5 *
(t+coord_xy.Z)));
            // cout << "[" << coord_xy.Z << "]";
        }
        //cout << "\n\n";
    }
    t += 0.001;
}

*/

enum class BufferIds : int {

    EBO,
    VBO,
    EBO_major_grid,
    EBO_minor_grid
};

enum class VertexIds : int { VAO, major_grid, minor_grid };

#define buffer(name) buffer_ids[(int)BufferIds::name]
#define vertex(name) vertex_ids[(int)VertexIds::name]

GLuint is_gridLoc;
GLuint grid_clrLoc;

int main()

{
    /*const char* grid_shader = R"(

#version 330 core
in vec3 fragPos;
out vec4 FragColor;

void main()
{
   FragColor = vec4(fragPos.y,.2,1-fragPos.y,1);

}
)";
*/

    const char* vertex_shader = R"(
#version 330 core

layout(location = 0) in vec3 coords;
layout(location = 1) in vec3 coords_side;

uniform mat4 MVP;
uniform float t;
uniform float f;
uniform float factr;
uniform bool is_grid;

out vec3 fragPos;
out vec3 sidePos;

float gauss(float x)
{
    return exp(-x);
}

float spiral_y (vec3 p) {
 
    float r = length(p.xz);
    float theta = atan(p.z, p.x);

    float centerFade = 1.0 - exp(-8.0 * r * r);

    float phase = 4.0 * theta * centerFade - 5.0 * r + 1.5 * t;
    return 0.65 * exp(-0.6 * r) * sin(phase);   
}



void main()
{
 
vec3 pos = coords;
vec3 side_pos = coords_side;

side_pos.y = spiral_y(side_pos);

   
pos.y = spiral_y(pos);

if (is_grid) {
        pos.y += 0.001;
}
vec4 Pos = MVP * vec4(pos, 1.0);

    fragPos = pos;
    sidePos = coords_side;
    

    
   
    gl_Position = Pos;
}
)";


    const char* grid_vertex_shader = R"(
#version 330 core
layout(location = 0) in vec3 coords_draw;
layout(location = 1) in vec3 coords_side;


)";


    const char* fragment_shader = R"(
#version 330 core

in vec3 fragPos;
in vec3 sidePos;
out vec4 FragColor;

uniform bool is_grid;
uniform vec4 grid_clr;

void main()
{
   if (is_grid){
     FragColor = grid_clr;
    
   }else {
     float h = fragPos.y;
     FragColor = vec4(h+h*h + h*h*h,.2, 1-.5*exp(h),1);
 
    }
}
)";

    mat_debug = false;
    GLFWwindow* window = make_window();

    GLuint program = create_program(vertex_shader, fragment_shader);

    // UNIFORMS 

    GLuint mvpLoc = glGetUniformLocation(program, "MVP");
    GLuint t_Loc = glGetUniformLocation(program, "t");
    GLuint f_Loc = glGetUniformLocation(program, "f");

    GLuint factr_Loc = glGetUniformLocation(program, "factr");
    
    is_gridLoc = glGetUniformLocation(program, "is_grid");
    grid_clrLoc = glGetUniformLocation(program, "grid_clr");

    glUseProgram(program);

  
    float freq = 2 * PI;  // freqency

    update_MVP_n_send(mvpLoc);

    glUniform1f(f_Loc, freq);

    static GLSurfaceHandel<XSZ, YSZ> gl_surface{ &sur };
    static SurfaceGrid<XSZ / 10, 2, XSZ, YSZ> gl_grid{ gl_surface };

    glUseProgram(program);

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
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


        /* glEnable(GL_POLYGON_OFFSET_FILL);
         glPolygonOffset(1.0,-1.0);
         */


        gl_surface.draw();
        gl_grid.draw();

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    glDeleteProgram(program);

    glfwTerminate();

    return 0;
}