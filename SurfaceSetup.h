#pragma once

#include <glm/glm.hpp>

#include "glbuffers.h"
#include "Gaussian.h"

// Grid constants (moved verbatim from Vizotium.cpp, no logic change)
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

// Globals defined in SurfaceSetup.cpp
extern VBO<3, glm::vec3> offset;
extern glm::vec3 offset_data[3];
extern glm::vec3* surface_offset;
extern glm::vec3* offset_ptr;
extern glm::vec2 mapscale;
extern bool win_resized;

extern SurfaceEBO<grid_sz> sur_ebo;

// random fields
// first half contains the rndm_field
// second half contains the noise
extern ComplexNoise<grid_pow> noise;
extern PlainVBO<grid_sz_sq * 2> rndm_field;
