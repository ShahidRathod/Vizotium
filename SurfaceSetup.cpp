#include "pch.h"
#include "glbuffers.h"
#include "Gaussian.h"

#define glerror(str)  std::cout <<  str << glGetError() << "\n";

GLuint compile_shader(GLenum type, const char* src,GLint& success);

#include "ShaderLoader.h"
//#include "OpenGLSetup.h"

void linkprogram(GLuint prog);

constexpr int grid_pow = 8;
constexpr int grid_sz = 1 << grid_pow;
constexpr int grid_sz_sq = grid_sz * grid_sz;

constexpr int offsetInstanceDivisor = grid_sz_sq;
constexpr int map_instance_count = 2 * grid_sz_sq;

constexpr int offset_layout = 0;
constexpr int height_layout = 1;
constexpr int heightclr_layout = 2;
constexpr int noise_layout = 3;
constexpr int noiseclr_layout = 4;
constexpr int noise_layout_stride = sizeof(float) * grid_sz_sq;

void update_MVP_n_send(GLuint loc);

VBO<3, glm::vec3> offset;

glm::vec3 offset_data[] = {
    {  -0.73, 1-.53, 0.0f },
    { -0.73, -.53, 0.0f },
    { 1, 0.0f, 0.0f },
};
glm::vec3* surface_offset = &offset_data[2];
glm::vec3 * offset_ptr;
glm::vec2 mapscale = { 1.f,1.f};
bool win_resized = true;

SurfaceEBO<grid_sz> sur_ebo;
// random fields

ComplexNoise<grid_pow> noise;
PlainVBO<grid_sz_sq * 2> rndm_field;

// first half contains the rndm_field
//second half contains the noise

float* rndm_field_mem;

GLsync draw_done;

//ShaderReader<4000, 64> reader("shaders.h");

GLuint surface_program = 0;

GLuint grid_szLoc;
GLuint mvpLoc;
GLuint mapscaleLoc;
GLuint vaos[4];



void setup_surface(GLuint program) {

    memcpy(offset.data, &offset_data, sizeof(offset_data));
    glerror(" e1");
    grid_szLoc = glGetUniformLocation(program, "grid_sz");
    mvpLoc = glGetUniformLocation(program, "MVP");
    mapscaleLoc = glGetUniformLocation(program, "mapscale");
    glerror(" e2");
    GLuint& vao = vaos[0];
    GLuint& mapscalevao = vaos[1];
    GLuint& mapoffsetvao = vaos[2];
    GLuint& sur_vao = vaos[3];
    glerror(" e3");

    glGenVertexArrays(4, vaos);
    glBindVertexArray(vao);
    glerror(" e4");

    //this only generated buffer ids

    rndm_field.init_buffer();
    offset.init_buffer();
    sur_ebo.init_buffer();

    glerror(" e5");


    rndm_field.bind();
    glerror(" e6");

    noise.make_new_field();
    glerror(" e7");

    noise.output_grayscale(rndm_field.data);
    noise.grayscale_noise(rndm_field.data + grid_sz_sq);

    GLuint surface_flags =
        GL_MAP_WRITE_BIT |
        GL_MAP_PERSISTENT_BIT |
        GL_MAP_COHERENT_BIT;
  

    rndm_field.upload_persistant(surface_flags);
    rndm_field_mem = (float*)rndm_field.map_full(surface_flags);
    glerror(" e8");

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

   glerror(" e9");
   
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

   glerror(" e10");

    glVertexAttribPointer(
        noise_layout,
        1,
        GL_FLOAT,
        GL_TRUE,
        sizeof(float),
        (void*)(noise_layout_stride)
    );
    glEnableVertexAttribArray(noise_layout);
    glerror(" e11");


    glVertexAttribPointer(
        noiseclr_layout,
        1,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float),
        (void*)noise_layout_stride
    );
    glEnableVertexAttribArray(noiseclr_layout);

    glerror(" e12");

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

    glerror(" e13");

    glUseProgram(program);
    glUniform1i(grid_szLoc, grid_sz);
    //glUniform2fv(mapscaleLoc, 2, &mapscale[0]);
    
    glerror(" e14");

}

GLuint surface_setup() {
    
    surface_program = glCreateProgram();
    

    reader.compile_shader_for("surface", GL_VERTEX_SHADER, surface_program);
    reader.compile_shader_for("surface", GL_FRAGMENT_SHADER, surface_program);

    linkprogram(surface_program);

    glerror("before surface_setup");
    setup_surface(surface_program);
    glerror("after surface_setup");
    
    return surface_program;
}

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


void draw_field(bool inp) {
    glUseProgram(surface_program);
    
    glBindVertexArray(vaos[0]);

    if (win_resized) {
        glUniform2fv(mapscaleLoc, 1, &mapscale[0]);
        win_resized = false;
    }

    if (inp) update_MVP_n_send(mvpLoc);
    
    glDrawElementsInstanced(
        GL_TRIANGLE_STRIP,
        sur_ebo.draw_count(),
        GL_UNSIGNED_SHORT,
        nullptr,
        3);

    draw_done = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

GLuint grid_program;
GLuint grid_shader_grid_szLoc;
GLuint grid_mvpLoc;

void grid_setup() {

    grid_program = glCreateProgram();
    

    reader.compile_shader_for("line", GL_VERTEX_SHADER, grid_program);
    reader.compile_shader_for("line", GL_FRAGMENT_SHADER, grid_program);

    linkprogram(grid_program);

    grid_shader_grid_szLoc = glGetUniformLocation(grid_program, "grid_sz");
    grid_mvpLoc = glGetUniformLocation(grid_program, "MVP");


   
    glUseProgram(grid_program);
    glUniform1i(grid_shader_grid_szLoc, grid_sz);
   
    //sur_ebo.bind();
    //rndm_field.bind();
    // 
    //glBindBuffer(GL_SHADER_STORAGE_BUFFER, rndm_field.id);
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, rndm_field.id);
    
}


void draw_grid(bool inp) {

    glUseProgram(grid_program);
    glBindVertexArray(vaos[0]);
    if (inp) update_MVP_n_send(grid_mvpLoc);
    //glBindVertexArray(vaos[0]);

    glDrawArrays(GL_TRIANGLE_STRIP,0,3);

}

