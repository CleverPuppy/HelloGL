#include "opengl_utils.hpp"
#include <cassert>
#include <cstddef>

int main()
{
    GLProgramVersion glVersion{ API_TYPE::OGL, 4, 3 };
    // GLProgramVersion glVersion{ API_TYPE::GLES, 3, 2 };
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

    const int locationBase = 2;
    glVertexAttribPointer(locationBase + 0, 2, GL_FLOAT, GL_FALSE, sizeof(Triangle), (void*)0);
    glEnableVertexAttribArray(locationBase + 0);

    glVertexAttribPointer(locationBase + 1, 3, GL_FLOAT, GL_FALSE, sizeof(Triangle), (void*)sizeof(vec2));
    glEnableVertexAttribArray(locationBase + 1);

    struct UniformData {
        vec4 vec1; // offset = 0
        vec4 vec2; // offset = 16
        vec4 vec3; // offset = 32
    };

    UniformData uniformData = {{1.0, 0.0, 0.0, 1.0},
                               {0.0, 1.0, 0.0, 1.0},
                               {0.0, 0.0, 1.0, 1.0}};

    GLProgramGenerator programGen;
    GLuint program = programGen.AppendShader(glVersion, GL_VERTEX_SHADER, R"(
            #extension GL_ARB_enhanced_layouts: require
            const int locationBase = 2;
            layout(location = locationBase + 0) in vec2 Pos;
            layout(location = locationBase + 1) in vec3 Color;
            uniform Block {
                layout(offset = 0) vec4 color1;
                layout(offset = 32) vec4 color3;
            };
            out vec4 out_Color;
            void main()
            {
                gl_Position = vec4(Pos, 1, 1);
                // out_Color = vec4(Color, 1);
                out_Color = color1 + color3;
            }
        )").AppendShader(glVersion, GL_FRAGMENT_SHADER, R"(
            precision highp float;
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