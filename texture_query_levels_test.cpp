#include "opengl_utils.hpp"
#include <cassert>
#include <cstddef>

int main()
{
    GLProgramVersion glVersion{API_TYPE::OGL, 4, 3};
    GLFWHelper glfwHelper;
    glfwHelper.InitWindow(500, 500, "test", glVersion);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    constexpr int tex_width = 500;
    constexpr int tex_height = 500;
    int max_levels = 7;
    assert(glGetError() == GL_NO_ERROR);

    glTexStorage2D(GL_TEXTURE_2D, max_levels, GL_RGB8, tex_width, tex_height);
    std::cout << "Generate Texture levels " << max_levels << std::endl;
    assert(glGetError() == GL_NO_ERROR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenerateMipmap(GL_TEXTURE_2D);

    auto pg = GLProgramGenerator();
    int program = pg.AppendShader(glVersion, GL_VERTEX_SHADER, R"(
void main()
{
    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
}
    )").AppendShader(glVersion, GL_FRAGMENT_SHADER, R"(
uniform sampler2D sampler1;
out vec4 color;
void main()
{
    int level = textureQueryLevels(sampler1);
    if (level == 0)
    {
        color = vec4(1.0, 0, 0, 1);
    }
    else
    {
        color = vec4((float(level) / 255.0), 0.0, 0, 1);
    }
}
)").AttachAndLink();
    assert(glGetError() == GL_NO_ERROR);

    glUseProgram(program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(program, "sampler1"), 0);
    assert(glGetError() == GL_NO_ERROR);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glfwHelper.Render([&]()
                      {
                          glClearColor(0.3, 0.3, 0.4, 1.0);
                          assert(glGetError() == GL_NO_ERROR);

                          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                          assert(glGetError() == GL_NO_ERROR);

                          glUseProgram(program);
                          assert(glGetError() == GL_NO_ERROR);

                          glPointSize(100.0f);
                          assert(glGetError() == GL_NO_ERROR);

                          glDrawArrays(GL_POINTS, 1, 1);
                          assert(glGetError() == GL_NO_ERROR);

                          float pixels[100];
                          glReadPixels(tex_width / 2, tex_height / 2, 1, 1, GL_RGBA, GL_FLOAT, pixels);
                          int level_from_shader = pixels[0] * 255;
                          if (max_levels == level_from_shader)
                          {
                              std::cout << "Pass" << std::endl;
                              exit(0);
                          }
                          else
                          {
                              std::cout << "Wrong level " << level_from_shader << ",Should be " << max_levels << std::endl;
                              exit(-1);
                          }
                      });

    return 0;
}