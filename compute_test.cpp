#include "opengl_utils.hpp"
#include <cassert>
#include <cstddef>

int main()
{
    GLProgramVersion glVersion{ API_TYPE::GLES, 3, 2 };
    GLFWHelper glfwHelper;
    constexpr int tex_width = 32 * 10;
    constexpr int tex_height = 32 * 10;
    glfwHelper.InitWindow(tex_width, tex_height, "test compute", glVersion);

    GLuint Tex;
    glGenTextures(1, &Tex);
    glBindTexture(GL_TEXTURE_2D, Tex);
    std::vector<float> data;
    data.reserve(tex_width * tex_height * 4);
    for (int i = 0; i < tex_width * tex_height; ++i)
    {
        int j = i * 4;
        data[j] = 1.0;
        data[j + 1] = 0.0;
        data[j + 2] = 0.0;
        data[j + 3] = 1.0;
    }
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_width, tex_height, 0, GL_RGBA, GL_FLOAT, data.data());
    assert(glGetError() == GL_NO_ERROR);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, tex_width, tex_height);
    assert(glGetError() == GL_NO_ERROR);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_width, tex_height, GL_RGBA, GL_FLOAT, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLfloat vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f };

    GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

    GLuint vbo, ibo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);


    GLuint program = GLProgramGenerator::SimpleTextureProgram(glVersion);
    assert(program != (GLuint)-1);

    // create a compute program change texture data
    GLuint ComputeProgram;
    {
        GLProgramGenerator pg;
        ComputeProgram = pg.AppendShader(glVersion, GL_COMPUTE_SHADER, R"(
            layout(rgba32f, binding=0) writeonly highp uniform image2D image;
            layout(local_size_x = 32, local_size_y = 32) in;
            uniform uint iFrame;
            void main()
            {
                float tex_max = 320.;
                float rate = 0.01;
                ivec2 p = ivec2(gl_GlobalInvocationID.xy);
                vec2 q = ( vec2(p) * 2. - tex_max) / tex_max;
                float d = length(q) - 0.2;
                vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
                color.y *= (sin(d + float(iFrame) * rate) + 1.0)/ 2.0;
                imageStore(image, p, color);
            }
        )").AttachAndLink();
        assert(ComputeProgram != (GLuint)-1);
    }

    /*
        if glMemoryBarrier is not supported,
        could use sleep to wait compute shader finish.
    */
    // sleep(3);

    glUseProgram(program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Tex);
    GLuint sampler;
    glGenSamplers(1, &sampler);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindSampler(0, sampler);
    glUniform1i(glGetUniformLocation(program, "Tex"), 0);

    assert(glGetError() == GL_NO_ERROR);
    
    int iFrameLoc = glGetUniformLocation(ComputeProgram, "iFrame");
    assert(iFrameLoc >= 0);
    uint iFrame = 0;
    glfwHelper.Render([&]()
        {
            iFrame += 1;
            glUseProgram(ComputeProgram);
            glBindImageTexture(0, Tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
            glUniform1ui(iFrameLoc, iFrame);
            assert(glGetError() == GL_NO_ERROR);
            glDispatchCompute(10, 10, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            glClearColor(0.2, 0.2, 0.2, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUseProgram(program);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
        });

    return 0;
}