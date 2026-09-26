#include <glad/glad.h>

template <int N, typename T> struct ZeroSafeArray {
	T data[N];
	ZeroSafeArray() {}
};
template <typename T> struct ZeroSafeArray<0,T> {
	ZeroSafeArray() {}
};

template <int N, typename T,GLuint buffer_type> 
struct GlBuffer : ZeroSafeArray<N,T> {
	GLuint id;
	void init() { 
		if constexpr (N > 0) {
			glGenBuffers(1,&id);
		}
	}

	void upload(GLuint draw_type) {
		if constexpr (N > 0) {
			glBindBuffer(buffer_type, id);
			glBufferData(buffer_type, N * sizeof(T), this->data, draw_type);
		}
	}
};

template<int N>
using VBO = GlBuffer<N, float, GL_ARRAY_BUFFER>;

template <int N>
struct MappedVBO : VBO<N> {

	MappedVBO(GLuint access) : VBO<N>() {
		glMapBuffer(this->id,access);
	}

	void commit_data() {
		
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
};

template <int N, typename T, GLuint buffer_type>
void bindbuffer(GlBuffer<N,T,buffer_type> buff) {
	glBindBuffer(buffer_type,buff.id);
}

template <int N>
using EBO = GlBuffer<N, float, GL_ELEMENT_ARRAY_BUFFER>;




template <int N,typename T>
void give_attribute_at(VBO<N>& vbo , int count, int stride , int loc) {
	bindbuffer(vbo);
	glVertexAttribPointer(
		loc,
		sizeof(T)/sizeof(float),
		GL_FLOAT,
		GL_FALSE,
		sizeof(T),
		nullptr
	);

	glEnableVertexAttribArray(loc);
}


template <int N, typename T>
void give_attribute_at(VBO<N>& vbo, EBO<N>& ebo, int count, int stride, int loc) {
	bindbuffer(ebo);
	bindbuffer<T>(vbo,count,stride,loc);
}


template<int VN , int EN>
struct DrawHandel {

	VBO<VN>* vbo;
	EBO<EN>* ebo = nullptr;
	GLuint vao_id;


	DrawHandel(VBO<VN>& vb = nullptr,EBO<EN>& eb = nullptr) {
		vbo = &vb;
		ebo = &eb;
		glGenVertexArrays(1, &vao_id);
	}

	void bind_draw_buffer() {
		if (vbo) bindbuffer(vbo);
		if (ebo) bindbuffer(ebo);

	}

	template <typename T>
	void give_attribute_at(int count, int stride, int loc) {
		bind_draw_buffer();

		glBindVertexArray(vao_id);
		glVertexAttribPointer(
			loc,
			sizeof(T) / sizeof(float),
			GL_FLOAT,
			GL_FALSE,
			sizeof(T),
			nullptr
		);

		glEnableVertexAttribArray(loc);
	}

	void draw(int draw_count  = -1) {
		if (draw_count = -1) draw_count = EN;
		glBindVertexArray(vao_id);
		glDrawElements(
			GL_TRIANGLES, // our fundamental draw is triangle
			draw_count,
			GL_UNSIGNED_INT,
			(void*)(0)
		);
	}
};

