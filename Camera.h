#pragma once

#include <cmath>
#include <iostream>

#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

inline bool mat_debug = true;

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

#define DEBUG_MATRIX(name) if(mat_debug){ std::cout <<"\n\n" << #name << ":\n"; print_mat(name); }
#define DEBUG(name) if(mat_debug){ std::cout <<"\n\n" << #name << ":\n"; std::cout<<name; }

#define X_AXIS_VEC3 glm::vec3 (1,0,0)
#define Y_AXIS_VEC3 glm::vec3 (0,1,0)
#define Z_AXIS_VEC3 glm::vec3 (0,0,1)

#define X_AXIS_VEC4 glm::vec4 (1,0,0,0)
#define Y_AXIS_VEC4 glm::vec4 (0,1,0,0)
#define Z_AXIS_VEC4 glm::vec4 (0,0,1,0)

#define RAD(x) glm::radians((x))

constexpr double PI = 3.14159265358979323846;

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

    inline void limit_angle(float& angle, float lower, float upper) {
        if (angle >= upper) {
            angle = upper - 0.01;
        }
        if (angle <= lower) {
            angle = lower + 0.01;
        }
    }

    void inc_yaw(float x) {
        yaw += x;
        // limit_angle(yaw,0,360); no need to bound the rotation around the Z-axis.
    }

    void inc_pitch(float y) {
        pitch += y;
        limit_angle(pitch, 0, 180);
    }

    void scale_inc(float s) { scale += s; }
    inline float give_scale() { return std::exp(scale); }

    glm::mat4 model() {
        glm::mat4 model(1.0f);

        model = glm::rotate(model, RAD(yaw), Z_AXIS_VEC3);
        model = glm::rotate(model, RAD(pitch), X_AXIS_VEC3);
        model = glm::scale(model, glm::vec3(give_scale()));

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
            r * cp * cy, //x
            r * sp,      //y is independent of Yaw it is the axis of rotation of Yaw
            r * cp * sy  //z
        );

        int sign = sgn(cp);

        std::cout << "\nsign: " << sign;
        view = glm::lookAt(glm::vec3(eye), glm::vec3(0, 0, 0), glm::vec3(0, sign * 1, 0));
        DEBUG_MATRIX(view);
        return view;
    }

    glm::mat4 update_MVP() {
        glm::mat4 mvp = (perspective() * (view()));
        DEBUG_MATRIX(mvp);
        return mvp;
    }
};
