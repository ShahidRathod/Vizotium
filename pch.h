#pragma once

// Precompiled header: stable third-party and standard headers shared by
// every translation unit. Compiled once, reused by all .cpp files.
// Do NOT add frequently-changing project headers here (glbuffers.h,
// Gaussian.h, ShaderLoader.h, ...): touching pch.h forces a full rebuild.
// Each .cpp must include this file FIRST, before any other code.

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <numbers>
