#pragma once

#include "Surface.h"

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

extern GLuint is_gridLoc;
extern GLuint grid_clrLoc;



struct SetupState {
    bool new_vbo;
    bool new_ebo;

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



    MeshData data;
    
    template <SetupState state>
    inline void coords_vao_setup() {

        if constexpr (state.new_vbo) {
            glGenBuffers(1, &VBO);
        }
        if constexpr (state.new_ebo) {
            glGenBuffers(1, &EBO);
        }

        glGenVertexArrays(1, &VAO);


        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO); // Shared or not - we always make the
        // VBObuffer of vboid in the current context.

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);


        // since this is a VAO setup for coordinates
        // therefore arguments for:
        // layout = 0 for now
        // (x,y,z) cartisian coordiates


        glVertexAttribPointer(
            state.loc,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            nullptr
        );

        glEnableVertexAttribArray(state.loc);
    }
    
    void upload_vbo() {

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vbo_sz, vbo_data, GL_STATIC_DRAW);
    }

    void upload_ebo() {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, ebo_sz, ebo_data, GL_STATIC_DRAW);
    }


    // --- Surface ---
    GLDrawHandel(Surface<x_sz, y_sz>* sur, MeshData given_data) {


        vbo_data = sur->gl_arr();
        vbo_sz = sur->gl_vbo_sz();
        ebo_data = sur->gl_ebo_arr();
        ebo_sz = sur->gl_ebo_sz();
        ebo_draw_count = sur->gl_ebo_count();


        constexpr SetupState state = {true,true,0};

        std::cout << "vao_setup:surface\n";
        coords_vao_setup<state>();
    }
    
    // --- Grid --- (shares the VBO id from the surface's GLDrawHandel)
    template <int line_intervl, int line_width>
    GLDrawHandel(Grid<line_intervl, line_width, x_sz, y_sz>& grid,
        GLDrawHandel& sur_gl, MeshData give_data ) {

        VBO = sur_gl.VBO;

        ebo_data = grid.main_grid.gl_ebo_arr();
        ebo_sz = grid.main_grid.gl_ebo_sz();
        vbo_data = sur_gl.vbo_data;
        ebo_draw_count = grid.draw_count();
        
        std::cout << "vao_setup:grid\n";
        
        
        constexpr SetupState state = { false,true,0};

        coords_vao_setup<state>();

        is_grid = true;
        grid_color = grid.rgba;
    }

    // Only meaningful for a handle that owns its VBO (the Surface case) -
    // Grid's handle has no vbo_data and should never call this.

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






