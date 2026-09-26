#define _CRT_SECURE_NO_WARNINGS

#include <math.h>
#include <cstdlib>


// DO NOT reorder these include
#include "OpenGLSetup.h"
#include "Camera.h"
#include "Surface.h"
#include "DrawHandel.h"

//---------------------

#include "Shapes.h"
#include "ShaderLoader.h"
#include "Gaussian.h"
#include "glbuffers.h"

using std::cerr;
using std::cout;


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



#define KEY_FUNC_HLPR(key, func)                      \
    (glfwGetKey(win, GLFW_KEY_##key) == GLFW_PRESS) { \
        key_press = true;                             \
        func;                                         \
        cout << #key;                                 \
    }

#define KEY_FUNC_ELSE_IF(key, func) else if KEY_FUNC_HLPR (key, func)
#define KEY_FUNC_IF(key, func) if KEY_FUNC_HLPR (key, func)

constexpr int grid_pow = 8;
constexpr int grid_sz = 1 << 8;
constexpr int grid_sz_sq = grid_sz * grid_sz;


// ----- random fields -----------
static ComplexNoise<grid_pow> noise;
static VBO<grid_sz_sq> rndm_field;

void make_new_field() {
    noise.init_noise();
    noise.fft.inverse_fft();
    noise.output_grayscale(rndm_field.data);
}

void make_random_field() { 
    make_new_field();
    rndm_field.bind();
    void* dst = rndm_field.map_full(GL_MAP_FLUSH_EXPLICIT_BIT);


}
//---------------------------

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
        KEY_FUNC_ELSE_IF(ENTER, make_random_field())

        return key_press;
}

#define CLEAR_SCREEN std::cout << "\033[2J\033[1;1H"




int main() {

    mat_debug = false;
    GLFWwindow* window = make_window();

    GLuint program = glCreateProgram();
    ShaderReader<4000, 64> reader("shaders.h",program);
  

    reader.compile_shader_for("surface", GL_VERTEX_SHADER);
    reader.compile_shader_for("surface",GL_FRAGMENT_SHADER);

    // UNIFORMS
    GLuint mvpLoc = glGetUniformLocation(program, "MVP");


    glUseProgram(program);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);

    rndm_field.init_gl_buffer();
    make_new_field();

    //DrawHandel<grid_sz,0> grid_draw{&rndm_field};

    rndm_field.upload_persistant(GL_DYNAMIC_DRAW);

    while (!glfwWindowShouldClose(window)) {

        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        bool inp = process_input(window, camera);
        Time.update();
        if (inp) {
            if (mat_debug) CLEAR_SCREEN;
            update_MVP_n_send(mvpLoc);

            //cout << "[Yaw:] " << camera.yaw << " [Pitch:] " << camera.pitch;
            inp = false;
        }


        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0, -1.0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(program);

    glfwTerminate();    
    return 0;

}