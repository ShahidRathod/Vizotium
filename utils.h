#pragma once
#include <iostream>
#include <cstdlib>

constexpr int n_sz = 20;
#define RED "\x1b[31m"
#define RESET "\x1b[0m"



struct Tag {

    int    opn_tg_strt;
    int    start;
    int    end;
    int    cls_tg_end;
    char   name[n_sz];

    Tag* next = nullptr;
    Tag* inside = nullptr;
};


template <int sz, int max_copy_depth>
struct TagTree {
    Tag  arr[sz];
    Tag  root_obj;
    Tag* root = arr;
    Tag* curnt = root;
    Stack<max_copy_depth, Tag> rng_stack;


    Tag arr[sz];
    int stk_len = 0;

    int len = 1;

    void addRange(Tag obj) {
        if (len >= sz) {
            std::cerr << "Max camnd tag limit reached";
            exit(EXIT_FAILURE);
        }

        Tag* ptr = top;

        while (ptr != nullptr) {
            if (ptr->end < obj.start) {
                ptr = ptr->next;
            }
            else {
                ptr = ptr->inside;
            }
        }

        arr[len] = obj;
        ptr = arr + len;
        len++;
    }

    void pop() {
        stk_len--;
    }

    void addStack(Tag obj) {
        if (len + 1 > sz) {
            std::cerr << RED "Max stack length reached\n" RESET;
        }
        else {
            arr[stk_len + 1] = obj;
            len++;
        }
        return arr + stk_len;
    }
};


struct TagWriter {

    FILE* file;

    char* dst;
    char* pen;

    TagWriter(FILE* fl, char* d) {
        file = fl;
        pen = dst = d;
    }

    inline size_t write_by_range(int start, int end) {

        fseek(file, start, SEEK_SET);
        size_t sz_read = fread(pen, sizeof(char), end - start, file);
        pen += sz_read;
        return sz_read;
    }

    void tag_tree_write(Tag* root) {

        int st = root->start;
        int end;
        Tag* inside = root->inside;

        while (inside != nullptr) {

            end = inside->opn_tg_strt;
            write_by_range(st, end);
            tag_tree_write(inside);
            st = inside->cls_tg_end;
            inside = inside->next;
        }

        write_by_range(st, root->end);

    }
};


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

constexpr int cammnds = 2;
ConstexprStr<max_subtg_name_len, cammnds> cammnd_tags{ "copy,paste," };



template <int sz> struct CircularBuff {
    int len = 0;
    char arr[sz + 1] = { ' ' };

    CircularBuff() { arr[sz] = '\0'; }
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

