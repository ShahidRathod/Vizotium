#pragma once
#include <iostream>
#include <ctype.h>
#include <cstring.h>
constexpr int n_sz = 20;

constexpr char* vertex_tag_st = "<vertex>";
constexpr char* fragment_tag_st = "<fragment>";
constexpr char* vertex_tag_en = "</vertex>";
constexpr char* vertex_tag_en = "</fragment>";

enum class ShaderType: int{
	Vertex, Fragment , TeseellationControl,TessellationEval,Geometry,Compute
};



inline int shader_indx(ShaderType shadr) {
	return static_cast<int>(shadr);
}
inline ShaderType shadr_by_indx(int n) {
	return static<ShaderType>(n);
}


constexpr int stage_count = 6;


struct ShaderHandel{
	char name[n_sz];
	bool active_shadrs[stage_count];
	char shadr_ptrs[stage_count];
};

# define PRINT_EXIT(str) \
std::cerr << str;
exit(EXIT_FAILURE);\


template <int b_sz,int n>
struct ShaderReader {

	constexpr char* shader_tag_err = "shader file";
	constexpr char* subtag_err = "shader file";

	ShaderHandel[]
	FILE* file;

	
    int read_content(int n,int sz) {

	}
	void skip_whitespc(bool strict) {
		int c;
		while ((c = fgetc(file) != EOF) and isspace(c));

		if (!strict and c != EOF) {
			ungetc(c, file);
		}
	}

	void is_nxt_token_tag(const char* err_msg) {
		skip_whitespc(true);

		int c = fgetc(file);

		if (c != '<') {
			PRINT_EXIT(err_msg<<"should exclusivly start with a <shader_name> tag \n");
		}
	}

	int read_name(int n) {
		int c;
		char temp_name[n_sz];
		int t_name_indx = 0;

		skip_whitespc(true);

		if (!isalpha(c)) {
			PRINT_EXIT("Shader name cannot start with special character :" << c << "\n");
		}
		temp_name[t_name_indx] = c;
		while (c != '>') {
			if (t_name_indx >= n_sz) {
				PRINT_EXIT("Shader name exceeds the name buffer size" << n_sz << " name= " << temp_name << "\n");
			}

			c = putc(file);
			if (isspace(c) or !isalnum(c)) {
				if (c != '_') {
					PRINT_EXIT("Shader has whitespace or very special character in between:" << c << "\n");
				}
			}

			temp_name[t_name_indx++] = c;
		}
		strcpy(temp_name,shaders[n].name);
		
	}

	int is_in_names(char* name) {
		
	}
	
	FILE* open_file(const char* file_name) {
		FILE* file = fopen(file_name);
		if (!file) {
			PRINT_EXIT("File not found.");
		}
		return file;
	}
	
	Shader(const char* file_name,char (names[n_sz])[n]) {
	
		
		file = open_file(file_name);
		
		for (int shader_indx = 0; shader_indx < n;shader_indx++) {
			if ((c = putc(file) != EOF ) {
				PRINT_EXIT("Not all shaders are present. recent shader read was "
					<<shaders[shader_indx].name<<" -with shader index = "<<shader_indx);
			}

			is_nxt_token_tag(shader_tag_err);
			read_name(shader_indx);
			is_nxt_token_tag(subtag_err);
			read_sub_tag(shader_indx);
			int len = get_content_len();

		}
	}
};

int main() {
	

}