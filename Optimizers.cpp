#include "pch.h"
#include "OpenGLSetup.h"
#include "Gaussian.h"


enum class OptimizerType : int {
	Plain, Adam, Rms
};

struct TrailEdge {
	glm::vec3 p1,p2;
};


template <int trail_sz>
struct Optimizer {
	OptimizerType type;
	VBO<TrailEdge,trail_sz> vbo;


};