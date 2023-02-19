#include "opengl_utils.hpp"
#include <OpenGL/gl.h>
#include <cassert>
#include <cstddef>

int main()
{
    GLFWHelper glfwHelper;
    glfwHelper.InitWindow(500, 400, "test", ContextType::OpenGL, 4, 1);
    glfwHelper.Render([](){
        glClearColor(0.2, 0.3, 0.4, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GLuint VAO, VBO;

        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        struct Triangle {
            vec2 pos;
            vec3 color;
        } triangles[3] = {
            {-0.7f, -0.2, 1.0f, 0.0f, 0.0f},
            {0.7f, -0.2, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.5, 0.0f, 0.0f, 1.0f},
        };

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(triangles), triangles, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Triangle), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Triangle), (void*)sizeof(vec2));
        glEnableVertexAttribArray(1);

        GLProgramGenerator programGen;
        GLuint program = programGen.AppendShader(GL_VERTEX_SHADER, R"(#version 410 core
            layout(location = 0) in vec2 Pos;
            layout(location = 1) in vec3 Color;
            out vec4 out_Color;
            void main()
            {
                gl_Position = vec4(Pos, 1, 1);
                out_Color = vec4(Color, 1);
            }
        )").AppendShader(GL_FRAGMENT_SHADER, R"(#version 410
            in vec4 out_Color;
            out vec4 color;
            void main()
            {
                color = out_Color;
            }
        )").AttachAndLink();
        assert(program != (GLuint)-1);

        glUseProgram(program);
        glDrawArrays(GL_TRIANGLES, 0, 3);

    });

    return 0;
}