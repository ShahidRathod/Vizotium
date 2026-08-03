#pragma once


constexpr int n_sz = 20;

struct Range {
    int tg_start;
    int tg_end;
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


    void range_tree_write(Range* root) {

        int st = root->start;
        int end;
        Range* inside = root->inside;

        while (inside != nullptr) {

            end = inside->tg_start;
            write_from_range(st, end);
            range_tree_write(inside);
            st = inside->tg_end;
            inside = inside->next;
        }

        write_from_range(st, root->end);

    }


};
