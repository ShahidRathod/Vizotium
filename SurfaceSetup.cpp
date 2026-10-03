#include "pch.h"

#include "glbuffers.h"
#include "Gaussian.h"

// Free function defined in OpenGLSetup.h (compiled via Vizotium.cpp).
// Forward-declared here (before ShaderLoader.h) so the template
// definition context in that header sees the name; no logic change.
GLuint compile_shader(GLenum type, const char* src);

#include "ShaderLoader.h"

// Free function defined in OpenGLSetup.h (compiled via Vizotium.cpp).
// Forward-declared here (before ShaderLoader.h) so the template
// definition context in that header sees the name; no logic change.
GLuint compile_shader(GLenum type, const char* src);


// Defined in OpenGLSetup.h (compiled via Vizotium.cpp); declared here
// so SurfaceSetup.cpp stays a single file without that header.
void linkprogram(GLuint prog);

constexpr int grid_pow = 7;
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

// Single shared shader reader: every shader compile uses this.
ShaderReader<4000, 64> reader("shaders.h");

// Surface program handle, its uniform locations, and its VAOs.
// Locations are queried once in setup_surface; draw_field re-binds
// program + vao explicitly every frame (more programs will come).
GLuint surface_program = 0;
GLuint grid_szLoc;
GLuint mvpLoc;
GLuint mapscaleLoc;
GLuint vaos[4]; // vaos[0] is the surface vao bound for drawing

void setup_surface(GLuint program) {
    memcpy(offset.data, &offset_data, sizeof(offset_data));

    grid_szLoc = glGetUniformLocation(program, "grid_sz");
    mvpLoc = glGetUniformLocation(program, "MVP");
    mapscaleLoc = glGetUniformLocation(program, "mapscale");

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


    glUseProgram(program);
    glUniform1i(grid_szLoc, grid_sz);
    glUniform2fv(mapscaleLoc, 2, &mapscale[0]);
}

GLuint surface_setup() {
    surface_program = glCreateProgram();

    reader.compile_shader_for("surface", GL_VERTEX_SHADER, surface_program);
    reader.compile_shader_for("surface", GL_FRAGMENT_SHADER, surface_program);

    linkprogram(surface_program);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);

    setup_surface(surface_program);

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

void update_mapscale() {
    if (win_resized) {
        glUniform2fv(mapscaleLoc, 1, &mapscale[0]);
        win_resized = false;
    }
}

void draw_field() {
    glUseProgram(surface_program);
    glBindVertexArray(vaos[0]);

    glDrawElementsInstanced(
        GL_TRIANGLE_STRIP,
        sur_ebo.draw_count(),
        GL_UNSIGNED_SHORT,
        nullptr,
        3);

    //glDrawArrays(GL_TRIANGLE_STRIP, 0, grid_sz_sq);

    draw_done = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}
