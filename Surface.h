#pragma once

//#include "FunctionArray.h"

#include <cmath>
#include <cstddef>
#include <iostream>

#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

bool mat_debug = true;
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

inline std::ostream& operator<<(std::ostream& os, const Ebo_tringl& v) {
    os << "(" << v.v1 << ", " << v.v2 << ", " << v.v3 << ")";
    return os;
}

struct Ebo_sqre {
    Ebo_tringl t1, t2;

};
inline std::ostream& operator<<(std::ostream& os, const Ebo_sqre& v) {
    os << v.t1 << v.t2;
    return os;
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
    std::cout << "|";
    for (int i = 0; i < T::length(); i++) {
        for (int j = 0; j < T::col_type::length(); j++) {
            std::cout << mat[i][j] << "  ";
        }
        std::cout << "|\n";
    }
}

enum class Clr : int {
    R, G, B, A
};

#define DEBUG_MATRIX(name) if(mat_debug){ std::cout <<"\n\n" << #name << ":\n"; print_mat(name); }



#define DEBUG(name) if(mat_debug){ std::cout <<"\n\n" << #name << ":\n"; std::cout<<name; }

#define X_AXIS_VEC3 glm::vec3 (1,0,0)
#define Y_AXIS_VEC3 glm::vec3 (0,1,0)
#define Z_AXIS_VEC3 glm::vec3 (0,0,1)

#define X_AXIS_VEC4 glm::vec4 (1,0,0,0)
#define Y_AXIS_VEC4 glm::vec4 (0,1,0,0)
#define Z_AXIS_VEC4 glm::vec4 (0,0,1,0)
#define RAD(x) glm::radians(x)

constexpr double PI= 3.14159265358979323846;
template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}
struct Camera {

    float yaw = 0.1f;
    float pitch = 0.1f;
    float scale = 0;

    float fov = 55.0f;
    float aspect = 1280.0f / 720.0f;

    glm::mat4 mvp = glm::mat4(1.0f);

    Camera() {
        yaw = 0.1;
        pitch = 0.1;
        scale = 1;
    }
   

    inline void limit_angle(float& angle,float lower,float upper) {

        if (angle >= upper) {
            angle = upper-1;
        }
        if (angle <= lower) {
            angle = lower+1;
        }

    }
    void inc_yaw(float x){
        yaw += x;

       // limit_angle(yaw,0,360);
      
    }
    void inc_pitch(float y) {
        pitch += y;
       limit_angle(pitch,0,180);
    }

    void scale_inc(float s) { scale += s; }
    inline float give_scale() { return std::exp(scale); }

    glm::mat4 model() {

        glm::mat4 model(1.0f);

      
        model = glm::rotate(model , RAD(yaw),Z_AXIS_VEC3);
        model = glm::rotate(model, RAD(pitch), X_AXIS_VEC3);
        model = glm::scale(model,glm::vec3(give_scale()));

        DEBUG_MATRIX(model);
        return model;
    }

    glm::mat4 perspective() {
        glm::mat4 pers = glm::perspective(RAD(fov), aspect, 0.01f, 25.0f);
        DEBUG_MATRIX(pers);
        return pers;
    }

    glm::mat4 view() {
        glm::mat4 view;

        float cy = cos(RAD(yaw));
        float sy = sin(RAD(yaw));
        float cp = cos(RAD(pitch));
        float sp = sin(RAD(pitch));
        
        float r = give_scale();

        glm::vec3 eye(
            r * cp * cy,
            r * sp,
            r * cp * sy
        );
        int sign = sgn(cp);

        std::cout<<"\nsign: "<<sign;
        view = glm::lookAt(glm::vec3(eye),glm::vec3(0,0,0),glm::vec3(0,sign*1,0));
        DEBUG_MATRIX(view);
        return view;
    }

    glm::mat4 update_MVP() {
        

        glm::mat4 mvp = (perspective() * (view()));
        DEBUG_MATRIX(mvp);
        return mvp;
    }

};




template <int x_sz, int y_sz>
class Surface {
public:

    constexpr static int size = x_sz * y_sz;
    constexpr static int ebo_sqre_sz = (x_sz - 1) * (y_sz - 1);
    constexpr static int ebo_sz = ebo_sqre_sz * 6;

    Ebo_sqre ebo_arr[ebo_sqre_sz];
    Vertex arr[size];


    Surface(const float c_x, const float c_y, const float x, const float y) {
        float x_st = c_x - x;
        float y_st = c_y - y;
        float y_inc = 2 * y / (y_sz - 1);
        float x_inc = 2 * x / (x_sz - 1);

        /// Eb array insitialization 
        int ebo_stride = x_sz - 1;


        for (int i = 0; i < y_sz - 1; i++) {
            for (int j = 0; j < ebo_stride; j++) {
                int indx = j + i * ebo_stride;

                Ebo_sqre& sqre = ebo_arr[indx];

                /// ebo array is mappings to arr which has stride x_sz
                int ebo_indx = j + i * x_sz;
                sqre = Ebo_sqre{
                    {ebo_indx, ebo_indx + 1, ebo_indx + x_sz},
                    {ebo_indx + 1, ebo_indx + x_sz, ebo_indx + 1 + x_sz}
                };

                //if (mat_debug) { std::cout << sqre << "\n"; }
            }
        }

        for (int i = 0; i < y_sz; i++) {
            for (int j = 0; j < x_sz; j++) {
                int indx = j + i * x_sz;
                Vertex& coord_xy = arr[indx];
                coord_xy.X = x_st + x_inc * j;
                coord_xy.Y = 0;                  
                coord_xy.Z = y_st + y_inc * i;
            }
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


    float* gl_arr() {return &(arr[0].X);}
    int gl_vbo_sz() { return size * sizeof(Vertex); }
    int* gl_ebo_arr() { return &(ebo_arr[0].t1.v1); }
    int gl_ebo_sz() { return ebo_sqre_sz * sizeof(Ebo_sqre); }
    int gl_ebo_count() { return ebo_sz; }
};

inline float fn(float a, float b) {
    return std::sin(a * b);
}

// each grid cell contains the nth x line and nth y line 

template <int len>
struct GridCell{

    Ebo_sqre ebo[len];
};

template <int x_sz,int y_sz,int line_intervl,int line_width>
struct Grid {

    // note the number of square in a surface<x_sz y_sz> is (x_sz-1)*(y_sz-1);
    glm::vec4 rgba ;

    constexpr int x_grid_len = (x_sz -1) / line_intervl;
    constexpr int y_grid_len = (y_sz -1) / line_intervl;

    GridCell<y_sz> x_lines[x_grid_len];
    GridCell<y_grid_len> y_lines[x_sz];

    Grid(Surface<x_sz,y_sz> sur,glm::vec4 clr) {
        rgba = clr;
        GridCell<x_sz>* grid_ptr= (GridCell*)(sur.ebo_arr);
        
        for (int i = 0; i < x_grid_len;i++) {
            x_lines[i].ebo = grid_ptr[i * line_intervl];
        }

        for (int i = 0; i < x_sz; i++)
        {
            for (int k = 0; k < grid_len; k++)
            {
                (y_lines[i].ebo)[k] = grid_ptr[i].ebo[k * line_intervl];

            }
        }
    }
};