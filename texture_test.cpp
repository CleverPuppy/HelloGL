#include "opengl_utils.hpp"
#include <cassert>
#include <cstddef>

int main()
{
    GLProgramVersion glVersion{API_TYPE::OGL, 3, 3};
    GLFWHelper glfwHelper;
    glfwHelper.InitWindow(500, 400, "test", glVersion);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    constexpr int tex_width = 500;
    constexpr int tex_height = 500;
    uint8_t data[tex_width * tex_height * 3];
    for(int i = 0; i < tex_height * tex_width; ++i)
    {
        int idx = i * 3;
        data[idx] = 50;
        data[idx + 1] = 0;
        data[idx + 2] = 100;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_width, tex_height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLfloat vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f};

    GLushort indices[] = {0, 1, 2, 0, 2, 3};

    GLuint vbo, ibo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);


    GLProgramGenerator programGen;
    GLuint program = programGen.AppendShader(glVersion, GL_VERTEX_SHADER, R"(
            layout(location = 0) in vec3 Pos;
            layout(location = 1) in vec2 TexCoord;
            out vec2 out_TexCoord;
            void main()
            {
                gl_Position = vec4(Pos, 1);
                out_TexCoord = TexCoord;
            }
        )")
        .AppendShader(glVersion, GL_FRAGMENT_SHADER, R"(
            in vec2 out_TexCoord;
            out vec4 color;
            uniform sampler2D texture1;
            void main()
            {
                color = texture(texture1, out_TexCoord);
            }
        )")
        .AttachAndLink();
    assert(program != (GLuint)-1);

    glUseProgram(program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(program, "texture1"), 0);

    glfwHelper.Render([&glVersion, program, tex_width, tex_height, &data]()
                      {
                          glClearColor(0.2, 0.3, 0.4, 1.0);
                          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                            for(int i = 0; i < tex_height * tex_width; ++i)
                            {
                                int idx = i * 3;
                                data[idx] += 1;
                                data[idx + 1] += 0.5;
                                data[idx + 2] += 0.75;
                            }
                          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_width, tex_height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                          glUseProgram(program);
                          glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
                      });

    return 0;
}