#define _CRT_SECURE_NO_WARNINGS

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "utils.h"

enum class Relation { ordered, notcoexist, norelation, singular };

enum class Inverse { False, True };

struct Rule {
    char c[3];
    int c_i[2];
    Relation relation = Relation::norelation;
    Inverse inverse = Inverse::False;
    Rule(const char* chrs, Relation rel, Inverse inv) {
        c[0] = chrs[0];
        c[1] = chrs[1];
        relation = rel;
        inv = inverse;
    }

    Rule(const char chr, Relation rel, Inverse inv) {
        c[0] = c[1] = chr;
        relation = rel;
        inv = inverse;
    }

    inline constexpr char a() { return c[0]; }
    inline constexpr char b() { return c[1]; }
};

enum class Dispatch : int { not_tag, open, close, paste, count };

constexpr int str_len(const char* str) {
    int i = 0;
    while (str[i] != '\0') i++;
    return i;
}



template <int sz>
struct SyntaxData {
    int pos[sz] = { 0 };
    int count[sz] = { 0 };
    const char* chars = nullptr;
    int char_count = 0;
    SyntaxData() {}
    constexpr SyntaxData(const char(&c)[sz]) { chars = c; }

    constexpr int charindex(const char c) const {
        for (int i = 0; i < sz && chars[i] != '\0'; i++) {
            if (chars[i] == c) return i;
        }
        return -1;
    }

    void reset() {
        char_count = 0;
        memset(pos, 0, sizeof(pos));
        memset(count, 0, sizeof(count));
    }

    bool eval_char(char c) {
        int i = charindex(c);
        if (i != -1) {
            if (!count[i]) pos[i] = char_count;
            count[i]++;
            return true;
        }
        return false;
    }

    int offset(char c) const {
        int i = charindex(c);
        if (i != -1) {
            return (count[i] != 0) * pos[i];
        }
        return 0;
    }

    inline int count_of(const char c) { return count[charindex(c)]; }

    bool check_relation(Rule spec) {
        int i_a = charindex(spec.a());
        int i_b = charindex(spec.b());
        if (!(count[i_a] && count[i_b])) return true;
        bool res = true;
        switch (spec.relation) {
        case Relation::ordered:
            res = (pos[i_a] < pos[i_b]);
            break;
        case Relation::notcoexist:
            res = !(count[i_a] && count[i_b]);
            break;
        case Relation::singular:
            res = count[i_a] == 1;
            break;
        }
        return static_cast<int>(spec.inverse) ? !res : res;
    }

    template <int r_sz>
    bool check_rules(Rule(&rules)[r_sz]) {
        for (int i = 0; i < r_sz;i++) {
            if (!check_relation(rules[i])) return false;
        }
        return true;
    }
};

SyntaxData tag_data("</$:_>");
Rule tag_rule[] = {
    {"/$",  Relation::notcoexist,Inverse::False},
    {"/:",  Relation::notcoexist,Inverse::False},
    {'<',   Relation::singular,  Inverse::False},
    {'/',   Relation::singular,  Inverse::False},
    {'$',   Relation::singular,  Inverse::False},
    {"$:",  Relation::ordered,   Inverse::False},
    { "$_",  Relation::ordered,  Inverse::False},
    { ":_",  Relation::ordered,  Inverse::False},
    { "/_",  Relation::ordered,  Inverse::False}
};



template <int b_sz, int n_sz>
struct ShaderReader {
    bool at_comnt = false;
    bool in_tag = false;
    char cursr = 0;
    static constexpr int delim_len = 2 * n_sz;

    char buffer[b_sz] = {};
    char* write_ptr = buffer;

    int ln_no = 1;
    int char_no = 0;

    int mem_left = b_sz;
    int reader_sz = 0;
    bool skip_newln = true;

    int curnt_indx = 0;
    FILE* file;
    long file_sz = 0;
    int depth = 0;
    TagTree<50, 10> tgtree;
    CircularBuff<delim_len> delim_tkn;
    int active_shaders[stage_count] = { 0 };
    Tag* currnt_tag = tgtree.top();

    GLuint program;
    ShaderReader() {}

    void inscope(TagPos new_tag_pos) {
        Tag* old_scope = tgtree.top();
        Tag* new_tag = tgtree.make_tag();
        new_tag->open = new_tag_pos;
        old_scope->addInside(new_tag);
        tgtree.addStack(new_tag);
        currnt_tag = tgtree.top();
        bool v = currnt_tag == new_tag;
        depth++;
    }

    void outscope(TagPos old_tag_pos) {
        currnt_tag->close = old_tag_pos;
        currnt_tag->commit_name();
        tgtree.pop();
        currnt_tag = tgtree.top();
        depth--;
    }

    hashT currnt_tag_hash() { return currnt_tag->tag_hash; }

    char get_nxt() {
        int c = buffer[char_no];
        cursr = (char)c;
        if (delim_tkn.compare_str(R"(\\)", 2)) at_comnt = true;
        if (cursr == '\n') {
            ln_no++;
            at_comnt = false;
        }
        if (c != ' ') delim_tkn.put_char(c);
        char_no++;
        return c;
    }

    bool skip_whitespc() {
        bool hit_newln = false;
        while (isspace(cursr)) {
            if (cursr == '\n') hit_newln = true;
            get_nxt();
        }
        return hit_newln;
    }

    char cpy_tag_str(char* dst, bool expect_spc, const char* end_delim) {
        if (skip_whitespc()) {
            RAISE_NO_NEWLINE_INSIDE_TAGS;
        }

        if (!isalpha(cursr)) {
            RAISE_SHADER_NAME_INVALID_START(cursr);
        }

        int t_name_indx = 0;
        char temp_name[n_sz] = {};

        while (!contains(cursr, end_delim)) {
            if (isspace(cursr)) {
                if (!expect_spc) break;
                skip_whitespc();
                if (contains(cursr, end_delim))
                    break;
                else {
                    RAISE_SHADER_NAME_HAS_WHITESPACE;
                }
            }
            if (t_name_indx >= n_sz) {
                RAISE_SHADER_NAME_TOO_LONG(n_sz, dst);
            }
            if (isalnum(cursr) == 0 && cursr != '_') {
                RAISE_SHADER_NAME_INVALID_CHAR(cursr);
            }
            temp_name[t_name_indx++] = cursr;
            get_nxt();
        }
        char end_char = cursr;
        get_nxt();
        strcpy(dst, temp_name);
        dst[t_name_indx + 1] = '\n';
        return end_char;
    }

    void parse_tag_cls() {
        cpy_tag_str(currnt_tag->buffer, false, ">");
        currnt_tag->check_end_same(char_no, ln_no);
    }

    void parse_tag_open() {
        cpy_tag_str(currnt_tag->buffer, false, ">");
        currnt_tag->commit_hash();
        currnt_tag->commit_name();
        char* first_str = currnt_tag->tag_name;
    }

    void parse_paste() {
        currnt_tag->type = TagType::Paste;
        while (true) {
            char end = cpy_tag_str(currnt_tag->buffer, false, ":>");
            currnt_tag->append_hash_lnk();
            if (end == '>') break;
        }

        HashLinkT hash_link = currnt_tag->hash_lst;
        Tag* tg_found = tgtree.find(hash_link);
        currnt_tag->addInside(tg_found);
        bool paste_in_itself = tgtree.find_in_branch(tg_found->inside, (hash_link.end), true);
        if (paste_in_itself) {
            RAISE_RECURSIVE_PASTING;
        }
    }

    Dispatch check_tag_syntax(TagPos& tag_pos) {
        tag_data.reset();
        int temp_char_no = char_no;
        bool temp_in_comnt = at_comnt;
        int temp_ln_no = ln_no;

        while (true) {
            bool is_special = !isalnum(cursr) && cursr != ' ';
            if (is_special && !tag_data.eval_char(cursr)) return Dispatch::not_tag;
            tag_data.char_count++;
            if (cursr == '>') break;
            get_nxt();
        }

        bool is_tag = tag_data.check_rules(tag_rule);
        bool is_paste = tag_data.count_of('$');
        bool is_cls = tag_data.count_of('/');

        Dispatch dis_type = static_cast<Dispatch> (is_tag + 1 * is_cls + 2 * is_paste);

        if (dis_type != Dispatch::not_tag) {
            tag_pos.start = temp_char_no - 1;
            tag_pos.end = char_no;
            tag_pos.ln_no = temp_ln_no;

            char_no = temp_char_no + tag_data.offset('/') + tag_data.offset('$');
            get_nxt();
            cursr;
            at_comnt = temp_in_comnt;
            ln_no = temp_ln_no;
        }
        return dis_type;
    }

    void loop() {
        while ((cursr != '<' && !at_comnt) && cursr != '\0') get_nxt();
    }

    void content_loop() {
        do {
            loop();
            TagPos tag_pos;
            Dispatch type = check_tag_syntax(tag_pos);
            switch (type) {
            case (Dispatch::open):
                inscope(tag_pos);
                parse_tag_open();
                break;
            case Dispatch::close:
                parse_tag_cls();
                outscope(tag_pos);
                break;
            case Dispatch::paste:
                inscope(tag_pos);
                parse_paste();
                outscope(tag_pos);
                break;
            }
        } while ((cursr != '\0'));
    }

    FILE* open_file(const char* file_name) {
        FILE* f = fopen(file_name, "r");
        if (!f) RAISE_FILE_NOT_FOUND;
        return f;
    }

    ShaderReader(const char* file_name, GLint prog) {
        program = prog;
        file = open_file(file_name);
        fseek(file, 0, SEEK_END);
        file_sz = ftell(file) - 1;
        fseek(file, 0, SEEK_SET);
        if (file_sz >= b_sz) RAISE_INSUFFICIENT_SPACE;

        fread(buffer, sizeof(char), file_sz, file);
        fclose(file);
        strcpy(currnt_tag->tag_name, file_name);
        currnt_tag->append_hash_lnk();

        get_nxt();
        content_loop();
    }
    
    void compile_shader_for(const char* enitity, GLenum type) {
        
        int index = GLshader_to_index(type);
     
        HashNode enitiyNode, shaderNode;
        enitiyNode.val = hash(enitity);
        shaderNode.val = all_names.hashes[index];
        enitiyNode.next = &shaderNode;
        shaderNode.next = nullptr;

        Tag* found = tgtree.find_in_branch(tgtree.root, &enitiyNode);
        TagWriter writer(buffer, found);
        GLuint compiled_shader = compile_shader(type,writer.dst);

        glAttachShader(program,compiled_shader);

        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);

        if (!success) {
            char log[1024];
            glGetProgramInfoLog(program, 1024, nullptr, log);
            std::cerr << log << '\n';
        }
        glDeleteShader(compiled_shader);
    }
};

