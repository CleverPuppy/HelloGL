#include "opengl_utils.hpp"
#include <cassert>
#include <cstddef>

int main()
{
    GLProgramVersion glVersion{ API_TYPE::GLES, 3, 2 };
    GLFWHelper glfwHelper;
    glfwHelper.InitWindow(500, 400, "test", glVersion);

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
    GLuint program = programGen.AppendShader(glVersion, GL_VERTEX_SHADER, R"(
            layout(location = 0) in vec2 Pos;
            layout(location = 1) in vec3 Color;
            out vec4 out_Color;
            void main()
            {
                gl_Position = vec4(Pos, 1, 1);
                out_Color = vec4(Color, 1);
            }
        )").AppendShader(glVersion, GL_FRAGMENT_SHADER, R"(
            in vec4 out_Color;
            out vec4 color;
            void main()
            {
                color = out_Color;
            }
        )").AttachAndLink();
    assert(program != (GLuint)-1);

    glfwHelper.Render([&glVersion, program](){
        glClearColor(0.2, 0.3, 0.4, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(program);
        glDrawArrays(GL_TRIANGLES, 0, 3);

    });

    return 0;
}