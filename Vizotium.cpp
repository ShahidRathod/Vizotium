
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cstdlib>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "OpenGLSetup.h"
#include "glbuffers.h"
#include "Gaussian.h"
#include "ShaderLoader.h"
#include "SurfaceSetup.h"

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

    GLuint grid_szLoc = glGetUniformLocation(surface_program, "grid_sz");
    GLuint mvpLoc = glGetUniformLocation(surface_program, "MVP");
    GLuint mapscaleLoc = glGetUniformLocation(surface_program, "mapscale");

    GLuint vaos[4];
    GLuint& vao = vaos[0];
    GLuint& mapscalevao = vaos[1];
    GLuint& mapoffsetvao = vaos[2];
    GLuint& sur_vao = vaos[3];

    glGenVertexArrays(4, vaos);
    glBindVertexArray(vao);

    //this only generated buffer ids 

    rndm_field.init_buffer();
    offset.init_buffer();
    sur_ebo.init_buffer();


    rndm_field.bind();

    noise.make_new_field();
    noise.output_grayscale(rndm_field.data);
    noise.grayscale_noise(rndm_field.data + grid_sz_sq);

    GLuint surface_flags =
        GL_MAP_WRITE_BIT |
        GL_MAP_PERSISTENT_BIT |
        GL_MAP_COHERENT_BIT;

    rndm_field.upload_persistant(surface_flags);
    rndm_field_mem = (float*)rndm_field.map_full(surface_flags);

    sur_ebo.bind();
    sur_ebo.upload(GL_STATIC_DRAW);

    glVertexAttribPointer(
        height_layout,
        1,
        GL_FLOAT,
        GL_TRUE,
        sizeof(float),
        nullptr
    );
   
   glEnableVertexAttribArray(height_layout);

   glVertexAttribPointer(
       heightclr_layout,
       1,
       GL_FLOAT,
       GL_FALSE,
       sizeof(float),
       nullptr
   );
   glEnableVertexAttribArray(heightclr_layout);

    glVertexAttribPointer(
        noise_layout,
        1,
        GL_FLOAT,
        GL_TRUE,
        sizeof(float),
        (void*)(noise_layout_stride)
    );
    glEnableVertexAttribArray(noise_layout);


    glVertexAttribPointer(
        noiseclr_layout,
        1,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float),
        (void*)noise_layout_stride
    );
    glEnableVertexAttribArray(noiseclr_layout);

    offset.bind();
    offset.upload(GL_STATIC_DRAW);
    glVertexAttribPointer(
        offset_layout,
        3,
        GL_FLOAT,
        GL_TRUE,
        3*sizeof(float),
        nullptr
    );
    glEnableVertexAttribArray(offset_layout);
    glVertexAttribDivisor(offset_layout, 1); // first the ortho progrction will be drawn 


    glUseProgram(surface_program);
    glUniform1i(grid_szLoc, grid_sz);
    glUniform2fv(mapscaleLoc, 2, &mapscale[0]);

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