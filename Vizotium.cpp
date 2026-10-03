
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cstdlib>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "OpenGLSetup.h"
#include "glbuffers.h"
#include "Gaussian.h"
#include "ShaderLoader.h"
// Surface state is defined in SurfaceSetup.cpp (single file, no header).
// Declarations below mirror its definitions; values unchanged.
constexpr int grid_pow = 7;
constexpr int grid_sz = 1 << grid_pow;
constexpr int grid_sz_sq = grid_sz * grid_sz;

extern VBO<3, glm::vec3> offset;
extern glm::vec3 offset_data[3];
extern glm::vec3* surface_offset;
extern glm::vec3* offset_ptr;
extern glm::vec2 mapscale;
extern bool win_resized;

extern SurfaceEBO<grid_sz> sur_ebo;

extern ComplexNoise<grid_pow> noise;
extern PlainVBO<grid_sz_sq * 2> rndm_field;

void setup_surface(GLuint surface_program,
                   GLuint& grid_szLoc,
                   GLuint& mvpLoc,
                   GLuint& mapscaleLoc);

using std::cout, std::cerr;




void update_MVP_n_send(GLuint mvp_location) {
    glm::mat4 mvp = camera.update_MVP();
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
}




float* rndm_field_mem;

GLsync draw_done;

void update_random_field() {

    noise.make_new_field();
    noise.output_grayscale(rndm_field_mem);
    noise.grayscale_noise(rndm_field_mem + grid_sz_sq);
    GLenum type = glClientWaitSync(draw_done, 0, (int)1e4);

    switch (type)
    {
    case GL_ALREADY_SIGNALED:
        std::cout << "already signaled";
    case GL_CONDITION_SATISFIED:
        std::cout << "condition satisfied";
    case GL_TIMEOUT_EXPIRED:
        std::cout << "time out ";
    case GL_WAIT_FAILED:
        std::cout << "error in sync";
    default:
        break;
    }
    rndm_field.flushfull();
}


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
        KEY_FUNC_ELSE_IF(ENTER, update_random_field())

        return key_press;
}


int main()
{

    mat_debug = false;
    memcpy(offset.data, &offset_data, sizeof(offset_data));

    GLFWwindow* window = make_window();

    GLuint surface_program = glCreateProgram();
    ShaderReader<4000, 64> reader("shaders.h");

    reader.compile_shader_for("surface", GL_VERTEX_SHADER, surface_program);
    reader.compile_shader_for("surface", GL_FRAGMENT_SHADER, surface_program);

  
    linkprogram(surface_program);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);

    GLuint grid_szLoc;
    GLuint mvpLoc;
    GLuint mapscaleLoc;

    setup_surface(surface_program, grid_szLoc, mvpLoc, mapscaleLoc);

    bool inp = true;


    framebuffer_size_callback(window, winwidth, winheight);
    while (!glfwWindowShouldClose(window))
    {

        Time.update();
        

        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (win_resized) {
            glUniform2fv(mapscaleLoc, 1, &mapscale[0]);
            win_resized = false;
        }

        if (inp) {
            update_MVP_n_send(mvpLoc);
            inp = false;
        }

        glDrawElementsInstanced(
            GL_TRIANGLE_STRIP,
            sur_ebo.draw_count(),
            GL_UNSIGNED_SHORT,
            nullptr,
            3);

        //glDrawArrays(GL_TRIANGLE_STRIP, 0, grid_sz_sq);

        draw_done = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        glfwSwapBuffers(window);
        glfwPollEvents();
        inp = process_input(window, camera);

    }

    glDeleteProgram(surface_program);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}