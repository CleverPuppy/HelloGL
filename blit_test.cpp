#include "opengl_utils.hpp"
#include <cassert>
#include <cstddef>

int main()
{
    GLProgramVersion glVersion{ API_TYPE::OGL, 3, 3 };
    GLFWHelper glfwHelper;
    constexpr int tex_width = 32 * 10;
    constexpr int tex_height = 32 * 10;
    glfwHelper.InitWindow(tex_width, tex_height, "blit_test", glVersion);

    GLProgramGenerator PG;
    GLuint program = PG.AppendShader(GL_VERTEX_SHADER, R"(
in vec2 inPos;
void main()
{
    gl_Position = vec4(inPos, 1.0, 1.0); 
}
    )").AppendShader(GL_FRAGMENT_SHADER, R"(
out vec4 color;
void main()
{
    vec2 Loc = gl_FragCoord.xy / 320 - 0.5;
    float r = 0.0;
    float g = 0.0;
    float b = 0.0;
    if (Loc.x < 0) r = 1.0;
    if (Loc.y < 0) g = 1.0;
    if (Loc.x > 0 && Loc.y > 0) b = 1.0;
    color = vec4(r, g, b, 1.0);
}
)").AttachAndLink();
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    float vboData[] = {
        -1.0, 1.0,
        1.0, 1.0,
        -1.0, -1.0,
        1.0, 1.0,
        1.0, -1.0,
        -1.0, -1.0
    };
    int glInBindPoint = glGetAttribLocation(program, "inPos");
    glBufferData(GL_ARRAY_BUFFER, sizeof(vboData), vboData, GL_STATIC_DRAW);
    glVertexAttribPointer(glInBindPoint, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
    glEnableVertexAttribArray(glInBindPoint);
    glUseProgram(program);

    glfwHelper.Render([&]()
        {
            glClearColor(0.2, 0.2, 0.2, 1.0);
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
            glPointSize(50.0);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBlitFramebuffer(160, 0, 240, 320,
                80, 0, 160, 320, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        });

    return 0;
}