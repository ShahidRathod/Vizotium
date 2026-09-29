#include <glad/glad.h>
#include <iostream>
template <int N, typename T> struct ZeroSafeArray {
	T data[N];
	ZeroSafeArray() {}
	T& operator[](int i) {
		return data[i];
	}

	constexpr int len() { return N; }
	int bytesize() { return sizeof(data); }

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
	bool is_flush = false;
	bool is_mapped = true;
	int map_start = -1;
	int map_end = -1;
	void* mapped_ptr;

	void init_buffer() {
		if constexpr (N > 0) {
			glGenBuffers(1, &id);
		}
	}
	bool is_binded() {
		return BindedBuffer<buffer_type>::id == this->id;
	}
	void bind() {
		if (!is_binded()) {
			glBindBuffer(buffer_type, this->id);
			BindedBuffer<buffer_type>::change(id);
		}
	}

	void upload(GLuint draw_type) {
		if (!is_binded()) std::cout << "not binded";
		if constexpr (N > 0) {
			glBufferData(buffer_type, N * sizeof(T), this->data, draw_type);
		}
	}

	void upload_persistant(GLuint flags) {
		if (!is_binded()) std::cout << "not binded";
		if constexpr (N > 0) {
			glBufferStorage(buffer_type, N * sizeof(T), this->data, flags);
		}
		is_persistant = true;
	}

	void* map(int start , int end,GLuint more_flags) {
		if (!is_binded()) std::cout << "not binded";
		GLuint flags = (is_persistant) ? GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT : GL_MAP_WRITE_BIT;
		std::cout <<"error " << glGetError();
		mapped_ptr = glMapBufferRange(
			buffer_type,
			start,
			end,
			flags | more_flags
		);
		return mapped_ptr;
	}

	void* map_full(GLuint more_flags = 0) {
		return map(0,this->bytesize(),more_flags);
	}

	void unmap() {
		glUnmapBuffer(buffer_type);
	}

	void flush(size_t start, size_t end) {
		glFlushMappedBufferRange(buffer_type, start, end);
	}

	void flushfull() {
		flush(0,this->bytesize());
	}

	~GlBuffer() {
		glDeleteBuffers(1, &id);
	}
};



template<int N, typename T>
using VBO = GlBuffer<N, T, GL_ARRAY_BUFFER>;

template<int N>
using PlainVBO = VBO<N, float>;


template <int N>
using EBO = GlBuffer<N, GLuint, GL_ELEMENT_ARRAY_BUFFER>;



