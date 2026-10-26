#include "pch.h"
#include <iostream>
#include <cstdlib>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "OpenGLSetup.h"


GLuint surface_setup();
void grid_setup();
void optimizer_setup();
void update_mapscale();
void draw_field(bool);
void update_rndm_field();
void draw_grid(bool);
void draw_all(bool);
void update_all();
void new_optimizer();
bool camera_close_surface(glm::vec3& );

extern GLuint mvpLoc;
extern GLuint grid_mvpLoc;

using std::cout, std::cerr;


ShaderReader<shader_file_sz, max_tags>* reader = nullptr;

void update_MVP_n_send(GLuint mvp_location) {
    glm::mat4 mvp = camera.free_look();
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
}


bool inp;


constexpr float move_v = 0.0002;
constexpr float v_limit = 0.02;

glm::vec3 acceleration = { 0.f, 0.f, 0.f };

void process_input(GLFWwindow* win, Camera& cam) {

    acceleration = glm::vec3(0.f);

        KEY_FUNC_IF(UP, cam.scale_inc(0.01f))
            KEY_FUNC_IF(DOWN, cam.scale_inc(-0.01f))

            /*KEY_FUNC_ELSE_IF(D, cam.inc_yaw(2.f))
            KEY_FUNC_ELSE_IF(A, cam.inc_yaw(-2.f))
            KEY_FUNC_ELSE_IF(W, cam.inc_pitch(2.f))
            KEY_FUNC_ELSE_IF(S, cam.inc_pitch(-2.f))
            */

            KEY_FUNC_IF(A, acceleration.z = +move_v)
            KEY_FUNC_ELSE_IF(D, acceleration.z = -move_v)
            else acceleration.z = -glm::sign(cam.free_velct.z) * move_v;

            KEY_FUNC_IF(W, acceleration.x = -move_v)
            KEY_FUNC_ELSE_IF(S, acceleration.x = +move_v)
            else acceleration.x = -glm::sign(cam.free_velct.x) * move_v;

            KEY_FUNC_IF(SPACE, acceleration.y = +move_v)
            KEY_FUNC_ELSE_IF(LEFT_ALT, acceleration.y = -move_v)
            else acceleration.y = -glm::sign(cam.free_velct.y) * move_v;

            KEY_FUNC_IF(8, cam.scale_inc(0.05f))
            KEY_FUNC_IF(2, cam.scale_inc(-0.05f))


        KEY_FUNC_IF(END, glfwSetWindowShouldClose(win, true))
        KEY_FUNC_IF(SPACE, Time.stop_start());
        KEY_FUNC_IF(ENTER, update_rndm_field())
        KEY_FUNC_IF(RIGHT_SHIFT, new_optimizer())
        KEY_FUNC_IF(UP, update_all())

        inp = cam.update_camera(acceleration, v_limit) || inp;
        camera_close_surface(cam.free_pos);
}

int main() {


    ShaderReader<shader_file_sz, max_tags> shader_reader("shaders.h");
    reader = &shader_reader;
    mat_debug = false;

    GLFWwindow* window = make_window();
    framebuffer_size_callback(window, winwidth, winheight);
    GLuint surface_program = surface_setup();
    grid_setup();
    
    optimizer_setup();
    new_optimizer();

    inp = true;
    while (!glfwWindowShouldClose(window))
    {
        //CLEAR_SCREEN;
        Time.update();

        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        //update_all();
        draw_all(inp);
        draw_field(inp);
        draw_grid(inp);
 

        if (inp) {
          /*  camera.debug_eye_angles();
            cout << "\nfree position: ";
            print_vec(camera.free_position);
            cout << "\nfree eye: ";
            print_vec(camera.free_eye);

            cout << "\nfree eye_p: ";
            print_vec(camera.free_eye_p);
            */
        }
        else {
            camera.no_input();
        }

        inp = false;

        glfwSwapBuffers(window);
        glfwPollEvents();
        process_input(window, camera);


    }

    glDeleteProgram(surface_program);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}