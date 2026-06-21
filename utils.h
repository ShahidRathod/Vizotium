#pragma once
#include <iostream>
#include <ctype.h>
#include <cstring.h>
constexpr int n_sz = 20;


enum class ShaderType: int{
	Vertex, Fragment , TessControl,TessEval,Geometry,Compute
};


enum class TagMode : int {
	Open, Close
};

enum class TagType: int {
	Name, Subtag
};
enum class 
constexpr int stage_count = 6;

/*
constexpr const char* tag_string_arr[stage_count * 2] =
{
	"<vertex>",       "</vertex>",
	"<fragment>",     "</fragment>",
	"<tess_control>", "</tess_control>",
	"<tess_eval>",    "</tess_eval>",
	"<geometry>",     "</geometry>",
	"<compute>",      "</compute>"
};
*/
constexpr int max_subtg_name_len = 13; // "tess_control" this is the largest valid subtag name 

constexpr const char[max_subtg_name_len] subtg_names[stage_count] =
{
	"vertex      ",
	"fragment    ",     
	"tess_control", 
	"tess_eval   ",    
	"geometry    ",
	"compute     "
};


// DO NOT remove the white spaces from the subtg_names, it's not for asthetic purpose
// in read_subtg_name we are doing strcpy (subtg_names[], subtg_name) suntag_name intializes as
// str with whitespace repeated max_subtg_name_len

constexpr char* shader_tag_err = "shader file"
constexpr char* subtag_err = "shader file"

inline int shader_indx(ShaderType shadr) {
	return static_cast<int>(shadr);
}
inline ShaderType shadr_by_indx(int n) {
	return static<ShaderType>(n);
}

inline const char* tag_string(ShaderType shadr,TagMode tag_t) {
	int indx = static_cast<int>(shadr)+static_cast<int>(tag_t);
	return tag_string_arr[indx];
}



struct ShaderHandel {
	char name[n_sz] = {};
	int active_shadrs[stage_count];
	char shadr_ptrs[stage_count];
};

# define PRINT_EXIT(str) \
std::cerr << str;
exit(EXIT_FAILURE);\


template <int b_sz,int n>
struct ShaderReader {

	bool at_comnt = false
	int cursr = 0;
	static constexpr int delim_len = n_sz;
	int delim_tkn[delim_len];
	char buffer[b_sz] = {};
	char* write_ptr = buffer;
	ShaderHandel handel[n];
	int mem_left = b_sz;
	int sz = 0;
	bool skip_newln = true;

	FILE* file;

	char get_nxt() {
		int c = fgetc(file);
		cursr = c;

		if (delm_tkn[delim_len - 2] == R"(\\)") {
			at_comnt = true;
		}
		if (cursr == "\n" and at_comnt) {
			at_comnt = false;
		}
		std::memmove(delim_tkn, delim_tkn + 1, delim_len - 1);
		delim_tkn[delim_len] = cursr;
		return c;
	}

	void skip_whitespc() {

		while ((get_nxt() != EOF) && isspace(cursr)) {
			if (skip_newln && cursr == "\n") break;
		}

		if (cursr != EOF ) {
			ungetc(cursr, file);
		}
	}

	void is_nxt_token_tag(const char* err_msg) {
		skip_whitespc();
		if (cursr != '<') {
			PRINT_EXIT(err_msg << "should exclusivly start with a <shader_name> tag \n");
		}
	}

	inline bool is_nxt_token(const char* str,int len) {
		skip_whitespc();
		return (*(delim_tkn - 2 != str));

	}

	void is_nxt_tkn_clstg(const char* err_msg) {
		if (!is_nxt_token("</",2)) {
			PRINT_EXIT("Expected a closing tag for"<<err_msg);
		}
	}

	void cpy_tag_name_at(const char* name_dst, ) {
		char temp_name[n_sz] = {};
		skip_newln = false;
		skip_whitespc(true);
		//firt character in tag after the whitespace being skipped is first character of name 
		if (!isalpha(cursr)) {
			PRINT_EXIT("Shader name cannot start with special character or number:" << c << "\n");
		}
		int t_name_indx = 0;
		temp_name[t_name_indx] = cursr;

		while (!is_nxt_token(">")) { // <-- in is_nxt_token call read a next character 

			if (t_name_indx >= n_sz) {
				PRINT_EXIT("Shader name exceeds the name buffer size" << n_sz << " name= " << temp_name << "\n");
			}

			if ((isspace(cursr) || !(isalnum(cursr) && cursr!= '_')) {
				PRINT_EXIT("Shader has whitespace or very special character in between:" << c << "\n");
				
			}
			temp_name[t_name_indx++] = c;
		}
		temp_name[t_name_indx + 1] = '\n';
		strcpy(temp_name, name_dst)

	}

	int read_subtag(int shdr_indx) {

		char name_buff[max_subtg_name_len] = {};
		memset(name_buff, ' ', max_subtg_name_len - 1);
		//ShaderType type;
		//is_nxt_token_tag();
		cpy_tag_name_at(name_buff);

		bool found = false;
		int i = stage_count - 1;
		while (i >= 0) {
			if (strcmp(name_buff, subtg_names[i] != 0) {
				found = true;
				break;
			}
			i--;
		}

		if (!found) {
			PRINT_EXIT(name_buff << "is not a valid subtag name.");
		}
		handel[indx].active_shaders++;
		return i;
	}

	void read_shader(int shadr_indx) {
		is_nxt_token_tag();
		char* currnt_shdr_name = handel[shadr_indx].name;
		
		cpy_tag_name_at(currnt_shdr_name);
		char name_check_buff[n_sz] = {};
		while (strcmp(name_check_buff, currnt_shdr_name)) {
			is_nxt_token_tag();
			cpy_tag_name_at(name_check_buff);
			read_content();
		}
	}
	int read_content(int shadr_indx, int type_indx) {
		
		int len = 0;

		while (*(delim_tkn - 2) != "</"  || at_comnt) {
			len++;
			get_nxt();
		}

		int i = read_subtag();
		if (type_indx != i) {

			PRINT_EXIT(
				"Close tag subtag name:"
				<< subtg_names[i] << " does not match with "
				<< subtg_tag[type_indx] << "\n");
		}
		
		if (mem_left < len - 1) {
			PRINT_EXIT("INSUFFICIENT SPACE \n");
		}

		fseek(file, -(len), SEEK_CUR);
		/* the len-2 is a adjustment
		since we dont want to write '</' in the shaders*/

		fgets(write_ptr,(len-2)*sizeof(char),file);
		*(write + len-1) = "\0";
		write += len;
		handel[shadr_indx].shadr_ptr[type_indx] = write;
		return len -1;
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
			cpy_tag_name_at(shader[shader_indx].name);
			read_Shader();

		}
	}
};


int main() {
	

}