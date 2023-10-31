#include "opengl_utils.hpp"
#include <cassert>
#include <cstddef>


int main()
{
    GLProgramVersion glVersion{ API_TYPE::OGL, 3, 3 };
    // GLProgramVersion glVersion{ API_TYPE::GLES, 3, 2 };
    int window_width = 600;
    int window_height = 600;

    GLFWHelper glfwHelper;
    glfwHelper.InitWindow(window_width, window_height, __FILE__, glVersion);

    /* Create two textures, one for tile, one for the whole */
    GLenum texInternalFormat = GL_RGBA;
    GLuint texTile, texAccum;
    glGenTextures(1, &texTile);
    glBindTexture(GL_TEXTURE_2D, texTile);
    std::vector<GLuint> data;

    auto fillDataByTileSize = [&data](int tile_height, int tile_width)
    {
        data.resize(tile_height * tile_width * 4);
        for (int i = 0; i < tile_height; ++i)
        {
            for (int j = 0; j < tile_width; ++j)
            {
                int base = (tile_width * i + j) * 4;
                if (j > tile_width / 2)
                {
                    data[base] = 255;
                    data[base + 1] = 0;
                }
                else {
                    data[base] = 0;
                    data[base + 1] = 255;
                }

                if (i > tile_height / 2)
                {
                    data[base + 2] = 255;
                }else {
                    data[base + 2] = 0;
                }

                data[base + 3] = 255;
            }
        }
    };

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    assert(glGetError() == GL_NO_ERROR);

    // glGenerateMipmap(GL_TEXTURE_2D);
    // assert(glGetError() == GL_NO_ERROR);
    GLuint sampler;
    glGenSamplers(1, &sampler);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindSampler(0, sampler);
    assert(glGetError() == GL_NO_ERROR);

    glGenTextures(1, &texAccum);
    glBindTexture(GL_TEXTURE_2D, texAccum);
    glTexImage2D(GL_TEXTURE_2D, 0, texInternalFormat, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    GLuint FBO;
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texAccum, 0);
    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    assert(fboStatus == GL_FRAMEBUFFER_COMPLETE);

    GLfloat vertices[] = {
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
    1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 0.0f, 0.0f, 1.0f};

    GLushort indices[] = {0, 1, 2, 0, 2, 3};

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

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
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
void main()
{
    TexCoord = aTexCoord;
    gl_Position = vec4(aPos, 1.0);
}
        )").AppendShader(glVersion, GL_FRAGMENT_SHADER, R"(
#ifdef GL_ES
precision highp float;
#endif
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D tileTex;

void main()
{
    FragColor = texture(tileTex, TexCoord);
}
        )").AttachAndLink();
    assert(program != (GLuint)-1);
    glUseProgram(program);
    assert(glGetError() == GL_NO_ERROR);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texTile);
    assert(glGetError() == GL_NO_ERROR);
    glUniform1i(glGetUniformLocation(program, "tileTex"), 0);
    assert(glGetError() == GL_NO_ERROR);

    constexpr int maxSize = 257;
    constexpr int viewport_offset_x = 0;
    constexpr int viewport_offset_y = 0;

    auto test = [&](){
        std::vector<GLuint>pixels;

        for (int tile_width = 1; tile_width < maxSize; ++tile_width)
        {
            for (int tile_height = 1; tile_height < maxSize; ++tile_height)
        {
                pixels.resize(tile_height * tile_width * 4);
                fillDataByTileSize(tile_width, tile_height);
                glBindTexture(GL_TEXTURE_2D, texTile);
                glTexImage2D(GL_TEXTURE_2D, 0, texInternalFormat, tile_width, tile_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
                glGenerateMipmap(GL_TEXTURE_2D);
                glClearColor(0.2, 0.3, 0.4, 1.0);
                glViewport(viewport_offset_x, viewport_offset_y, tile_width, tile_height);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, NULL);
                glReadPixels(viewport_offset_x, viewport_offset_y, tile_width, tile_height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
                
                bool bWholeTileMatch = true;
                for(int i = 0; i < tile_width; ++i)
                {
                    for (int j = 0; j < tile_height; ++j)
                    {
                        int base = i * j;
                        bool bAllMatch = true;
                        if (pixels[base] != data[base])
                        {
                            bAllMatch = false;
                        }

                        if (pixels[base + 1] != data[base + 1])
                        {
                            bAllMatch = false;
                        }

                        if (pixels[base + 2] != data[base + 2])
                        {
                            bAllMatch = false;
                        }

                        if (pixels[base + 3] != data[base + 3])
                        {
                            bAllMatch = false;
                        }

                        if (!bAllMatch)
                        {
                            // std::cout << "Not Match [" << i << ", " << j << "], ";
                            bWholeTileMatch = false;
                            break;
                        }
                    }
                }

                if (bWholeTileMatch)
                {
                    std::cout << "Size = " << tile_width << "," << tile_height << ": Pass." << std::endl;
                }
            }
        }
    };

    test();

    return 0;
}