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

VBO<2 ,Vertex> mapPos;
Vertex mapPos_data[] = { 
    {  -0.3,  -0.4 , 0.f },
    {  0.3,  -0.4, 0.0f },
};

Vertex* mappos_ptr;

glm::vec3 mapscale = { 0.1,0.1 ,0.1};
bool win_resized = true;

constexpr int grid_pow = 7;
constexpr int grid_sz = 1 << grid_pow;
constexpr int grid_sz_sq = grid_sz * grid_sz;


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

void update_random_field() {
    make_new_field();
    make_new_field();
    noise.output_grayscale(rndm_field_mem);
    noise.grayscale_noise(rndm_field_mem + grid_sz_sq);
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
    memcpy(vbo.data,&strip_quad,sizeof(strip_quad));
    memcpy(mapPos.data, &mapPos_data, sizeof(mapPos_data));

    GLFWwindow* window = make_window();

    GLuint program = glCreateProgram();
    ShaderReader<4000, 64> reader("shaders.h", program);


    reader.compile_shader_for("heightmap", GL_VERTEX_SHADER);
    reader.compile_shader_for("heightmap", GL_FRAGMENT_SHADER);

    linkprogram(program);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);

    // UNIFORMS
    // UNIFORMS
    GLuint mvpLoc = glGetUniformLocation(program, "MVP");
    GLuint map_scaleLoc = glGetUniformLocation(program, "scale");
    GLuint grid_szLoc = glGetUniformLocation(program, "grid_sz");

    GLuint vaos[3];
    GLuint& vao = vaos[0];
    GLuint& mapscalevao = vaos[1];
    GLuint& mapoffsetvao = vaos[2];

    glGenVertexArrays(3,vaos);


    glBindVertexArray(vao);

    // this only generated buffer ids 
    rndm_field.init_buffer();
    vbo.init_buffer();
    mapPos.init_buffer();
    

    rndm_field.bind();
    make_new_field();
    noise.output_grayscale(rndm_field.data);
    noise.grayscale_noise(rndm_field.data+grid_sz_sq);
    rndm_field.upload_persistant(GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT);
    rndm_field_mem = (float*)rndm_field.map_full(GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT);

    glVertexAttribPointer(
        2,
        1,
        GL_FLOAT,
        GL_TRUE,
        sizeof(float),
        nullptr
    );

    glVertexAttribDivisor(2,1);
    glEnableVertexAttribArray(2);




    mapPos.bind();
    mapPos.upload_persistant(GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
    mappos_ptr = (Vertex*)mapPos.map_full();

    constexpr int offsetInstanceDivisor = grid_sz_sq;
    constexpr int map_instance_count =  2*grid_sz_sq;

    if (!mapPos.is_binded()) std::cout << "not binded";

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

    if (!vbo.is_binded()) std::cout << "not binded";
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        nullptr
    );
    glEnableVertexAttribArray(0);
    glUseProgram(program);

    
   
    glUniform1i(grid_szLoc, grid_sz);

    
    cout <<"\n\n---" << glGetError();
    framebuffer_size_callback(window,winwidth,winheight);
    while (!glfwWindowShouldClose(window))
    {

        bool inp = process_input(window, camera);
        Time.update();
        if (inp) {
            if (mat_debug) CLEAR_SCREEN;
            update_MVP_n_send(mvpLoc);

            //cout << "[Yaw:] " << camera.yaw << " [Pitch:] " << camera.pitch;
            inp = false;
        }

        if (win_resized) {
            glUniform3fv(map_scaleLoc,1,&mapscale[0]);
            win_resized = false;
        }

        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

         
        glDrawArraysInstanced(
            GL_TRIANGLE_STRIP, 
            0, 
            4,
            map_instance_count);

        //glDrawArrays(GL_TRIANGLE_STRIP,0,4);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glDeleteProgram(program);


    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}