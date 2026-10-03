#include <glad/glad.h>
#include <glm/glm.hpp>

#include "glbuffers.h"
#include "Gaussian.h"

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

// Mapped pointer to rndm_field storage (defined in Vizotium.cpp)
extern float* rndm_field_mem;

void setup_surface(GLuint surface_program,
                   GLuint& grid_szLoc,
                   GLuint& mvpLoc,
                   GLuint& mapscaleLoc) {
    grid_szLoc = glGetUniformLocation(surface_program, "grid_sz");
    mvpLoc = glGetUniformLocation(surface_program, "MVP");
    mapscaleLoc = glGetUniformLocation(surface_program, "mapscale");

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
}
