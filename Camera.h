#pragma once

#include <cmath>
#include <iostream>

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>

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

extern glm::vec3* surface_offset;

struct Camera {
    float yaw = 90.f;
    float pitch = 0.1f;
    float scale = 0;

    float fov = 55.0f;
    float aspect = 1280.0f / 720.0f;
    glm::vec3 offset{ .2f };

    glm::vec3 free_position;
    glm::vec3 free_eye;
    glm::vec3 free_eye_p;

    float eye_yaw;
    float eye_pitch;

    float mouse_x;
    float mouse_y;

    float dx, dy;

    float mouse_x_centre;
    float mouse_y_centre;

    Camera() {

        yaw = 0.1;
        pitch = 60;
        scale = 1;
        free_position = glm::vec3(1,1,1);

        free_eye = glm::vec3(1,0, 0);
        free_eye_p = glm::vec3(0, 0, 1);

        eye_yaw = eye_pitch = mouse_x = mouse_y = 0;

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
        glm::mat4 view (1);
       
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


        view *= glm::lookAt(glm::vec3(eye), glm::vec3(0,0,0), glm::vec3(0, sign * 1, 0));
        DEBUG_MATRIX(view);
        return view;
    }

    glm::mat4 update_MVP() {

        glm::mat4 mvp =  perspective()*view();
        //mvp= glm::translate(mvp,*surface_offset);
        std::cout << "\nscale: " << scale;
        DEBUG_MATRIX(mvp);
        return mvp;
    }

    glm::mat4 free_look() {

        glm::mat4 view(1);
        glm::translate(view, free_position);

        view *= glm::lookAt(free_position, free_position-free_eye, glm::vec3(0, 1, 0));

        return perspective() * view;
    }

    void inc_free_x(float val) {
        free_position += val * free_eye;
    }
    void inc_free_z(float val) {
        free_position += val * free_eye_p;
    }

    void inc_free_y(float val) {
        free_position.y += val;
    }


    glm::vec3  apply_mouse_rotation(glm::vec3 & eye,glm::vec3 axis2) {
        
        eye = glm::rotate(eye, RAD(eye_yaw), -Y_AXIS_VEC3);
        
        glm::vec3 rotated_z = glm::rotate(Z_AXIS_VEC3, RAD(eye_yaw), -Y_AXIS_VEC3);
        eye = glm::rotate(eye, RAD(eye_pitch), rotated_z);

        return eye;
    }

    void calibrate_eye(float x,float y) {

        dx = (x-mouse_x)/4;
        dy = (y-mouse_y)/4;

        mouse_x = x;
        mouse_y = y;

        eye_yaw += dx;
        eye_pitch += dy;

        glm::vec3 base_eye(1, 0, 0);

        glm::vec3 base_eye_p(0, 0, 1);

        
        free_eye = apply_mouse_rotation(base_eye,Z_AXIS_VEC3);
        free_eye_p = apply_mouse_rotation(base_eye_p,Z_AXIS_VEC3);


        /*print_vec(free_eye);
        print_vec(free_eye_p);
        */
    }

    void debug_eye_angles() {

     /*  std::cout << "\neye_yaw: " << eye_yaw << "  eye_pitch: " << eye_pitch
            << "\ndx: " << dx << "  dy: " << dy << "\n\n"
            << "\n mouse_x:  " << mouse_x << "mouse_y:  " << mouse_y
            << "\n mouse_x_c:  " << mouse_x_centre << "mouse_y_c:  " << mouse_y_centre;*/
    }

    void no_input() {
        //mouse_x = mouse_x_centre;
        //mouse_y = mouse_y_centre;
    }
};


