#include <cstring>
#include <cctype>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstddef>

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

template <size_t sz, int N> struct ConstexprStr {
    static const int size = (sz + 1) * N;
    char data[size] = { ' ' };

    constexpr ConstexprStr(const char* str) {
        for (int i = 0; i < size; i++)
            data[i] = ' ';

        int indx = 0;
        int n = 0;
        int j = 0;
        char c = 0;

        for (int i = 0; i < size && n + j < size; i++) {
            c = str[i];
            if (c == '\0')
                break;
            else if (c == ',') {
                data[n + j] = '\0';
                n = j / sz + 1;
                j += sz;
                continue;
            }
            data[n + j] = c;
            n++;
        }
    }

    constexpr const char* operator[](int i) const {
        return &data[(sz + 1) * i];
    }
};

constexpr int max_subtg_name_len =
14; // "tess_control" this is the largest valid subtag name
constexpr int stage_count = 6;
ConstexprStr<max_subtg_name_len, stage_count> subtag_names{
    "vertex,fragment,tess_control,tess_eval,geometry,compute," };

inline int shader_indx(ShaderType shadr) { return static_cast<int>(shadr); }
inline ShaderType shadr_by_indx(int n) { return static_cast<ShaderType>(n); }

inline const char* tag_string(ShaderType shadr, TagMode tag_t) {
    int indx = static_cast<int>(shadr) + static_cast<int>(tag_t);
    return subtag_names[indx];
}




template <int sz> struct CircularBuff {
    int len = 0;
    char arr[sz + 1];

    CircularBuff() {
        arr[sz] = '\0';
    }
    int get_index(int x) { return x % sz; }

    void put_char(char c) {
        arr[get_index(len)] = c;
        arr[sz] = '\0';
        len++;
    }

    int compare_str(const char* str, int str_len) {

        bool is_equal = true;

        for (int i = 0; i < str_len; i++) {
            int indx = get_index(len - str_len + i);
            if (arr[indx] != str[i]) {
                is_equal = false;
                break;
            }
        }
        return is_equal;
    }

};




struct ShaderHandel {
    char name[n_sz] = {};
    int active_shaders[stage_count] = { 0 };
    int shdr_line_no[stage_count * 2] = { -1 };
    int start = -1;
    int end = -1;
    char* shadr_ptrs[stage_count] = { nullptr };

    char* operator[](const char* shdr);

};

#define PRINT_EXIT(str)                                                        \
    std::cerr << "LINE NO:(" << ln_no << "):" << char_no << " " << str;        \
    exit(EXIT_FAILURE);


#define PRINT_EXIT_NO_LINE(str)                                                        \
    std::cerr << str;        \
    exit(EXIT_FAILURE);


static int check_subtag(const char* sbtg_name) {
    bool found = false;

    int indx = stage_count - 1;
    while (indx >= 0) {
        if (strcmp(sbtg_name, subtag_names[indx]) == 0) {
            std::cout << sbtg_name << " " << subtag_names[indx] << "\n";
            found = true;
            break;
        }
        indx--;
    }

    if (!found) {
        PRINT_EXIT_NO_LINE(sbtg_name << " is not a valid subtag name.");
    }

    return indx;
}

template <int b_sz, int n> struct ShaderReader {

    bool at_comnt = false;
    bool in_tag = false;
    char cursr = 0;
    static constexpr int delim_len = n_sz;
    char delim_tkn[delim_len];
    char buffer[b_sz] = {};
    char* write_ptr = buffer;

    int ln_no = 1;
    int char_no = 0;
    int char_no_s = 0;
    ShaderHandel handel[n];
    int mem_left = b_sz;
    int sz = 0;
    bool skip_newln = true;

    // this is a fix to a deterministic anamoly 
    // in read content the every newline cha
    int new_ln_fix = 0;

    ShaderHandel* curnt_element = handel;
    int curnt_indx = 0;
    FILE* file;

    char get_nxt() {
        int c = fgetc(file);
        cursr = (char)c;

        if (strcmp(delim_tkn + delim_len - 2, R"(\\)") == 0) {
            at_comnt = true;
        }

        if (cursr == '\n') {

            ln_no++;
            new_ln_fix++;
            at_comnt = false;
        }

        std::memmove(delim_tkn, delim_tkn + 1, delim_len - 1);
        delim_tkn[delim_len - 1] = cursr;
        delim_tkn[delim_len] = '\0';
        char_no_s++;
        return c;
    }

    inline bool is_nxt_char(const char chr) {

        if (skip_whitespc()) {
            PRINT_EXIT("Expected (" << chr << ") before newline");
        }
        return delim_tkn[0] == chr;
    }

    bool skip_whitespc() {
        bool hit_newln = false;
        while ((cursr != EOF) && isspace(cursr)) {
            if (cursr == '\n') {

                hit_newln = true;
            }
            get_nxt();
        }
        return hit_newln;
    }

    void is_nxt_token_tag(const char* err_msg) {
        skip_whitespc();
        if (cursr != '<') {
            PRINT_EXIT(err_msg << "was expected before: " << (int)cursr << cursr
                << "\n");
        }
        get_nxt();
    }

    void cpy_tag_name_at(char* name_dst) {
        char temp_name[n_sz] = {};

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
        while (cursr != '>') {

            if (isspace(cursr)) {
                skip_whitespc();
                if (cursr == '>')
                    break;
                else
                    PRINT_EXIT("Shader name has whitespace in between ");
            }
            if (t_name_indx >= n_sz) {
                PRINT_EXIT("Shader name exceeds the name buffer size"
                    << n_sz << " name= " << temp_name << "\n");
            }

            if (isalnum(cursr) == 0 && cursr != '_') {
                PRINT_EXIT("Shader has very special character in "
                    "between:"
                    << cursr << "\n");
            }

            temp_name[t_name_indx++] = cursr;
            get_nxt();
        }

        get_nxt();
        temp_name[t_name_indx + 1] = '\n';
        strcpy(name_dst, temp_name);
    }

    void read_element() {
        char* element_name = curnt_element->name;

        is_nxt_token_tag("Element tag ");
        cpy_tag_name_at(element_name);

        curnt_element->start = ln_no;

        char name_buff[n_sz] = {};

        for (int i = 0; i < stage_count; i++) {
            is_nxt_token_tag("Element closing or Shader ");

            bool is_cls = !skip_whitespc() && cursr == '/';

            if (is_cls) get_nxt();

            cpy_tag_name_at(name_buff);

            std::cout << "Element tag name: " << element_name
                << "  Shader name tag: " << name_buff << "\n";

            if (is_cls) {
                if (strcmp(name_buff, element_name) != 0) {
                    PRINT_EXIT("Element("
                        << curnt_indx << "): open tag-" << element_name
                        << " does not match close tag-" << name_buff);
                }

                curnt_element->end = ln_no;
                break;
            }
            else {

                int type_indx = check_subtag(name_buff);

                curnt_element->shdr_line_no[2 * type_indx] = ln_no;

                std::cout << "active shader: "
                    << curnt_element->active_shaders[type_indx] << "\n";
                std::cout << "active shader profile: \n";

                
                if ((curnt_element->active_shaders[type_indx]++) > 1) {
                    PRINT_EXIT("Shader: "
                        << name_buff << " Element: " << curnt_element->name
                        << " is already defined line at ("
                        << curnt_element->shdr_line_no[2 * type_indx] << ", "
                        << curnt_element->shdr_line_no[2 * type_indx + 1]
                        << ")");
                }
                for (int i = 0; i < stage_count; i++) std::cout << subtag_names[i] << ": " << curnt_element->active_shaders[i] << "\n";

                std::cout << type_indx << "<-typeindx\n";

                read_content(type_indx, name_buff);
            }
        }
    }       



    int read_content(int type_indx, char* opn_shdr_tg) {
        new_ln_fix = 0;
        long cntn_strt = ftell(file) - 1;
        bool cls_found = false;

        while ((cursr != '<' || at_comnt) && cursr != EOF) {
            get_nxt();
        }

        long cntn_end = ftell(file) - 1;
        long tg_strt = ftell(file);
        long len = cntn_end - cntn_strt - new_ln_fix;

        get_nxt();

        if (!skip_whitespc() && cursr != '/') {
            PRINT_EXIT("Exprected a closing tag Maybe you forgot '/'\n");
        }

        get_nxt(); // to make the cursr past the '/' charater 
        //because in the cpt_tag_name_at has skip_whitepsc 
        // and it will terminate immediately if the cursr is not a whitespc

        curnt_element->shdr_line_no[2 * type_indx + 1] = ln_no;
        char cls_shdr_tg[max_subtg_name_len];
        cpy_tag_name_at(cls_shdr_tg);

        long tg_end = ftell(file);

        long tg_len = tg_end - tg_strt;

        std::cout << cls_shdr_tg << "<-cls shdr tag\n";
        // int i  = check_subtag(cls_shdr_tg);

        if (strcmp(opn_shdr_tg, cls_shdr_tg) != 0) {
            PRINT_EXIT("Close tag subtag name:" << cls_shdr_tg
                << " does not match with "
                << opn_shdr_tg << "\n");
        }

        if (mem_left < len - 1) {
            PRINT_EXIT("INSUFFICIENT SPACE \n");
        }

        fseek(file, cntn_strt, SEEK_SET);
        size_t sz = fread(write_ptr, sizeof(char), len, file);

        write_ptr[(sz) / sizeof(char)] = '\0';

        fseek(file, tg_end, SEEK_SET);

        std::cout << "[write]" << write_ptr << "[write]\n";
        write_ptr += len + 1;
        curnt_element->shadr_ptrs[type_indx] = write_ptr;
        return len;
    }

    FILE* open_file(const char* file_name) {
        FILE* file = fopen(file_name, "r");
        if (!file) {
            PRINT_EXIT("File not found.");
        }
        return file;
    }

    ShaderReader() {}

    ShaderReader(const char* file_name) {

        file = open_file(file_name);
        get_nxt(); //  we need to read first character of file before calling is
        //  whitespc
        // else the isspace(cursr = 0) == false and loop will not
        // continue and nxt_token_tag is get cursr as 0 and its not
        // '<' out error 

        for (int shader_indx = 0; shader_indx < n; shader_indx++) {
            skip_whitespc();
            if (cursr == EOF) {
                PRINT_EXIT(
                    "Not all shaders are present. recent shader read was "
                    << handel[shader_indx].name
                    << " -with shader index = " << shader_indx);
            }
            read_element();
            curnt_element++;
            curnt_indx++;
        }
    }


    ShaderHandel& operator[](const char* str) {
        int elmen_indx = 0;
        bool found = false;

        for (int i = 0; i < n; i++) {
            if (strcmp(str, handel[i].name) == 0) {
                found = true;
                elmen_indx = i;
                break;
            }
        }

        if (!found) {
            PRINT_EXIT("'" << str << "' is not a Element of ShaderHandel\n");
        }

        return handel[elmen_indx];
    }
};

char* ShaderHandel::operator[](const char* shdr) {
    int indx = check_subtag(shdr);
    if (active_shaders[indx] == 0) {
        PRINT_EXIT_NO_LINE(shdr << " is not a active shader of " << name);
    }
    return shadr_ptrs[indx];
}


int main() {

    ShaderReader<1000,2> reader("shaders.h");


    std::cout << "\n\n[program completed]";
}