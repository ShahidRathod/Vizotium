#pragma once

#include "Surface.h"

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

extern GLuint is_gridLoc;
extern GLuint grid_clrLoc;



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

template <int x_sz, int y_sz> struct GLDrawHandel {
    GLuint VBO, VAO, EBO;

    float* vbo_data = nullptr;
    int vbo_sz = 0;
    int* ebo_data = nullptr;
    int ebo_sz = 0;
    int  ebo_draw_count;

    bool is_grid = false;
    glm::vec4 grid_color{};

    template <SetupState state>
    inline void coords_vao_setup() {
        
        if constexpr (!state.shared_vbo) {
            glGenBuffers(1, &VBO);
        }
        if constexpr (!state.shared_ebo) {
            glGenBuffers(1, &EBO);
        }

        if constexpr (!state.shared_vao) {
            glGenVertexArrays(1, &VAO);
        }

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO); // Shared or not - we always make the
        // VBObuffer of vboid in the current context.

        if constexpr (state.upload_vbo) {
            glBufferData(GL_ARRAY_BUFFER, vbo_sz, vbo_data, GL_STATIC_DRAW);
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

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
                3,
                GL_FLOAT,
                GL_FALSE,
                sizeof(Vertex),
                nullptr
            );
        }

        glEnableVertexAttribArray(state.loc);
    }

    // --- Surface ---
    GLDrawHandel(Surface<x_sz, y_sz>*sur) {
        vbo_data = sur->gl_arr();
        vbo_sz = sur->gl_vbo_sz();
        ebo_data = sur->gl_ebo_arr();
        ebo_sz = sur->gl_ebo_sz();
        ebo_draw_count = sur->gl_ebo_count();


        constexpr SetupState state =
        {
            false,
            false,// waiting for grid to shrink width 

            false,
            true,

            false,
            true,

            Surface<x_sz, y_sz>::ebo_sz,
            0

        };

        std::cout << "vao_setup:surface\n";
        coords_vao_setup<state>();
    }

    // --- Grid --- (shares the VBO id from the surface's GLDrawHandel)
    template <int line_intervl, int line_width>
    GLDrawHandel(Grid<line_intervl, line_width, x_sz, y_sz>&grid,
        GLDrawHandel & sur_gl) {

        VBO = sur_gl.VBO;
        ebo_data = grid.main_grid.ebo_arr();
        ebo_sz = grid.main_grid.gl_ebo_sz();
        vbo_data = sur_gl.vbo_data;
        ebo_draw_count = grid.draw_count();

        std::cout << "vao_setup:grid\n";
        constexpr SetupState state =
        {
            true, // shared vbo_data
            false, // uploading vbo this time earlier this was done by commit_vbo()

            false,
            true,

            false,
            true,

            Grid<line_intervl, line_width, x_sz, y_sz>::draw_count(),
            0

        };

        coords_vao_setup<state>();

        is_grid = true;
        grid_color = grid.rgba;
    }

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







