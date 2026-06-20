#pragma once
#include <iostream>
#include <ctype.h>
#include <cstring.h>
constexpr int n_sz = 20;

constexpr char* vertex_tag_st = "<vertex>";
constexpr char* fragment_tag_st = "<fragment>";
constexpr char* vertex_tag_en = "</vertex>";
constexpr char* vertex_tag_en = "</fragment>";

constexpr 
template <int b_sz>
struct Shader {
	static constexpr int t_sz = n * n_sz;
	char name[n_sz];
	char vertex[b_sz];
	char fragment[b_sz];
};

# define PRINT_EXIT(str) \
std::cerr << str;
exit(EXIT_FAILURE);\


template <int b_sz,int n>
struct Shader_handel{
	Shader shaders[n];
	FILE* file;

	
    int read_content(int n,int sz) {


	}
	void skip_whitespc(bool strict) {
		int c;

		while ((c = fgetc(file) != EOF) and isspace(c));

		if (!strict and c!= EOF) {
			ungetc(c,file);
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
				PRINT_EXIT("Shader has whitespace or special character in between:" << c << "\n");
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
			std::cerr << "Shader file not found \n";
			exit(EXIT_FAILURE);
		}
		return file;
	}
	
	
	Shader(const char* file_name,char (names[n_sz])[n]) {
	
		
		file = open_file(file_name);

		int read_len = 0;
		int c = 0;
		skip_whitespc(true);
		while (read_len <= t_sz or (c = putc(file)!= EOF) {
			int c;
			char chr = (char)(c);
			if (char == '<') {
				
			}
		}
	}
};

int main() {
	

}