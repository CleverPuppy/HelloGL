#include "opengl_utils.hpp"
#include <cassert>
#include <cstddef>

int main()
{
    GLProgramVersion glVersion{API_TYPE::GLES, 3, 2};
    GLFWHelper glfwHelper;
    constexpr int tex_width = 32 * 10;
    constexpr int tex_height = 32 * 10;
    glfwHelper.InitWindow(tex_width, tex_height, "test", glVersion);

    GLuint arrTexture[2];
    glGenTextures(2, arrTexture);

    // prepare read texture
    {
        GLuint readTexture = arrTexture[0];
        glBindTexture(GL_TEXTURE_2D, readTexture);

        float data[tex_width * tex_height * 4];
        for(int i = 0; i < tex_height * tex_width; ++i)
        {
            int idx = i * 4;
            data[idx] = 0.0f;
            data[idx + 1] = 1.0f;
            data[idx + 2] = 0.0f;
            data[idx + 3] = 1.0f;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_width, tex_height, 0, GL_RGBA, GL_FLOAT, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // prepare write texture
    {
        GLuint writeTexture = arrTexture[1];
        glBindTexture(GL_TEXTURE_2D, writeTexture);
        // glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, tex_width, tex_height);

        float data[tex_width * tex_height * 4];
        for(int i = 0; i < tex_height * tex_width; ++i)
        {
            int idx = i * 4;
            data[idx] = 1.0f;
            data[idx + 1] = 0.0f;
            data[idx + 2] = 0.0f;
            data[idx + 3] = 1.0f;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_width, tex_height, 0, GL_RGBA, GL_FLOAT, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

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


    GLuint program = GLProgramGenerator::SimpleTextureProgram(glVersion);
    assert(program != (GLuint)-1);

    // create a compute program change texture data
    GLuint ComputeProgram;
    {
        GLProgramGenerator pg;
        ComputeProgram = pg.AppendShader(glVersion, GL_COMPUTE_SHADER, R"(
            layout(binding=0, rgba32f) readonly highp uniform image2D image;
            layout(binding=1, rgba32f) writeonly highp uniform image2D image2;
            layout(local_size_x = 32, local_size_y = 32) in;
            void main()
            {
                ivec2 p = ivec2(gl_GlobalInvocationID.xy);
                vec4 color = imageLoad(image, p);
                color = vec4(1.0);
                imageStore(image2, p, color);
            }
        )").AttachAndLink();
        assert(ComputeProgram != (GLuint)-1);
    }
    glUseProgram(ComputeProgram);
    glBindImageTexture(0, arrTexture[0], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, arrTexture[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glDispatchCompute(10, 10, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glFinish();

    /*
        if glMemoryBarrier is not supported,
        could use sleep to wait compute shader finish.
    */
    // sleep(3);

    glUseProgram(program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, arrTexture[1]);

    GLuint sampler;
    glGenSamplers(1, &sampler);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindSampler(0, sampler);

    glUniform1i(glGetUniformLocation(program, "Tex"), 0);

    glfwHelper.Render([&glVersion, program, tex_width, tex_height]()
                      {
                          glClearColor(0, 0, 0, 1.0);
                          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                          glUseProgram(program);
                          glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
                      });

    return 0;
}