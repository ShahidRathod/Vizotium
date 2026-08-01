#pragma once

#include "Surface.h"

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstring>


struct SetupState {
    bool new_vbo;
    bool new_ebo;

    int loc;
};

template <SetupState state> struct GLDrawHandel {
    GLuint VBO, VAO, EBO;
    MeshData data;

    inline void vao_setup(GLuint s_vbo,GLuint s_ebo) {

        if constexpr (state.new_vbo) {
            glGenBuffers(1, &VBO);
        }
        else {
            VBO = s_vbo;
        }
        if constexpr (state.new_ebo) {
            glGenBuffers(1, &EBO);
        }
        else {
            EBO = s_ebo;
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
        glBufferData(GL_ARRAY_BUFFER, data.vbo_sz, data.vbo_data, GL_STATIC_DRAW);
    }

    void upload_ebo() {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.ebo_sz, data.ebo_data, GL_STATIC_DRAW);
    }

    GLDrawHandel(MeshData given_data,GLuint shared_vbo = 0,GLuint shared_ebo = 0) {
        data = given_data;
        vao_setup(shared_vbo, shared_ebo);
    }
  

    template <typename FuncT>
    void draw(FuncT fn) {
        fn();
        glBindVertexArray(VAO);

        glDrawElements(
            GL_TRIANGLES,
            data.draw_count,
            GL_UNSIGNED_INT,
            (void*)(0)
        );


    }

};

