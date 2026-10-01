#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cstdlib>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "OpenGLSetup.h"
#include "glbuffers.h"
#include "ShaderLoader.h"
#include "Gaussian.h"

using std::cout, std::cerr;

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



struct Vertex {
    float x, y, z;
};

struct Tri {
    Vertex p1, p2, p3;
};

struct StripQuad {
    Vertex p1, p2, p3, p4;
};



VBO<1, StripQuad> vbo;
StripQuad strip_quad = {
    {  1.0f,  0.0f, 0.0f },
    {  1.0f,  1.0f, 0.0f },
    {  0.0f,   0.f, 0.0f },
    {  0.f,   1.f, 0.0f },

};

float* gridmap;

VBO<3, Vertex> offset;

Vertex offset_data[] = {
    { -.95, -.9, 0.0f },
    {  -.95, .05, 0.0f },
    {0,0,0}
};

Vertex* offset_ptr;

glm::vec3 mapscale = { 0.1,0.1 ,0.1 };
bool win_resized = true;

constexpr int grid_pow = 7;
constexpr int grid_sz = 1 << grid_pow;
constexpr int grid_sz_sq = grid_sz * grid_sz;

SurfaceEBO<grid_sz> sur_ebo;
// ----- random fields -----------
static ComplexNoise<grid_pow> noise;
static PlainVBO<grid_sz_sq * 2> rndm_field; // first half contains the rndm_field 
//second half contains the noise 

float* rndm_field_mem;
void make_new_field() {
    noise.init_noise();
    noise.apply_spectral_bias();
    noise.fft.inverse_fft();
}

GLsync draw_done;

void update_random_field() {
    make_new_field();
    make_new_field();
    noise.output_grayscale(rndm_field_mem);
    noise.grayscale_noise(rndm_field_mem + grid_sz_sq);
    GLenum type = glClientWaitSync(draw_done,0,(int)1e4);
    
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


void map_func() {
    gridmap[2] += 0.01; ;
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
        KEY_FUNC_ELSE_IF(3, map_func())

        KEY_FUNC_ELSE_IF(END, glfwSetWindowShouldClose(win, true))
        KEY_FUNC_ELSE_IF(SPACE, Time.stop_start())
        KEY_FUNC_ELSE_IF(ENTER, update_random_field())

        return key_press;
}

#define CLEAR_SCREEN std::cout << "\033[2J\033[1;1H"

int main()
{

    mat_debug = false;
    memcpy(vbo.data, &strip_quad, sizeof(strip_quad));
    memcpy(offset.data, &offset_data, sizeof(offset_data));

    GLFWwindow* window = make_window();

    GLuint program = glCreateProgram();
    GLuint surface_program = glCreateProgram();
    ShaderReader<4000, 64> reader("shaders.h");


    reader.compile_shader_for("heightmap", GL_VERTEX_SHADER, program);
    reader.compile_shader_for("heightmap", GL_FRAGMENT_SHADER, program);

    reader.compile_shader_for("surface", GL_VERTEX_SHADER, surface_program);
    reader.compile_shader_for("surface", GL_FRAGMENT_SHADER, surface_program);

    linkprogram(program);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);

    // UNIFORMS
    // UNIFORMS
    GLuint mvpLoc = glGetUniformLocation(program, "MVP");
    GLuint map_scaleLoc = glGetUniformLocation(program, "scale");
    GLuint grid_szLoc = glGetUniformLocation(program, "grid_sz");


    
    linkprogram(surface_program);
    GLuint mvpLoc2 = glGetUniformLocation(surface_program, "MVP");
    GLuint grid_szLoc2 = glGetUniformLocation(surface_program, "grid_sz");


    GLuint vaos[4];
    GLuint& vao = vaos[0];
    GLuint& mapscalevao = vaos[1];
    GLuint& mapoffsetvao = vaos[2];
    GLuint& sur_vao = vaos[3];

    glGenVertexArrays(4, vaos);


    glBindVertexArray(vao);

    //this only generated buffer ids 

    rndm_field.init_buffer();
    vbo.init_buffer();
    offset.init_buffer();
    sur_ebo.init_buffer();

    
    rndm_field.bind();
    make_new_field();
    noise.output_grayscale(rndm_field.data);
    noise.grayscale_noise(rndm_field.data + grid_sz_sq);

    GLuint surface_flags =
        GL_MAP_WRITE_BIT |
        GL_MAP_PERSISTENT_BIT|
        GL_MAP_COHERENT_BIT;

    rndm_field.upload_persistant(surface_flags);
    rndm_field_mem = (float*)rndm_field.map_full(surface_flags);

    glVertexAttribPointer(
        2,
        1,
        GL_FLOAT,
        GL_TRUE,
        sizeof(float),
        nullptr
    );


    sur_ebo.bind();
    sur_ebo.upload(GL_STATIC_DRAW);


    glEnableVertexAttribArray(2);


    offset.bind();
    offset.upload_persistant(GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
    offset_ptr = (Vertex*)offset.map_full();

    constexpr int offsetInstanceDivisor = grid_sz_sq;
    constexpr int map_instance_count = 2 * grid_sz_sq;

    if (!offset.is_binded()) std::cout << "not binded";

    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        nullptr
    );

    glVertexAttribDivisor(1, offsetInstanceDivisor);
    glEnableVertexAttribArray(1);
   
    vbo.bind();

    vbo.upload_persistant(GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);

    gridmap = (float*)vbo.map_full();

    glVertexAttribDivisor(2, 0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        nullptr
    );
    glEnableVertexAttribArray(0);
    

   

    //sur_ebo.print();
    cout << "\n\n---" << glGetError();
    framebuffer_size_callback(window, winwidth, winheight);

    while (!glfwWindowShouldClose(window))
    {
        
        bool inp = process_input(window, camera);
        Time.update();

        glUseProgram(program);
        glUniform1i(grid_szLoc, grid_sz);

        if (inp) {
            if (mat_debug) CLEAR_SCREEN;
            update_MVP_n_send(mvpLoc);
        }


        if (win_resized) {
            glUniform3fv(map_scaleLoc, 1, &mapscale[0]);
            win_resized = false;
        }

      
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
       
        glVertexAttribDivisor(2, 1);
        glDrawArraysInstanced(
            GL_TRIANGLE_STRIP,
            0,
            4,
            map_instance_count);
     


        glUseProgram(surface_program);

        if (inp) {
            update_MVP_n_send(mvpLoc2);
        }

        glUniform1i(grid_szLoc2, grid_sz);

        glVertexAttribDivisor(2, 0);
        glDrawElements(
            GL_TRIANGLE_STRIP,
            sur_ebo.draw_count(),
            GL_UNSIGNED_SHORT,
            nullptr);

        draw_done = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);

        glfwSwapBuffers(window);
        if (inp) inp = false;

        glfwPollEvents();
        
    }

    glDeleteProgram(program);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}