#include "pch.h"
#include "OpenGLSetup.h"
#include "Gaussian.h"


enum class OptimizerType : int {
	Plain, Adam, Rms , COUNT
};



constexpr int edge_layout = 1;

template <int trail_sz>
struct Optimizer {
	OptimizerType type;

	using TrailVbo = PersistantVBO<trail_sz, glm::vec3>;

	TrailVbo vbos[2];
	GLuint vaos[2];

	int points_left = 0;
	int points_alloc = 0;
	int point_count = 0;
	bool state = 0;

	Optimizer() {}

	void flip_state() {
		state = !state;
	}


	void vbo_setup(TrailVbo& vbo, GLuint& vao) {


		vbo.init_buffer();
		vbo.map_coherent();
		vbo.map_write();

		glBindVertexArray(vao);

		vbo.bind();

		glVertexAttribPointer(
			edge_layout,
			6,
			GL_FLOAT,
			GL_TRUE,
			sizeof(glm::vec3),
			nullptr
		);

		glEnableVertexAttribArray(edge_layout);
	}

	void gl_setup() {

		glGenVertexArrays(2, vaos);
		vbo_setup(vbos[0], vaos[0]);
		vbo_setup(vbos[1], vaos[1]);

	}

	void add_point(glm::vec3 point) {

		if (points_left) {
			TrailVbo& currnt_vbo = vbos[state];

			currnt_vbo.map();
			currnt_vbo[point_count] = point;
			currnt_vbo.unmap();
			points_left--;
		}
		else {

			TrailVbo& src_vbo = vbos[state];
			TrailVbo& dst_vbo = vbos[!state];
			
			dst_vbo.bind();
			points_alloc += trail_sz;

			dst_vbo.relloc(GL_DYNAMIC_DRAW,point_count);
			
			glBindBuffer(GL_COPY_READ_BUFFER, src_vbo.id);
			glBindBuffer(GL_COPY_WRITE_BUFFER, dst_vbo.id);

			
			glCopyBufferSubData(
				GL_COPY_READ_BUFFER, 
				GL_COPY_WRITE_BUFFER, 
				0, 
				0, 
				points_alloc*sizeof(glm::vec3));


		}
	}
};

constexpr int trail_sz = 200;
constexpr int optim_count = static_cast<int>(OptimizerType::COUNT);
Optimizer<trail_sz> optimizers[optim_count];


GLuint trial_program;

void optimizer_setup() {
	
	for (int i = 0; i < optim_count;i++) optimizers[i].gl_setup();

	trial_program = glCreateProgram();

	reader->compile_shader_for("trial", GL_VERTEX_SHADER, trial_program);
	reader->compile_shader_for("trial", GL_FRAGMENT_SHADER, trial_program);

	linkprogram(trial_program);

	glUseProgram(trial_program);

}
