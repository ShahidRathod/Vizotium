#pragma once


constexpr int n_sz = 20;

struct Range {
    int tg_start;
    int  start;
    int  end;
    char name[n_sz];
    Range* next = nullptr;
    Range* inside = nullptr;


};

template <int sz>
struct RangeTree {

    Range arr[sz];
    Range root_obj;
    Range* root = arr;
    Range* curnt = root;
    int len = 1;

    void addRange(Range obj) {
        if (len >= sz) {
            std::cerr << "Max camnd tag limit reached";
            exit(EXIT_FAILURE);

        }

        Range* ptr = root;
        while (ptr != nullptr) {
            if (ptr.end > obj.start) {
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

    void addInside(Range obj) {

    }

};

struct RangeWriter {

    FILE* file;

    char* dst;
    char* pen;

    RangeWriter(FILE* fl, char* d) {
        file = fl;
        pen = dst = d;
    }

    inline size_t write_from_range(int start, int end) {

        fseek(file, start, SEEK_SET);
        size_t sz_read = fread(pen, sizeof(char), end - start, file);
        pen += sz_read;
    }


    size_t range_tree_write_hlpr(Range* root) {

        if (root) return 0;

        Range* ptr = root;
        Range* inside = ptr->inside;


        int end = root->end;
        if (inside) end = inside->tg_start;
        size_t write_len = 0;

        write_from_range(ptr->start, end);

        write_len += range_tree_write_hlpr(inside);
        write_len += range_tree_write_hlpr(ptr->next);


        return write_len;

    }

    int range_tree_write(Range* root) {
        return range_tree_write_hlpr(root);
    }

};
// GET length gets converted to parse_range_tree;



