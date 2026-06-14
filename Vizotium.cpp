#include <cstdlib>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include "Surface.h"

using std::cerr;
using std::cout;

GLuint compile_shader(GLenum type, const char* src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        cerr << log << '\n';
    }

    return shader;
}

GLuint create_program(const char* vs_src, const char* fs_src)
{
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);

    GLuint prog = glCreateProgram();

    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);

    if (!success)
    {
        char log[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        cerr << log << '\n';
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return prog;
}

void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

GLFWwindow* make_window()
{
    if (!glfwInit())
        std::exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window =
        glfwCreateWindow(1280, 720, "vizotium", nullptr, nullptr);

    if (!window)
    {
        glfwTerminate();
        std::exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(
        reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        glfwTerminate();
        std::exit(EXIT_FAILURE);
    }

    glfwSetFramebufferSizeCallback(
        window,
        framebuffer_size_callback
    );

    glViewport(0, 0, 1280, 720);

    return window;
}

const char* vertex_shader = R"(
#version 330 core

layout(location = 0) in vec3 coords;
uniform mat4 MVP;
out vec3 fragPos;

void main()
{
    vec4 Pos;
    Pos = MVP *vec4(coords,1.0);
    fragPos = vec3(Pos.x,Pos.y,Pos.z);

    gl_Position = Pos;
}

)";

const char* fragment_shader = R"(
#version 330 core

in vec3 fragPos;

out vec4 FragColor;

void main()
{
    FragColor = vec4(16 + (235-16-50)* (1-fragPos.z),
                    30,
                    16 + (235-16-50)* fragPos.z,
                    0.7
                    );
}
)";


bool process_input(GLFWwindow* win,Camera& cam) {
   //camera translate : (up down left right) 
   // surface rotate : W S D A
   
    bool key_press = false;

    if (glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS) {
        key_press = true;
        cam.shift_x_by(0.001);
        cout << "up\n";
    }
    else if (glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS) {
        key_press = true;
        cam.shift_x_by(-0.001);
        cout << "down\n";
    }
    else if (glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS) {
        key_press = true;
        cam.shift_y_by(0.001);
        cout << "left\n";
    }
    else if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        key_press = true;
        cam.shift_y_by(-0.001);
        cout << "right\n";
    }
    else if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) {
        key_press = true;
        cam.rotate_x(0.5);
        cout << "W\n";
    }

    else if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) {
        key_press = true;
        cam.rotate_x(-0.5);
        cout << "S\n";
    }
    else if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) {
        key_press = true;
        cam.rotate_y(0.5);
        cout << "A\n";
    }
    else if  (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) {
        key_press = true;
        cam.rotate_y(-0.5);
        cout << "D\n";
    }
    
    return key_press;
}

float gauss(float x, float y) {
    double d = std::sqrt(x*x + y*y);
    return std::exp(-d);
}

int main()
{
    GLFWwindow* window = make_window();
    constexpr int YSZ = 100;
    constexpr int XSZ = 100;
    
    static Surface<100,100> sur(0.f,0.f,0.5f,0.5f);



    for (int i = 0; i < YSZ; i++) {
        for (int j = 0; j < XSZ; j++) {
            int indx = j + i * XSZ;
            Vertex& coord_xy = sur.arr[indx];
            coord_xy.Z = gauss(coord_xy.X,coord_xy.Y);
        }
    }

    GLuint VAO;
    GLuint VBO;
    GLuint EBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        sur.gl_vbo_sz(),
        sur.gl_arr(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sur.gl_ebo_sz(),
        sur.gl_ebo_arr(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),
        nullptr
    );

    glEnableVertexAttribArray(0);

    GLuint program = create_program(vertex_shader, fragment_shader);
    GLuint camLoc = glGetUniformLocation(program, "MVP");

    Camera camera {};
    
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        bool inp = process_input(window,camera);

        glUseProgram(program);

        glBindVertexArray(VAO);

        glDrawElements(
            GL_TRIANGLES,
            sur.gl_ebo_sz(),
            GL_UNSIGNED_INT,
            (void*)(0)
        );


        if (inp) {
/*
            glProgramUniformMatrix4fv(
                program,
                camLoc,
                1,
                GL_FALSE,
                glm::value_ptr(camera.MVP_upload())
            );*/
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(program);

    glfwTerminate();

    return 0;
}
