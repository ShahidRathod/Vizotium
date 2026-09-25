#include <glad/glad.h>



template <int N, typename T> struct ZeroSafeArray { T data[N]; };
template <typename T> struct ZeroSafeArray<0,T> {};

template <int N, typename T,GLuint buffer_type> 
struct GlBuffer : ZeroSafeArray<N,T> {

	GLuint id;
	GlBuffer() { 
		if constexpr (N > 0) {
			glGenBuffers(1, &id);
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
using VBOBase = GlBuffer<N, float, GL_ARRAY_BUFFER>;

template <int N>
struct VBO : VBOBase<N> {
	int location;
	VBO(int loc) : VBOBase<N>() {
		location = loc;
	}

};

template <int N, typename T, GLuint buffer_type>
void bindbuffer(GlBuffer<N,T,buffer_type> buff) {
	glBindBuffer(buffertype,buff.id);
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

	glEnableVertexAttribArray(state.loc);
}


template <int N, typename T>
void give_attribute_at(VBO<N>& vbo, EBO<N>& ebo, int count, int stride, int loc) {
	bindbuffer(ebo);
	bindbuffer<T>(vbo,count,stride,loc);
}
