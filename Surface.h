#pragma once

//#include "FunctionArray.h"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <string.h>

#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

bool mat_debug = true;

struct Color {
    float arr[4];
};

extern GLuint is_gridLoc;
extern GLuint grid_clrLoc;

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


template <int x_sz, int y_sz>
class Surface {
public:

    constexpr static int size = x_sz * y_sz;
    constexpr static int ebo_sqre_sz = (x_sz - 1) * (y_sz - 1);
    constexpr static int ebo_sz = ebo_sqre_sz * 6;

    float inc;

    Ebo_sqre ebo_arr[ebo_sqre_sz];
    Vertex arr[size];


    Surface(const float c_x, const float c_y, const float x, const float y) {

        float x_st = c_x - x;
        float y_st = c_x - x;
        float y_inc = 2 * x / (y_sz - 1);  // -1 because n-1 cordinated away from last cordinate
        float x_inc = 2 * x / (x_sz - 1);
        float inc = x_inc;
        /// Eb array insitialization 
        int ebo_stride = x_sz - 1;


        for (int i = 0; i < y_sz - 1; i++) {
            for (int j = 0; j < ebo_stride; j++) {
                int indx = j + i * ebo_stride;

                Ebo_sqre& sqre = ebo_arr[indx];

                // ebo array is GLTringle coordinate mappings and arr has stride x_sz
                int ebo_indx = j + i * x_sz;

                sqre = Ebo_sqre{
                    {ebo_indx, ebo_indx + 1, ebo_indx + x_sz},
                    {ebo_indx + 1, ebo_indx + x_sz, ebo_indx + x_sz + 1}
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

        std::cout << "surface constructor complete \n";
    }


    float* gl_arr() { return &(arr[0].X); }

    constexpr int gl_vbo_sz() { return size * sizeof(Vertex); }
    int* gl_ebo_arr() { return &(ebo_arr[0].t1.v1); }

    constexpr int gl_ebo_sz() { return ebo_sqre_sz * sizeof(Ebo_sqre); }
    constexpr int gl_ebo_count() { return ebo_sz; }
};

inline float fn(float a, float b) {
    return std::sin(a * b);
}


// The single generalized GL draw object (GLDrawHandel) is defined further
// below, after Grid, since it needs both Surface and Grid to be fully
// defined for its overloaded constructors.


// each grid cell contains the nth x line and nth y line 


Ebo_sqre sqre_mirror(Ebo_sqre sq) {
    return Ebo_sqre{ sq.t2,sq.t1 };
}

template <int len>
struct GridCell {
    Ebo_sqre ebo[len];

};

template <int x_n, int y_n, int x_sz, int y_sz>
struct GridEbo {

    GridCell<x_sz - 1> x_lines[x_n];
    GridCell<y_n> y_lines[y_sz - 1];

    int* ebo_arr() {
        return reinterpret_cast<int*> (x_lines);
    }

    constexpr int gl_ebo_sz() { return sizeof(x_lines) + sizeof(y_lines); }
};

template <int line_intervl, int line_width, int x_sz, int y_sz >
struct Grid {

    static_assert(line_intervl > 0, "line_intervl must be greater than zero");

    // note the number of square in a surface<x_sz y_sz> is (x_sz-1)*(y_sz-1);

    float x_f;
    float z_f;
    static constexpr int x_grids = (y_sz - 1) / line_intervl;
    static constexpr int y_grids = (x_sz - 1) / line_intervl;
    static constexpr int ebo_stride = x_sz - 1;
    glm::vec4 rgba;

    using GridT = GridEbo< x_grids, y_grids, x_sz, y_sz>;
    GridT main_grid;
    GridT side_grid;

    Grid(Vertex* data, int* ebo_arr, glm::vec4 clr) {

        Vertex* v_ptr = (Vertex*)(data);
        x_f = (v_ptr[1].X - v_ptr[0].X) * 0.19;
        z_f = (v_ptr[0].Z - v_ptr[x_sz].Z) * 0.19;

        rgba = clr;

        GridCell<ebo_stride>* grid_ptr = (GridCell<ebo_stride>*)(ebo_arr);

        // x_grid-1 and (i+1) in the loop because
        // we dont first and last , we dont want the edges


        // GridCell<x_sz - 1> x_lines[x_n];

        for (int i = 0; i < x_grids;i++) {
            //  x_lines[i].ebo = grid_ptr[i * line_intervl];
            Ebo_sqre* ith_line = main_grid.x_lines[i].ebo;
            Ebo_sqre* ith_side = side_grid.x_lines[i].ebo;

            memcpy(ith_line, &grid_ptr[(i + 1) * line_intervl], sizeof(Ebo_sqre) * ebo_stride);
            for (int i = 0; i < ebo_stride;i++) {
                ith_side[i] = sqre_mirror(ith_line[i]);
            }

        }

        // x lines with shortening lines along X are make thinner along Z and vice versa

        Vertex* vertx = (Vertex*)(data);
        for (int i = 0; i < x_grids; i++) {
            Ebo_sqre* ith_line = (Ebo_sqre*)(main_grid.x_lines[i].ebo);
            for (int k = 0; k < ebo_stride;k++) {
                Ebo_sqre sqre = ith_line[k];

                data[sqre.t1.v1].Z -= z_f;
                data[sqre.t1.v2].Z -= z_f;

                data[sqre.t1.v3].Z += z_f;
                data[sqre.t2.v3].Z += z_f;

            }
        }


        //GridCell<y_n> y_lines [y_sz-1];
        // architecture intent of y_lines:
        // unlike x_lines the every Ebo_sqre of x_lines element is contagiously mapped to the 
        // ebo array of surface . but in y lines teh required ebo_sqre elements are not contagious in memory but with interval is 
        // the core amibiguity emiminator fact. wheather it's x_lines or y_lines for both them the actual rendering order is horizontal 
        // always the grid ebo in a given veertical line is 

        for (int i = 0; i < y_sz - 1; i++) {
            for (int k = 0; k < y_grids; k++) {

                main_grid.y_lines[i].ebo[k] = grid_ptr[i].ebo[(k + 1) * line_intervl];
                side_grid.y_lines[i].ebo[k] = sqre_mirror(main_grid.y_lines[i].ebo[k]);

            }
        }

        // y lines width shortening

        for (int i = 0; i < y_sz - 1; i++) {

            for (int k = 0; k < y_grids; k++) {

                Ebo_sqre sqre = (Ebo_sqre)main_grid.y_lines[i].ebo[k];

                /*float before[] = {

                vertx[sqre.t1.v1].X,
                vertx[sqre.t1.v2].X,
                vertx[sqre.t1.v3].X,
                vertx[sqre.t2.v1].X
                };
                */

                data[sqre.t1.v1].X += x_f;
                data[sqre.t1.v3].X += x_f;

                data[sqre.t2.v1].X -= x_f;
                data[sqre.t2.v3].X -= x_f;

                /* float after[] = {

                vertx[sqre.t1.v1].X,
                vertx[sqre.t1.v2].X,
                vertx[sqre.t1.v3].X,
                vertx[sqre.t2.v1].X
                };



                for (int i = 0; i < 4; i++)
                 {
                     std::cout << " | "<<before[i] << " | " << after[i] <<"  |  "<<before[i]-after[i] << "\n";
                 }
                std::cout << "\n\n\n";*/
            }
        }


        std::cout << "grid constructor complete \n";
    }

    int draw_count() {
        return main_grid.gl_ebo_sz() / sizeof(int);
    }
};

struct SetupState {

    bool shared_vbo;
    bool upload_vbo;
    
    bool shared_ebo;
    bool upload_ebo;

    bool shared_vao;
    bool point_vao;
    
   

    int coords_len;
    int loc;

};


template <int x_sz, int y_sz>
struct GLDrawHandel {
    GLuint VBO, VAO, EBO;

    float* vbo_data = nullptr;
    int vbo_sz = 0; 
    int* ebo_data = nullptr; 
    int ebo_sz = 0;
    constexpr int  ebo_draw_count = 0;

    bool is_grid = false;
    glm::vec4 grid_color{};

    template <SetupState state>
    inline void coords_vao_setup() {
        
        if constexpr (!state.shared_ebo) {
            glGenBuffers(1, &VBO);
        

        if constexpr (!state.shared_ebo) {
            glGenBuffers(1, &EBO);
        }

        if constexpr (!state.shared_vao) {
            glGenVertexArrays(1, &VAO);
        }

        glBindVertexArray(vaoid);

        glBindBuffer(GL_ARRAY_BUFFER, vboid); // Shared or not - we always make the
        // VBObuffer of vboid in the current context. 

        if constexpr (buffer_data) {
            glBufferData(GL_ARRAY_BUFFER, vbo_sz, vbo_data, GL_STATIC_DRAW);
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboid);
        
        if (state.upload_ebo) {
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, ebo_sz, ebo_data, GL_STATIC_DRAW);
        }

        // since this is a VAO setup for coordinates 
        // therefore arguments for:  
        // layout = 0 for now 
        // (x,y,z) cartisian coordiates

        if constexpr (state.point_vao) {
            glVertexAttribPointer(
                state.loc,
                coords_len,
                GL_FLOAT,
                GL_FALSE,
                coords_len * sizeof(float),
                nullptr
            );
        }

        glEnableVertexAttribArray(state.loc); 
      
    }

    // --- Surface ---
    GLDrawHandel(Surface<x_sz, y_sz>* sur) {

        vbo_data = sur->gl_arr();     
        vbo_sz = sur->gl_vbo_sz();
        ebo_data = sur->gl_ebo_arr(); 
        ebo_sz = sur->gl_ebo_sz();
        ebo_draw_count  = sur->gl_ebo_count();

        constexpr SetupState state = 
        {
            false,
            false,

            false,
            true,

            false,
            true

            ebo_draw_count,
            0

        };

        glGenBuffers(1, &VBO); // owns the VBO
        coords_vao_setup<state>();
    }

    // --- Grid --- (shares the VBO id from the surface's GLDrawHandel)
    template <int line_intervl, int line_width>
    GLDrawHandel(Grid<line_intervl, line_width, x_sz, y_sz>& grid,
        GLDrawHandel& sur_gl) {

        VBO = sur_gl.VBO; 

        ebo_data = grid.main_grid.ebo_arr();
        ebo_sz = grid.main_grid.gl_ebo_sz();

        ebo_draw_count = grid.draw_count();

        constexpr SetupState state =
        {
            true, // shared vbo_data
            false, // not commiting vbo yet cuz we have commit_vbo after the grid constructor

            false,
            true,

            false,
            true

            ebo_draw_count,
            1

        };

        coords_vao_setup<state>();

        is_grid = true;
        grid_color = grid.rgba;
    }

    template <int coords_n>
    GLDrawHandel(Vector<coords_n> vector);
    
    
    // Only meaningful for a handle that owns its VBO (the Surface case) -
    // Grid's handle has no vbo_data and should never call this.
    void commit_vbo() {
        glBufferData(GL_ARRAY_BUFFER, vbo_sz, vbo_data, GL_STATIC_DRAW);
    }

    void draw() {
        glBindVertexArray(VAO);
        glUniform1i(is_gridLoc, is_grid);

        if (is_grid) {
            glUniform4fv(grid_clrLoc, 1, glm::value_ptr(grid_color));
        }

        glDrawElements(
            GL_TRIANGLES,
            ebo_draw_count,
            GL_UNSIGNED_INT,
            (void*)(0)
        );
    }

};
