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
    char data[size] = {' '};

    constexpr ConstexprStr(const char* str) {
        for (int i = 0; i < size; i++) data[i] = ' ';

        int indx = 0;
        int n = 0;
        int j = 0;
        char c = 0;

        for (int i = 0; i < size && n + j < size; i++) {
            c = str[i];
            if (c == '\0') break;
            else if (c == ',') {
                data[n+j] = '\0';
                n = j / sz+1;
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


constexpr int max_subtg_name_len =13;  // "tess_control" this is the largest valid subtag name
constexpr int stage_count = 6;
ConstexprStr<max_subtg_name_len, stage_count> subtag_names{
    "vertex,fragment,tess_control,tess_eval,geometry,compute," };

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
    int active_shaders[stage_count] = {0};
    int shdr_line_no[stage_count * 2] = {-1};
    int start = -1;
    int end = -1;
    char* shadr_ptrs[stage_count];

};

#define PRINT_EXIT(str) \
std::cerr<<"LINE NO:("<<ln_no <<"):"<<char_no<<" "<< str;\
exit(EXIT_FAILURE);\

template <int b_sz, int n>
struct ShaderReader {

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

    ShaderHandel* curnt_element = handel;
    int curnt_indx = 0;
    FILE* file;

    char get_nxt() {
        int c = fgetc(file);
        cursr = (char)c;

        if (strcmp(delim_tkn + delim_len - 2, R"(\\)") == 0) {
            at_comnt = true;
        }
        if (cursr == '\n' ) {
            ln_no++;
            at_comnt = false;
            
        }

        std::memmove(delim_tkn, delim_tkn+1 , delim_len - 1);
        delim_tkn[delim_len-1] = cursr;
        delim_tkn[delim_len] = '\0';
        char_no_s++;
        return c;
    }

    inline bool is_nxt_char(const char chr) {
       
        if ( skip_whitespc()) {
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
            PRINT_EXIT(
                err_msg
                << "was expected before: "<<(int)cursr<<cursr<<"\n");
        }
        get_nxt();
    }


    void cpy_tag_name_at( char* name_dst) {
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
        while (cursr!='>') {  
     
            if (isspace(cursr)) {
                skip_whitespc();
                if (cursr=='>') break;
                else PRINT_EXIT("Shader name has whitespace in between ");
                
            }
            if (t_name_indx >= n_sz) {
                PRINT_EXIT("Shader name exceeds the name buffer size"
                    << n_sz << " name= " << temp_name << "\n");
            }

            if ( isalnum(cursr)==0 && cursr != '_')
            {
                PRINT_EXIT(
                    "Shader has very special character in "
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

    int check_subtag(char* sbtg_name) {
        bool found = false;

        int indx = stage_count - 1;
        while (indx >= 0) {
            if (strcmp(sbtg_name, subtag_names[indx]) == 0) {
                std::cout << sbtg_name << " " << subtag_names[indx]<<"\n";
                found = true;
                break;
            }
            indx--;
        }

        if (!found) {
            PRINT_EXIT(sbtg_name << " is not a valid subtag name.");
        }
       
       

        return indx;
    }

    void read_element() {
    
        char* element_name = curnt_element->name;
        is_nxt_token_tag("Element tag ");
        cpy_tag_name_at(element_name);
        
        curnt_element->start = ln_no;
        char name_buff[n_sz] = {};

        for (int i = 0;i < stage_count;i++) {
          
            is_nxt_token_tag("Element closing or Shader ");

            if (!skip_whitespc() && cursr == '/') {
                PRINT_EXIT("Exprected a closing tag Maybe you forgot '/'\n");
                get_nxt();
            }

            cpy_tag_name_at(name_buff);

            std::cout << "Element tag name: " << element_name
                << "  Shader name tag: " << name_buff << "\n";

            int type_indx = check_subtag(name_buff);
            

            curnt_element->shdr_line_no[2 * type_indx] = ln_no;
            std::cout <<"active shader: " << curnt_element->active_shaders[type_indx]++ << "\n";
            if ((curnt_element->active_shaders[type_indx]++) > 1) {
                PRINT_EXIT(
                    "Shader: " << name_buff << " Element: "
                    << curnt_element->name << " is already defined line at  ("
                    << curnt_element->shdr_line_no[2 * type_indx]
                    << "," << curnt_element->shdr_line_no[2 * type_indx + 1] << ")" << "\n");
            }
            
            std::cout << type_indx << "<-typeindx\n";

            if (is_cls) {
                if ((strcmp(name_buff, element_name)) != 0) {
                    PRINT_EXIT(
                        "Element(" << curnt_indx
                        << "): open tag-" << element_name
                        << " does not match close tag-" << name_buff);

                    curnt_element->end = ln_no;
                }
                break;
            }
            
            read_content( type_indx, name_buff);
           
        }
    }

    int read_content( int type_indx,char* opn_shdr_tg) {
        int len = char_no_s;

        std::cout << "--------content----------\n";
        bool cls_found = false;

        while ((cursr!='<' || at_comnt)  && cursr != EOF) {
           //len++;
          //  std::cout << delim_tkn << "\n";
            get_nxt();
        }
        
        len = char_no_s - len;
        int tag_len = char_no_s;


        get_nxt();
        
        if (!skip_whitespc() && cursr!='/') {
            PRINT_EXIT("Exprected a closing tag Maybe you forgot '/'\n");
        }

        get_nxt();
        
        
        //skip_whitespc();
        std::cout << cursr << "<-cursr after\n";
        std::cout << "--------content----------\n";
        
        std::cout << len << "<-content length\n";
        curnt_element->shdr_line_no[2 * type_indx + 1] = ln_no;
        char cls_shdr_tg[max_subtg_name_len];
        cpy_tag_name_at(cls_shdr_tg);

       
        tag_len = char_no_s - tag_len;
        std::cout << len+ tag_len << "<-seek len length "
            <<char_no_s<<"<-char no s " 
            << tag_len<< "<-tag length\n";

        std::cout << cls_shdr_tg << "<-cls shdr tag\n";
        // int i  = check_subtag(cls_shdr_tg);

        if (strcmp(opn_shdr_tg,cls_shdr_tg)!=0) {
            PRINT_EXIT("Close tag subtag name:"
                <<cls_shdr_tg << " does not match with "
                << opn_shdr_tg << "\n");
        }

        if (mem_left < len - 1) {
            PRINT_EXIT("INSUFFICIENT SPACE \n");
        }
        
        fseek(file, -(len+tag_len+2), SEEK_CUR);
        
        /* the len-2 is a adjustment
        since we dont want to write '</' in the shaders*/

        size_t sz = fread(write_ptr, sizeof(char),len, file);
        write_ptr[sz/sizeof(char)] = '\0';
        fseek(file, (tag_len+1), SEEK_CUR);
        std::cout << "[write]"<< write_ptr<< "[write]\n";

        write_ptr += len;
        curnt_element->shadr_ptrs[type_indx] = write_ptr;
        
        return len - 1; 
    }

    FILE* open_file(const char* file_name) {
        FILE* file = fopen(file_name,"r");
        if (!file) {
            PRINT_EXIT("File not found.");
        }
        return file;
    }
    
    ShaderReader() {}

    ShaderReader(const char* file_name) {
        
        file = open_file(file_name);
        get_nxt(); //  we need to read first character of file before calling is whitespc 
                   // else the isspace(cursr = 0) == false and loop will not continue 
                   // and nxt_token_tag is get cursr as 0 and its not '<' out error  

        
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
    
    char* operator [] (char * str) {
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
            ptr++;
        }

        int type_indx = check_subtag((ptr+1));
        *ptr = '\0';

        int elmen_indx = 0;
        bool found = false;

        for (int i = 0; i < n; i++){
            if (strcmp(str, handel[i].name)) {
                found = true;
                elmen_indx = i;
            }

        }
        
        ShaderHandel& shadr_elem = handel[elmen_indx];

        if (!found) {
            PRINT_EXIT("Element name"<<str<<"was not found\n");
        }
        if (!shadr_elem.active_shadrs[type_indx]) {
            PRINT_EXIT(str<<"is a active shader of"<<shadr_elem.name);
        }

    }
};

int main() {

    
   
    for (int i = 0; i < stage_count; i++) std::cout << subtag_names[i]<<strlen(subtag_names[i]) << "\n";
    ShaderReader<200, 2> sr("shaders.txt");
   std::cout << "code compiled";
   
}