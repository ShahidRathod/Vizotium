#include <glad/glad.h>
#include <iostream>
template <int N, typename T> struct ZeroSafeArray {
	T data[N];
	ZeroSafeArray() {}
	T& operator[](int i) {
		return data[i];
	}
};
template <typename T> struct ZeroSafeArray<0, T> {
	ZeroSafeArray() {}
};

template <GLuint buffer_type>
struct BindedBuffer {
	static GLuint id;
	static void change(GLuint i) {
		id = i;
	}
};


template <GLuint buffer_type>
GLuint BindedBuffer<buffer_type>::id = -1;

template <int N, typename T, GLuint buffer_type>
struct GlBuffer : ZeroSafeArray<N, T> {
	GLuint id;
	bool is_persistant = false;

	void init_gl_buffer() {
		if constexpr (N > 0) {
			glGenBuffers(1, &id);
		}
	}
	bool is_binded() {
		return BindedBuffer<buffer_type>::id == this->id;
	}
	void bind() {
		if (!is_binded()) glBindBuffer(buffer_type, this->id);
		BindedBuffer<buffer_type>::change(id);
	}

	void upload(GLuint draw_type) {
		if constexpr (N > 0) {
			bind();
			glBufferData(buffer_type, N * sizeof(T), this->data, draw_type);
		}
	}

	void upload_persistant(GLuint draw_type) {
		if constexpr (N > 0) {
			bind();
			glBufferData(buffer_type, N * sizeof(T), this->data, draw_type);
		}
		is_persistant = true;
	}

	void* map_full(GLuint more_flags) {
		bind();
		GLuint flags = (is_persistant) ?
			GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT : GL_MAP_WRITE_BIT;

		return glMapBufferRange(
			buffer_type,
			0,
			sizeof(this->data),
			flags | more_flags
		);
	}

	void flush(size_t start, size_t end) {
		glFlushMappedBufferRange(buffer_type, start, end);
	}
};




template<int N>
using VBO = GlBuffer<N, float, GL_ARRAY_BUFFER>;


template <int N>
using EBO = GlBuffer<N, GLuint, GL_ELEMENT_ARRAY_BUFFER>;

