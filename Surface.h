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
            angle = upper-0.01;
        }
        if (angle <= lower) {
            angle = lower+0.01;
        }

    }
    void inc_yaw(float x){
        yaw += x;
       // limit_angle(yaw,0,360); no need to bound the rotation around the Z-axis. 
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
            r * cp * cy, //x
            r * sp,      //y is independent of Yaw it is the axis of rotation of Yaw 
            r * cp * sy  //z 
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
        float y_st = c_x - x;
        float y_inc = 2 * x / (y_sz - 1);  // -1 because n-1 cordinated away from last cordinate
        float x_inc = 2 * x / (x_sz - 1);

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

template <bool buffer_data,int coords_len,int loc>
inline void coords_vao_setup
(GLuint& vboid, GLuint& eboid, GLuint& vaoid,  int* ebo_arr, int ebo_sz , float* vbo_arr = nullptr, int vbo_sz=0) {


    glGenBuffers(1, &eboid);
    glGenVertexArrays(1,&vaoid);

    glBindVertexArray(vaoid);

    glBindBuffer(GL_ARRAY_BUFFER,vboid); // Shared or not - we always make the
                                         // VBObuffer of vboid in the current context. 

    if constexpr (buffer_data) {
        glBufferData(GL_ARRAY_BUFFER, vbo_sz, vbo_arr,GL_STATIC_DRAW);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboid);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,ebo_sz,ebo_arr,GL_STATIC_DRAW);


    // since this is a VAO setup for coordinates 
    // therefore arguments for:  
    // layout = 0 for now 
    // (x,y,z) cartisian coordiates

    glVertexAttribPointer(
        loc,
        coords_len,
        GL_FLOAT,
        GL_FALSE,
        coords_len * sizeof(float),
        nullptr
    ); 

    glEnableVertexAttribArray(loc);
}

template <int x_sz, int y_sz>
struct GLSurfaceHandel {
    GLuint VBO, VAO, EBO;
    Surface<x_sz, y_sz>* surPtr;

    GLSurfaceHandel(Surface <x_sz, y_sz>* sur) {
        surPtr = sur;
        glGenBuffers(1, &VBO);
        coords_vao_setup<false,3,0>(VBO,EBO,VAO, 
            surPtr->gl_ebo_arr(), surPtr->gl_ebo_sz());
      
    }
    void commit_vbo() {
        glBufferData(GL_ARRAY_BUFFER, surPtr->gl_vbo_sz(), surPtr->gl_arr(), GL_STATIC_DRAW);
    }

    void draw() {
        
        glBindVertexArray(VAO);
        glUniform1i(is_gridLoc, false);
 
        glDrawElements(
            GL_TRIANGLES,
            surPtr->gl_ebo_count(),
            GL_UNSIGNED_INT,
            (void*)(0)
        );
    }

};
    


// each grid cell contains the nth x line and nth y line 


Ebo_sqre sqre_mirror(Ebo_sqre sq) {
    return Ebo_sqre{sq.t2,sq.t1};
}

template <int len>
struct GridCell{
    Ebo_sqre ebo[len];

};

template <int x_n,int y_n,int x_sz,int y_sz>
struct GridEbo{

    GridCell<x_sz-1> x_lines[x_n];
    GridCell<y_n> y_lines[y_sz-1];

    int* ebo_arr() {
        return reinterpret_cast<int*> (x_lines);
    }
    int gl_ebo_sz() { return sizeof(x_lines) + sizeof(y_lines); }
};

template <int line_intervl,int line_width, int x_sz, int y_sz >
struct Grid {

    static_assert(line_intervl > 0,"line_intervl must be greater than zero");
    
    // note the number of square in a surface<x_sz y_sz> is (x_sz-1)*(y_sz-1);
   
    float x_f;
    float z_f;
    static constexpr int x_grids = (y_sz - 1) / line_intervl;
    static constexpr int y_grids = (x_sz - 1) / line_intervl;
    static constexpr int ebo_stride = x_sz - 1;
    glm::vec4 rgba;

    GLuint VBO, GVAO, EBO,G_SIDE_VAO, G_SIDE_EBO;
    using GridT = GridEbo< x_grids, y_grids,x_sz,y_sz>;
    GridT main_grid;
    GridT side_grid;

    Grid() {}

    Grid(Vertex* data ,GLuint sur_VBO , int *ebo_arr, glm::vec4 clr) {

        Vertex* v_ptr = (Vertex*)(data);
        x_f = (v_ptr[1].X - v_ptr[0].X)*0.19;
        z_f = (v_ptr[0].Z - v_ptr[x_sz].Z)*0.19;

        rgba = clr;
        VBO = sur_VBO; 

        GridCell<ebo_stride>* grid_ptr = (GridCell<ebo_stride>*)(ebo_arr);
         
         // x_grid-1 and (i+1) in the loop because
         // we dont first and last , we dont want the edges
        

         // GridCell<x_sz - 1> x_lines[x_n];
     
         for (int i = 0; i < x_grids ;i++) {
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
        for (int i = 0; i < x_grids ; i++) {
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

         for (int i = 0; i < y_sz-1 ; i++) {
             for (int k = 0; k < y_grids ; k++){
                 
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


         coords_vao_setup<false,3,0> // Grid shares vbo from surface  
             (sur_VBO,
                 EBO,
                 GVAO,
                 main_grid.ebo_arr(),
                 main_grid.gl_ebo_sz()
             );

       //  coords_vao_setup<true, 3, 1> // side_Grid_coords shares vbo from surface  
        //     (sur_VBO, EBO, GVAO, side_grid.ebo_arr(), side_grid.gl_ebo_sz());


         std::cout << "grid constructor complete \n";
    }
    
    void draw() {
        
        glUniform4fv(grid_clrLoc, 1, glm::value_ptr(rgba));
        glUniform1i(is_gridLoc, true);

        glBindVertexArray(GVAO);

        glDrawElements(
            GL_TRIANGLES,
            draw_count(),
            GL_UNSIGNED_INT,
            (void*)(0)
        );
    }

  
    int draw_count() {
        return main_grid.gl_ebo_sz() / sizeof(int);
    }
};

template <int line_intervl, int line_width ,int x_sz, int y_sz >
struct SurfaceGrid {
    
    using MajorGridT = Grid <line_intervl, line_width , x_sz, y_sz>;
    //using MinorGridT = Grid <line_intervl/2, line_width, x_sz, y_sz>;

    MajorGridT major_grid{};
    //MinorGridT minor_grid{};

    SurfaceGrid(GLSurfaceHandel<x_sz, y_sz> sur_handel) {

        major_grid = MajorGridT(sur_handel.surPtr->arr, sur_handel.VBO,
                                sur_handel.surPtr->gl_ebo_arr(),
                                glm::vec4(1)  );

        //minor_grid = MinorGridT(sur_handel.VBO,
        //                        sur_handel.surPtr->gl_ebo_arr(),
        //                       glm::vec4(1) );

        sur_handel.commit_vbo();
    }

    void draw() {
        major_grid.draw();
       // minor_grid.draw();
    }
 
}; 