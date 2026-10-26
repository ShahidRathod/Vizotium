#pragma once

#define RED "\x1b[31m"
#define RESET "\x1b[0m"


template <int N, typename T> struct ZeroSafeArray {
	T buffer[N];
	T* data = nullptr;

	ZeroSafeArray() {
		data = buffer;
	}
	int bytesize() { return sizeof(buffer); }

};

template <typename T> struct ZeroSafeArray<0, T>  {
	T* data = nullptr;
	T* buffer = nullptr;

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

template <int Cbuff_N,int Gbuff_N, typename T, GLuint buffer_type>
struct GlBuffer : ZeroSafeArray<Cbuff_N, T> {
	GLuint id;

	GLuint map_flags = 0;

	constexpr int Gbytesize() { return sizeof(T) * Gbuff_N; }

	void init_buffer() {
		glGenBuffers(1, &id);
	}
	
	void map_flushexplisit() { map_flags |= GL_MAP_FLUSH_EXPLICIT_BIT; }
	void map_persistant() { map_flags |= GL_MAP_PERSISTENT_BIT; }
	void map_coherent() { map_flags |= GL_MAP_COHERENT_BIT; }
	void map_write() { map_flags |= GL_MAP_WRITE_BIT; }

	bool is_binded() {
		return BindedBuffer<buffer_type>::id == this->id;
	}

	void bind() {
		if (!is_binded()) {
			glBindBuffer(buffer_type, this->id);
			BindedBuffer<buffer_type>::change(id);
		}
	}

	virtual void upload(GLuint draw_type, int count = 0) {
		if (!is_binded()) std::cout << "not binded";
		int size = ((count == 0) ? Gbuff_N : count) * sizeof(T);
		glBufferData(buffer_type, size, this->buffer, draw_type);
	}

	void upload_persistant(int count = 0) {
		if (!is_binded()) std::cout << "not binded";
		int size = ((count == 0) ? Gbuff_N : count) * sizeof(T);
		glBufferStorage(buffer_type, size, this->buffer, map_flags);
	}

	void map(int start, int end) {
		if (!is_binded()) std::cout << "not binded";
		void* mapped_ptr = glMapBufferRange(
			buffer_type,
			start,
			end,
			map_flags
		);
		this->data = (T*)(mapped_ptr);
	}

	void map_full() { 
		map(0, Gbytesize()); }

	void unmap() {
		glUnmapBuffer(buffer_type);
		this->data = this->buffer;
	}

	void flush(size_t start, size_t end) {
		glFlushMappedBufferRange(buffer_type, start, end);
	}

	void flushfull() { 
		flush(0,Gbytesize());
	}

	void delete_buffer() {
		glDeleteBuffers(1, &id);
	}

};


template <int N,typename T,GLuint buffer_type>
struct PersistantBuffer : GlBuffer<0,N,T,buffer_type>{};



template<int CN,int GN, typename T>
using VBOBase = GlBuffer<CN,GN, T, GL_ARRAY_BUFFER>;

template<int N, typename T>
using PersistantVBO = GlBuffer<0, N, T, GL_ARRAY_BUFFER>;

template<int N, typename T>
using VBO = GlBuffer<N, N, T, GL_ARRAY_BUFFER>;

template<int N>
using PersistantPlainVBO = VBOBase<0, N, float>;

template<int N>
using PlainVBO = VBO<N, float>;

template <typename T,int N>
struct LineEboGlStrip {
	T start;
	T arr[2 * (N)];
	T end;
	
	void make_degenerates() { 
		end = arr[2 * N - 1]; 
		start = arr[0];
	}

	void print() {
		//std::cout<<(int)start[0]<<" "<<(int)start[1]<<" ";
		for (int i = 0; i < 2 * N; i++) std::cout << (int)arr[i] << " ";
		std::cout << (int)end;
	}
};


using EboInt = unsigned short int;

template <int N>
using SurfaceEBOBuffer = 
GlBuffer<
	N-1,N-1, 
	LineEboGlStrip<EboInt,N>, 
	GL_ELEMENT_ARRAY_BUFFER>;

template <int N>
struct SurfaceEBO : SurfaceEBOBuffer<N> {

	void init_buffer() {

		SurfaceEBOBuffer<N>::init_buffer();
		for (EboInt i = 0; i < N - 1; i++) {
			LineEboGlStrip<EboInt, N>& line = this->data[i];
			for (EboInt j = 0; j < N; j++) {
				line.arr[2 * j] = i * N + j;
				line.arr[2 * j + 1] = j + (i + 1) * (N);
			}

			line.make_degenerates();
		}
	}

	void print() {
		std::cout << "\n";
		for (int i = 0; i < N - 1; i++) {
			this->data[i].print();
			std::cout << "\n";
		}
	}

	constexpr int draw_count() {
		return this->Gbytesize()/ (sizeof(EboInt));
	}

};

#define glerror(str)  \
int e = glGetError();\
std::cerr <<RED<< str << e << "\n"<<RESET;\
}




