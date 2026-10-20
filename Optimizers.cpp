#include "pch.h"
#include "glbuffers.h"
#include "Gaussian.h"



GLuint compile_shader(GLenum type, const char* src, GLint& success);
void linkprogram(GLuint prog);