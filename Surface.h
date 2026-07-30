#pragma once

//#include "FunctionArray.h"

#include <cmath>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string.h>

#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>

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

enum class Clr : int {
    R, G, B, A
};


struct MeshData {

    float* vbo_data = nullptr;
    size_t vbo_sz = 0;

    int*   ebo_data = nullptr;
    size_t ebo_sz = 0;
    
    int    draw_count;

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
    constexpr size_t gl_vbo_sz() { return size * sizeof(Vertex); }
    int* gl_ebo_arr() { return &(ebo_arr[0].t1.v1); }
    constexpr size_t gl_ebo_sz() { return ebo_sqre_sz * sizeof(Ebo_sqre); }
    constexpr int gl_ebo_count() { return ebo_sz; }

    MeshData mesh_data() {
        return {
         gl_arr    (),
         gl_vbo_sz (),
         gl_ebo_arr(),
         gl_ebo_sz (),
        };
    } 
};

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

    int* gl_ebo_arr() {
        return reinterpret_cast<int*> (x_lines);
    }

    static constexpr size_t gl_ebo_sz() {
        return sizeof(GridCell<x_sz-1>)*x_n + sizeof(GridCell<y_n>)*(y_sz-1);
        // was earlier sizeof(x_lines) + sizeof(y_lines)
    }
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
    Vertex* vbo_arr;

    Grid(Vertex* data, int* ebo_arr, glm::vec4 clr) {
        vbo_arr = data;

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

        // GridCell<y_n> y_lines [y_sz-1];
        // architecture intent of y_lines:
        // unlike x_lines the every Ebo_sqre of x_lines element is contagiously mapped to the
        // ebo array of surface . but in y lines teh required ebo_sqre 
        // elements are not contagious in memory but with interval is
        // the core amibiguity emiminator fact. wheather it's x_lines 
        // or y_lines for both them the actual rendering order is horizontal
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

                data[sqre.t1.v1].X += x_f;
                data[sqre.t1.v3].X += x_f;

                data[sqre.t2.v1].X -= x_f;
                data[sqre.t2.v3].X -= x_f;
            }

        }

        std::cout << "grid constructor complete \n";
    }

    static constexpr int draw_count() {
        return GridT::gl_ebo_sz() / sizeof(int);
    }


    MeshData mesh_data() {
        return {
         nullptr,
         0,
         main_grid.gl_ebo_arr(),
         main_grid.gl_ebo_sz(),
         draw_count()
        };
    }
};

