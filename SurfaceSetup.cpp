#include "SurfaceSetup.h"

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
