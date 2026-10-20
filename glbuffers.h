#pragma once



template <int N, typename T> struct ZeroSafeArray {
	T data[N];
	ZeroSafeArray() {}
	T* data_ptr = nullptr;
	T& operator[](int i) {
		return data_ptr[i];
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

template <int Cbuff_N,int Gbuff_N, typename T, GLuint buffer_type>
struct GlBuffer : ZeroSafeArray<Cbuff_N, T> {
	GLuint id;

	GLuint map_flags = 0;

	constexpr int GbyteSize() { return sizeof(T) * Gbuff_N; }

	void init_buffer() {
		data_ptr = data;
		glGenBuffers(1, &id);
	}

	void flush_explisit() { map_flags |= GL_MAP_FLUSH_EXPLICIT_BIT;}
	void map_persistant() { map_flags |= GL_MAP_PERSISTENT_BIT;}
	void map_coherent() {  map_flags |= GL_MAP_COHERENT_BIT;}

	bool is_binded() {
		return BindedBuffer<buffer_type>::id == this->id;
	}

	void bind() {
		if (!is_binded()) {
			glBindBuffer(buffer_type, this->id);
			BindedBuffer<buffer_type>::change(id);
		}
	}

	virtual void upload(GLuint draw_type) {
		if (!is_binded()) std::cout << "not binded";
		glBufferData(buffer_type, this->bytesize(), this->data_ptr, draw_type);
	}

	void upload_persistant(GLuint flags) {
		if (!is_binded()) std::cout << "not binded";
		glBufferStorage(buffer_type, GbyteSize(), this->data_ptr, map_flags);
	}

	void* map(int start , int end) {
		if (!is_binded()) std::cout << "not binded";
		mapped_ptr = glMapBufferRange(
			buffer_type,
			start,
			end,
			map_flags
		);
		return mapped_ptr;
	}

	void* map_full() { 
		return map(0,Gbytesize()); 
	}

	void unmap() {
		glUnmapBuffer(buffer_type);
	}

	void flush(size_t start, size_t end) {
		glFlushMappedBufferRange(buffer_type, start, end);
	}

	void flushfull() {
		flush(0,Gbytesize());
	}

	~GlBuffer() {
		glDeleteBuffers(1, &id);
	}

};


template <int N,typename T,GLuint buffer_type>
struct PersistantBuffer : GlBuffer<0,N,T,buffer_type>{
	PersistantBuffer() {
		map_persistant();
		data_ptr = (T*)map_full();
	}
};



template<int CN,int GN, typename T>
using VBO = GlBuffer<CN,GN, T, GL_ARRAY_BUFFER>;

template<int N, typename T>
using PersistantVBO = GlBuffer<0, N, T, GL_ARRAY_BUFFER>;


template<int N>
using PersistantPlainVBO = VBO<0, N, float>;


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
		return Gbytesize()/ (sizeof(EboInt));
	}


};

#define glerror(str)  std::cout <<  str << glGetError() << "\n";
