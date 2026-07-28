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
    int loc;  
};

template <SetupState state ,typename pre_draw_call,typename post_draw_call>
struct GLDrawHandel {

    GLuint VBO, VAO, EBO;

    MeshData mesh_data;
    
    GLDrawHandel(MeshData data) {
        mesh_data = data;
    }


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
            glBufferData(GL_ARRAY_BUFFER, mesh_data.vbo_sz, mesh_data.vbo_data, GL_STATIC_DRAW);
        }
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        if (state.upload_ebo) {
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh_data.ebo_sz, mesh_data.ebo_data, GL_STATIC_DRAW);
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

    void set_draw_calls(void(*pre_call)(), void(*post_call)() ){
        pre_draw_call = pre_call;
        post_draw_call = post_call;
    }

    void commit_vbo() {
        glBufferData(GL_ARRAY_BUFFER, mesh_data.vbo_sz, mesh_data.vbo_data, GL_STATIC_DRAW);
    }

    void draw() {

        pre_draw_call();

        glBindVertexArray(VAO);
        glDrawElements(
            GL_TRIANGLES,
            mesh_data.draw_count,
            GL_UNSIGNED_INT,
            (void*)(0)
        );

        post_draw_call();

    }

};







