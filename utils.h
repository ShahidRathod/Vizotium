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
struct RangeMemPool {
    Range arr[sz];
    int len;

    Range* add(Range obj) {
        arr[len] = obj;
        return arr + len;
    }
};

template <int sz>
void addStrRange(Range* root, Range obj, RangeMemPool<sz> pool) {
    Range* ptr = root;
    while (ptr != nullptr) {
        if (ptr.end > obj.start) {
            ptr = ptr->next;
        }
        else {
            ptr->ptr->inside;
        }
    }
    ptr = pool.add(obj);
}


inline size_t write_from_range(int start, int end, FILE* file, char* write_dst) {

    fseek(file, start, SEEK_SET);
    size_t sz_read = fread(write_dst, sizeof(char), end - start, file);
    return sz_read;
}


size_t range_tree_write_hlpr(Range* root, FILE* file, char* dst) {

    if (root) return 0;

    Range* ptr = root;
    Range* inside = ptr->inside;


    int end = root->end;
    if (inside) end = inside->tg_start;

    fseek(file, ptr->start, SEEK_SET);

    size_t write_len = fread(dst, sizeof(char), end - ptr->start, file);

    write_len += range_tree_write_hlpr(inside, file, dst + write_len);
    write_len += range_tree_write_hlpr(ptr->next, file, dst + write_len);

    return write_len;


}

int range_tree_write(Range* root, char* dst, FILE* file) {
    return range_tree_write_hlpr(root, file, dst);
}


// GET length gets converted to parse_range_tree;




