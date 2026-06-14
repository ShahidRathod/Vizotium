#pragma once

//#include "FunctionArray.h"

#include <cmath>
#include <cstddef>
#include <iostream>

#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Color {
    float arr[4];
};

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

inline std::ostream& operator<<(std::ostream& os, const Vertex& v) {
    os << "(" << v.X << ", " << v.Y << ")";
    return os;
}

struct Ebo_tringl {
    int v1, v2, v3;
};

struct Ebo_sqre {
    Ebo_tringl t1, t2;
};

enum class Clr : int {
    R, G, B, A
};



struct Camera {

    float yaw = 0;
    float pitch = 0;
    float scle = 1;

    float fov = 45.0;
    float aspect;


    glm::vec3 pos = glm::vec3(0, 0, 0);

    void shift_x_by(float v) { pos.x += v; }
    void shift_y_by(float v) { pos.y += v; }
    void shift_z_by(float v) { pos.z += v; }


    void rotate_x(float x) { yaw += x; }
    void rotate_y(float y) { pitch += y; }

    void scale(float s) { scle += s; }

    glm::mat4 model() {
        glm::mat4 model(1.0f);

        model = glm::translate(model, pos);

        glm::vec3 x_axis(0, 0, 1);
        glm::vec3 y_axis(0, 1, 0);
        model = glm::rotate(model, glm::radians(yaw), x_axis);
        model = glm::rotate(model, glm::radians(pitch), y_axis);
        model = glm::scale(model, glm::vec3(scle));

        return model;
    }

    glm::mat4 perspective() {
        return glm::perspective(fov, aspect, 0.01f, 10.0f);
    }

    glm::mat4 view() {
        glm::mat4 view(1.0f);
        view = glm::lookAt(pos, glm::vec3(0,0,0), glm::vec3(0, 0, 1));
        return view;
    }
    glm::mat4 MVP_upload() {
        return (perspective() * (view() * model()));
    }

};



template <int x_sz, int y_sz>
class Surface {
    constexpr static int size = x_sz * y_sz;
    constexpr static int ebo_sz = (x_sz - 1) * (y_sz - 1) * 6;

    Ebo_sqre ebo_arr[ebo_sz];
   


public:
    Vertex arr[size];
    Surface(const float c_x, const float c_y, const float x, const float y) {
        float x_st = c_x - x;
        float y_st = c_y - y;
        float y_inc = 2 * y / (y_sz - 1);
        float x_inc = 2 * x / (x_sz - 1);

        for (int i = 0; i < y_sz - 1; i++) {
            for (int j = 0; j < x_sz - 1; j++) {
                int indx = j + i * x_sz;
                Ebo_sqre& sqre = ebo_arr[indx];

                sqre = Ebo_sqre {
                    {indx, indx + 1, indx + x_sz},
                    {indx + 1, indx + x_sz, indx + 1 + x_sz}
                };
            }
        }

        for (int i = 0; i < y_sz; i++) {
            for (int j = 0; j < x_sz; j++) {
                int indx = j + i * x_sz;
                Vertex& coord_xy = arr[indx];
                coord_xy.X = x_st + x_inc * j;
                coord_xy.Y = y_st + y_inc * i;
                coord_xy.Z = 0;
            }
        }
    }

    void print() {
        for (int i = 0; i < y_sz; i++) {
            for (int j = 0; j < x_sz; j++) {
                std::cout << arr[j + i * x_sz] << "  ";
            }
            std::cout << "\n\n";
        }
    }

    /*template <Axis a, Axis b, Axis res>
    void apply_func(float (*fn)(float, float), float* fun ,int n) {
        constexpr int axis1 = static_cast<int>(a) ;
        constexpr int axis2 = static_cast<int>(b) ;
        constexpr int res_axis = static_cast<int>(res) ;

        float* point = reinterpret_cast<float*>(arr);

        for (int param_i = 0; param_i < n; param_i++) {
            float x = func_param[param_i];
            float y = func_param[param_i];
            (void)x;
            (void)y;

            for (int i = 0; i < x_sz; i++) {
                for (int j = 0; j < y_sz; j++) {
                    point[res_axis] = fn(point[axis1], point[axis2]);
                    point += 3;
                }
            }
        }
    }

    template <Axis a, Axis b, Axis res, int N>
    void update(float (*funcs[N])(float, float), float* func_param) {
        constexpr int axis1 = static_cast<int>(a) ;
        constexpr int axis2 = static_cast<int>(b) ;
        constexpr int res_axis = static_cast<int>(res);

        float* point = reinterpret_cast<float*>(arr);
        (void)func_param;

        for (int fn_i = 0; fn_i < N; fn_i++) {
            for (int i = 0; i < x_sz; i++) {
                for (int j = 0; j < y_sz; j++) {
                    point[res_axis] = (funcs[fn_i])(point[axis1], point[axis2]);
                    point += 3;
                }
            }
        }
    }
    

    
    void update_z(float (*)) {
        float (*funcs[1])(float, float) = {fn};
        update<Axis::X, Axis::Y, Axis::Z, 1>(funcs, nullptr);
    }
    */


    float* gl_arr() { 
        return &(arr[0].X); 
    
    }
    int gl_vbo_sz() { return size * sizeof(Vertex); }

    int* gl_ebo_arr() { return &(ebo_arr[0].t1.v1); }
    int gl_ebo_sz() { return ebo_sz * sizeof(Ebo_sqre); }
};

inline float fn(float a, float b) {
    return std::sin(a * b);
}

template <typename T>
void print_vec(T vec) {
    for (int i = 0; i < T::length(); i++) {
        std::cout << vec[i] << "  ";
    }
    std::cout << "\n";
}

template <typename T>
void print_mat(T mat) {
    for (int i = 0; i < T::length(); i++) {
        for (int j = 0; j < T::col_type::length(); j++) {
            std::cout << mat[i][j] << "  ";
        }
        std::cout << "\n";
    }
}
