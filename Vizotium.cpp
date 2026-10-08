#include "pch.h"
#include <iostream>
#include <cstdlib>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "OpenGLSetup.h"


GLuint surface_setup();
void grid_setup();
void update_mapscale();
void draw_field(bool);
void update_random_field();
void draw_grid(bool);

extern GLuint mvpLoc;
extern GLuint grid_mvpLoc;

using std::cout, std::cerr;


void update_MVP_n_send(GLuint mvp_location) {
    glm::mat4 mvp = camera.free_look();
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
}



bool process_input(GLFWwindow* win, Camera& cam) {
    bool key_press = false;
    KEY_FUNC_IF(UP, cam.scale_inc(0.01f))
        KEY_FUNC_ELSE_IF(DOWN, cam.scale_inc(-0.01f))

        /*KEY_FUNC_ELSE_IF(D, cam.inc_yaw(2.f))
        KEY_FUNC_ELSE_IF(A, cam.inc_yaw(-2.f))
        KEY_FUNC_ELSE_IF(W, cam.inc_pitch(2.f))
        KEY_FUNC_ELSE_IF(S, cam.inc_pitch(-2.f))
        */

        KEY_FUNC_ELSE_IF(D, cam.inc_free_x(.01))
        KEY_FUNC_ELSE_IF(A, cam.inc_free_x(-.01))
        KEY_FUNC_ELSE_IF(W, cam.inc_free_z(.01))
        KEY_FUNC_ELSE_IF(S, cam.inc_free_z(-.01))

        KEY_FUNC_ELSE_IF(SPACE, cam.inc_free_y(.01))
        KEY_FUNC_ELSE_IF(LEFT_ALT, cam.inc_free_y(-.01))

        KEY_FUNC_ELSE_IF(8, cam.scale_inc(0.05f))
        KEY_FUNC_ELSE_IF(2, cam.scale_inc(-0.05f))

        KEY_FUNC_ELSE_IF(END, glfwSetWindowShouldClose(win, true))
        KEY_FUNC_ELSE_IF(SPACE, Time.stop_start())
        KEY_FUNC_ELSE_IF(ENTER, update_random_field())

        return key_press;
}

bool inp;

int main() {

    mat_debug = false;

    GLFWwindow* window = make_window();

    GLuint surface_program = surface_setup();
    grid_setup();
    glerror("error before grid setup ");

    

    framebuffer_size_callback(window, winwidth, winheight);

    inp = true;
    while (!glfwWindowShouldClose(window))
    {

        Time.update();

        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        draw_field(inp);
        draw_grid(inp);

        glfwSwapBuffers(window);
        glfwPollEvents();
        inp = process_input(window, camera);


    }

    glDeleteProgram(surface_program);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}