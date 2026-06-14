#pragma once

#include <cstddef>
#include <cstring>
#include <iostream>

enum class Axis : int { 
    X = 0,
    Y = 1,
    Z = 2
};

struct Vertex {
    float X, Y, Z;

    float* operator[](int i) {
        float* val = ((float*)(this) + i);
        return val;
    }
};

enum class Mode : int { Left, Right };

struct Parameter {
    float x, y, t, a;
};

template <typename T>
void memshift(T*& ptr1, T*& ptr2, int len) {
    if (!ptr1 || !ptr2 || ptr2 <= ptr1) return;

    std::memmove(
        ptr1,
        ptr1 + len,
        static_cast<std::size_t>(ptr2 - ptr1) * sizeof(T)
    );

    ptr1 += len;
    ptr2 += len;
}

template <Mode m, typename T>
void mode_memshft(T*& ptr1, T*& ptr2, int len) {
    if constexpr (m == Mode::Left) {
        if (ptr1 && ptr2 && ptr2 > ptr1) {
            std::memmove(
                ptr1,
                ptr1 + len,
                static_cast<std::size_t>(ptr2 - ptr1) * sizeof(T)
            );
        }
    } else {
        if (ptr1 && ptr2 && ptr1 > ptr2) {
            std::memmove(
                ptr2,
                ptr2 + len,
                static_cast<std::size_t>(ptr1 - ptr2) * sizeof(T)
            );
        }
    }

    ptr1 += len;
    ptr2 += len;
}

template <typename T>
void write_inc(T*& ptr, T& val, int i) {
    *ptr = val;
    ptr += i;
}

enum Out : int { x, y, z };
inline int flt_sz = sizeof(float);

template <int n>
struct ParamHandel {
    Parameter *start, *z_ptr, *left, *right, *x_ptr, *end;

    ParamHandel() {
        start = z_ptr = left = right = x_ptr = end = nullptr;
    }

    inline int z_sz() { return static_cast<int>(z_ptr - start); }
    inline int y_sz() { return static_cast<int>(right - left - 1); }
    inline int x_sz() { return static_cast<int>(end - x_ptr); }
    inline int left_spc() { return static_cast<int>(left - z_ptr); }
    inline int right_spc() { return static_cast<int>(x_ptr - right); }

    ParamHandel(Parameter* st) {
        z_ptr = start = st;
        x_ptr = end = st + 3 * n;
        right = st + (3 * n) / 2 + 1;
        left = right - 1;
    }

    void add_param_z(Parameter param) {
        if (z_ptr == left) memshift(left, right, 1);
        write_inc(z_ptr, param, 1);
    }

    void rmv_param_z(int i) {
        Parameter* src = start + i;
        memshift(src, z_ptr, -1);
    }

    void add_param_y(Parameter param) {
        bool ysz_even = y_sz() % 2 == 0;
        Parameter*& y_ptr =
            ysz_even
                ? ((left != z_ptr) ? left : right)
                : ((right != x_ptr) ? right : left);

        write_inc(y_ptr, param, (y_ptr == right) ? 1 : -1);
    }

    void rmv_param_y(int i) {
        if (i < 0) return;

        bool ysz_even = y_sz() % 2 == 0;
        Parameter*& ref = ysz_even ? left : right;

        int half = y_sz() / 2;
        int delta = (i % 2 == 0) ? -(i / 2) : (i / 2);
        Parameter* src = left + half + delta;

        memshift(ref, src, ysz_even ? 1 : -1);
    }

    void add_param_x(Parameter param) {
        if (right == x_ptr) memshift(left, right, -1);
        write_inc(x_ptr, param, -1);
    }

    void rmv_param_x(int i) {
        Parameter* src = x_ptr + i;
        memshift(x_ptr, src, 1);
    }

    inline int free_space() {
        return static_cast<int>(left - z_ptr - 1 + x_ptr - right - 1);
    }

    int param_len_at(int i) {
        switch (i) {
        case 0: return z_sz();
        case 1: return y_sz();
        case 2: return x_sz();
        default : 
            std::cerr<<"Function range out of bound [param_len_at]\n";
            exit(EXIT_FAILURE);
        }
    }

    Parameter* param_strt(int i) {
        switch (i) {
        case 0: return start;
        case 1: return left;
        case 2: return x_ptr;
        default:
            std::cerr << "Function range out of bound [param_len_at]\n";
            exit(EXIT_FAILURE);
        }
    }
};

template <typename T, int n, Mode md>
struct SpaceState {
    T** ref = nullptr;
    int* skip_buff = nullptr;
    int s = 0;
    int min_spc = 0;
    int need = 0;

    SpaceState(int* buff, int m, int nd) {
        skip_buff = buff;
        min_spc = m;
        need = nd;
    }

    void move_hlpr(T*& ptr1, T*& ptr2, T*& ptrR, int len, int spc, int inc) {
        (void)ptrR;
        (void)spc;
        (void)inc;

        if (!ptr1 || !ptr2 || ptr2 <= ptr1) return;
        if (len == 0) return;

        std::memmove(
            ptr1,
            ptr1 + len,
            static_cast<std::size_t>(ptr2 - ptr1) * sizeof(T)
        );

        ptr1 += len;
        ptr2 += len;
    }

    void move(T*& ptr1, T*& ptr2, int spc) {
        if constexpr (md == Mode::Left) {
            move_hlpr(ptr1, ptr2, ptr1, 1, spc, 1);
        } else {
            move_hlpr(ptr2, ptr1, ptr2, -1, spc, -1);
        }
    }
};

template <typename T, Mode md>
struct ShiftPack {
    T *mid_strt, *mid_end, *side_strt, *bound1, *bound2, *side_end;
    using ShftPackT = ShiftPack<T, md>;

    static auto ptr_by_handel(Parameter*) {
        return static_cast<ShftPackT*>(nullptr);
    }

    int midspc() { return static_cast<int>(mid_end - mid_strt); }
    int sidespc() { return static_cast<int>(side_end - side_strt); }
    auto nxt_pack() { return this + 1; }
};

template <typename T>
struct ShiftPack<T, Mode::Right> {
    T *side_strt, *bound2, *bound1, *side_end, *mid_end, *mid_strt;
    using ShftPackT = ShiftPack<T, Mode::Right>;

    static auto ptr_by_handel(Parameter*) {
        return static_cast<ShftPackT*>(nullptr);
    }

    int midspc() { return static_cast<int>(mid_end - mid_strt); }
    int sidespc() { return static_cast<int>(side_end - side_strt); }
    auto nxt_pack() { return this - 1; }
};

template <int N, int n>
struct FuncArray {
    using funcT = float (*)(float, float, float);
    using ParamHandel_N = ParamHandel<n>;
    int sz = 0;
    funcT func_arr[N];
    ParamHandel_N handel[N];
    Parameter param_mem[N * n * 3];

    FuncArray() {
        handel[0] = ParamHandel_N(param_mem);
    }

    void appendFunc(funcT fn) {
        if (sz >= N) return;

        func_arr[sz] = fn;

        if (sz == 0) {
            handel[0] = ParamHandel_N(param_mem);
        } else {
            handel[sz] = ParamHandel_N(handel[sz - 1].end);
        }

        sz++;
    }

    template <Axis ax>
    void rmv_param(int f_indx, int param_indx) {
        ParamHandel_N& rmv_param = handel[f_indx];

        if constexpr (ax == Axis::X)
            rmv_param.rmv_param_x(param_indx);
        else if constexpr (ax == Axis::Y)
            rmv_param.rmv_param_y(param_indx);
        else
            rmv_param.rmv_param_z(param_indx);
    }

    template <Axis ax>
    void append_param(int i, Parameter p = {0, 0, 1, 1}) {
        ParamHandel_N& apnd = handel[i];

        if constexpr (ax == Axis::X)
            apnd.add_param_x(p);
        else if constexpr (ax == Axis::Y)
            apnd.add_param_y(p);
        else
            apnd.add_param_z(p);
    }

    template <Mode md>
    int mkspc_hlpr(int at, int need) {
        if (at < 0 || at >= sz || need <= 0) return 0;

        int collected = 0;
        constexpr int inc = (md == Mode::Left) ? -1 : 1;

        for (int idx = at; idx >= 0 && idx < sz; idx += inc) {
            ParamHandel_N& p = handel[idx];
            collected += p.left_spc() + p.right_spc();
            if (collected >= need) break;
        }

        return collected;
    }

    int mkspc_right(int at) {
        return mkspc_hlpr<Mode::Right>(at, 1 + n / 2);
    }

    int mkspc_left(int at) {
        return mkspc_hlpr<Mode::Left>(at, 1 + n / 2);
    }

    void mkspace(int at) {
        if (at < 0 || at >= sz) return;

        int left = mkspc_left(at);
        int right = mkspc_right(at);
        int space = left + right;
        auto mid_shft = handel[at].left - (left + right) / 2;

        (void)space;
        (void)mid_shft;
    }

    template <typename T>
    void print_hlpr(T* f1, T* f2, int s) {
        for (T* i = f1; i <= f2; ++i) {
            std::cout << "[" << i << "]";
        }
        for (int i = 0; i < s; i++) std::cout << "-";
    }

    template <typename T, std::size_t Count>
    void print_vec(const T (&vec)[Count]) {
        for (std::size_t i = 0; i < Count; i++) {
            std::cout << vec[i] << "  ";
        }
        std::cout << "\n";
    }

    template <typename T>
    void print_vec(const T& vec) {
        for (std::size_t i = 0; i < static_cast<std::size_t>(T::length()); i++) {
            std::cout << vec[i] << "  ";
        }
        std::cout << "\n";
    }

    void print() {
        for (int i = 0; i < N; i++) {
            auto hndl = handel[i];
            std::cout << "|";
            print_hlpr(hndl.start, hndl.z_ptr, hndl.left_spc());
            print_hlpr(hndl.left, hndl.right, hndl.right_spc());
            print_hlpr(hndl.x_ptr, hndl.end, 0);
            std::cout << "|";
        }
    }
    
    float give_func(int i ,float x,float y, float z) {
        funcT function = func_arr[i];
        ParamHandel ihandl = handel[i];

        for (int i = 0; i < 3;i++) {
            int len = ihandl.param_len_at(i);
            Parameter* params = ihandel.paratc
            for (int j = 0; j < len;j++) {
                *(ver[i]) += function(x,y,z);
            }
        }
    }
    
    funcT operator[] (int i) {

    }
};



