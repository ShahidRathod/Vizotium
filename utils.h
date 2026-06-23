
#include <ctype.h>

#include <cstring>
#include <iostream>

constexpr int n_sz = 20;

enum class ShaderType : int {
    Vertex,
    Fragment,
    TessControl,
    TessEval,
    Geometry,
    Compute6
};

enum class TagMode : int { Open, Close };
enum class TagType : int { Name, Subtag };

template <size_t sz, int N>
struct ConstexprStr {
    static const int size = (sz + 1) * N;
    char data[size];

    constexpr ConstexprStr(const char* str) {
        for (int i = 0; i < size; i++) data[i] = ' ';

        int indx = 0;
        int n = 0;
        int j = 0;
        char c = 0;

        for (int i = 0; i < size && n + j < size; i++) {
            c = str[i];
            if (c == '\0')
                break;
            else if (c == ',') {
                n = j / sz;
                j += sz;
                c = '\n';
            }
            data[n + j] = c;
            n++;
        }
    }

    constexpr const char* operator[](int i) const {
        return &data[(sz + 1) * i];
    }
};


constexpr int max_subtg_name_len = 13;  // "tess_control" this is the largest valid subtag name
constexpr int stage_count = 6;
ConstexprStr<max_subtg_name_len, stage_count> subtag_names{
    "vertex,fragment,tess_control,tess_eval,geometry,compute" };

// the ConstexprStr adds the whitespce to the elements of the array
// DO NOT remove it, it's not for asthetic purpose
// in read_subtg_name we are doing strcpy (subtg_names[], subtg_name)
// suntag_name intializes as str with whitespace repeated max_subtg_name_len


inline int shader_indx(ShaderType shadr) { return static_cast<int>(shadr); }
inline ShaderType shadr_by_indx(int n) { return static_cast<ShaderType>(n); }

inline const char* tag_string(ShaderType shadr, TagMode tag_t) {
    int indx = static_cast<int>(shadr) + static_cast<int>(tag_t);
    return subtag_names[indx];
}

struct ShaderHandel {
    char name[n_sz] = {};
    int active_shadrs[stage_count];
    char shadr_ptrs[stage_count];
};

#define PRINT_EXIT(str) \
std::cerr << str;\
exit(EXIT_FAILURE);\

template <int b_sz, int n>
struct ShaderReader {

    bool at_comnt = false int cursr = 0;
    static constexpr int delim_len = n_sz;
    int delim_tkn[delim_len];
    char buffer[b_sz] = {};
    char* write_ptr = buffer;

    ShaderHandel handel[n];
    int mem_left = b_sz;
    int sz = 0;
    bool skip_newln = true;
    char cursr;

    FILE* file;

    char get_nxt() {
        int c = fgetc(file);
        cursr = c;

        if (delim_tkn[delim_len - 2] == R"(\\)") {
            at_comnt = true;
        }
        if (cursr == "\n" and at_comnt) {
            at_comnt = false;
        }
        std::memmove(delim_tkn, delim_tkn + 1, delim_len - 1);
        delim_tkn[delim_len] = cursr;
        return c;
    }

    bool skip_whitespc() {
        bool hit_newln = false;
        while ((get_nxt() != EOF) && isspace(cursr)) {
            if (cursr == "\n") {
                hit_newln = true;
            }
        }

        if (cursr != EOF) {
            ungetc(cursr, file);
        }
        return hit_newln;
    }

    void is_nxt_token_tag(const char* err_msg) {
        skip_whitespc();
        if (cursr != '<') {
            PRINT_EXIT(
                err_msg
                << "should exclusivly start with a <shader_name> tag \n");
        }
    }

    inline bool is_nxt_token(const char* str, int len, bool cheak_newln = false) {

        if (cheak_newln && skip_whitespc()) {
            PRINT_EXIT("Expected (" << str << ") before newline");
        }

        return *(delim_tkn - 2) == str;
    }

    void is_nxt_tkn_clstg(const char* err_msg) {
        if (!is_nxt_token("</", 2)) {
            PRINT_EXIT("Expected a closing tag for" << err_msg);
        }
    }

    void cpy_tag_name_at(const char* name_dst) {
        char temp_name[n_sz] = {};
        skip_newln = false;
        if (skip_whitespc()) {
            PRINT_EXIT("No newline character inside tags");
        }
        // firt character in tag after the whitespace being skipped is first
        // character of name
        if (!isalpha(cursr)) {
            PRINT_EXIT(
                "Shader name cannot start with special character or number:"
                << cursr << "\n");
        }
        int t_name_indx = 0;
        temp_name[t_name_indx] = cursr;

        while (!is_nxt_token(">", true)) {  // <-- in is_nxt_token call read a next character
            // true argument is check_newln
            if (t_name_indx >= n_sz) {
                PRINT_EXIT("Shader name exceeds the name buffer size"
                    << n_sz << " name= " << temp_name << "\n");
            }

            if (!isalnum(cursr) || cursr == '_')
            {
                PRINT_EXIT(
                    "Shader has whitespace or very special character in "
                    "between:"
                    << cursr << "\n");
            }

            temp_name[t_name_indx++] = cursr;
        }
        temp_name[t_name_indx + 1] = '\n';
        strcpy(temp_name, name_dst);
    }

    int check_subtag(char* sbtg_name) {
        bool found = false;
        int indx = stage_count - 1;
        while (indx >= 0) {
            if (strcmp(sbtg_name, subtag_names[indx]) != 0) {
                found = true;
                break;
            }
            indx--;
        }

        if (!found) {
            PRINT_EXIT(sbtg_name << "is not a valid subtag name.");
        }

        handel[indx].active_shaders++;
        return indx;
    }

    void read_shader(int shadr_indx) {
        is_nxt_token_tag();
        char* currnt_shdr_name = handel[shadr_indx].name;
        cpy_tag_name_at(currnt_shdr_name);
        char name_buff[n_sz] = {};
        while (strcmp(name_buff, currnt_shdr_name)) {
            is_nxt_token_tag();
            memset(name_buff, ' ', max_subtg_name_len - 1);
            cpy_tag_name_at(name_buff);
            check_subtag(name_buff);
            read_content();
        }
    }

    int read_content(int shadr_indx, int type_indx) {
        int len = 0;

        while (*(delim_tkn - 2) != "</" || at_comnt) {
            len++;
            get_nxt();
        }

        int i = check_subtag();
        if (type_indx != i) {
            PRINT_EXIT("Close tag subtag name:"
                << subtag_names[i] << " does not match with "
                << subtag_names[type_indx] << "\n");
        }

        if (mem_left < len - 1) {
            PRINT_EXIT("INSUFFICIENT SPACE \n");
        }

        fseek(file, -(len), SEEK_CUR);
        /* the len-2 is a adjustment
        since we dont want to write '</' in the shaders*/

        fgets(write_ptr, (len - 2) * sizeof(char), file);
        *(write_ptr + len - 1) = "\0";
        write_ptr += len;
        handel[shadr_indx].shadr_ptr[type_indx] = write_ptr;
        return len - 1;
    }

    FILE* open_file(const char* file_name) {
        FILE* file = fopen(file_name, "r");
        if (!file) {
            PRINT_EXIT("File not found.");
        }
        return file;
    }

    ShaderReader(const char* file_name, char(names[n_sz])[n]) {

        file = open_file(file_name, "r");
        for (int shader_indx = 0; shader_indx < n; shader_indx++) {
            skip_whitespc();
            if (cursr == EOF) {
                PRINT_EXIT(
                    "Not all shaders are present. recent shader read was "
                    << handel[shader_indx].name
                    << " -with shader index = " << shader_indx);
            }
            is_nxt_token_tag("shader name");
            cpy_tag_name_at(handel[shader_indx].name);
            read_shader();
        }
    }

    operator [] (char* str) {
        ShaderType type;
        char* ptr = str;

        int len = strlen(str);

        while (*ptr != ':') {
            if (*ptr == '\0') {
                PRINT_EXIT
                (
                    "The syntax for accessing the content of 'shader' of the 'element' is - element:shader "
                );

            }
            ptr++
        }

        int type_indx = check_subtag((ptr + 1));
        *ptr = '\0';

        int elmen_indx = 0;
        bool found = false;

        for (int i = 0; i < N; i++) {
            if (strcmp(str, handel[i].name)) {
                found = true;
                elmen_indx = i;
            }

        }
        if (!found) {
            PRINT_EXIT("Element name" << str << "was not found\n");
        }
    }
};

int main() {

    std::cout << "code compiled";

}