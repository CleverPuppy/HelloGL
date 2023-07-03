#include "opengl_utils.hpp"
#include <cassert>
#include <cstddef>

int main()
{
    GLProgramVersion glVersion{ API_TYPE::GLES, 3, 2 };
     //GLProgramVersion glVersion{ API_TYPE::OGL, 4, 6 };
    GLFWHelper glfwHelper;
    constexpr int tex_width = 32 * 10;
    constexpr int tex_height = 32 * 10;
    glfwHelper.InitWindow(tex_width, tex_height, "test compute", glVersion);

    std::vector<float> data;
    data.resize(tex_width * tex_height * 4);

    for (int i = 0; i < tex_width * tex_height; ++i)
    {
        int j = i * 3;
        data[j] = 1.0;
        data[j + 1] = 0.0;
        data[j + 2] = 0.0;
    }
    
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    assert(glGetError() == GL_NO_ERROR);

    GLfloat vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f };

    GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
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

    assert(glGetError() == GL_NO_ERROR);

    GLuint RenderProgram;
    {
        GLProgramGenerator pg;
        RenderProgram = pg.AppendShader(glVersion, GL_VERTEX_SHADER, R"(
            layout (std430, binding = 1) buffer Pixels
            {
                vec3 data_ssbo[320 * 320];
            } pixels;
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec2 aFragCord;
            out vec2 aFragCoord;
            void main()
            {
                aFragCoord = aFragCord;
                gl_Position = vec4(aPos, 1);
            }
        )").AppendShader(glVersion, GL_FRAGMENT_SHADER, R"(
            precision highp float;
            precision highp int;
            layout (std430, binding = 1) buffer Pixels
            {
                vec3 data_ssbo[320 * 320];
            } pixels;
            in vec2 aFragCoord;
            out vec4 fragColor;
            uniform uint iFrame;

            void getColorByFragCoord(out vec3 outColor)
            {
                float rate = 0.01;
                vec2 q = gl_FragCoord.xy / 320. * 2. - 1.;
                float d = length(q) - 0.2;
                vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
                color.y *= (sin(d + float(iFrame) * rate) + 1.0)/ 2.0;
                outColor = color.xyz;
            }
            void main()
            {
                float width  = 320.;
                float height = 320.;
                fragColor = vec4(1., 1., 1., 1.);
                uint aLoc =  uint(gl_FragCoord.x)  * uint(320) + uint(gl_FragCoord.y)  ;
                fragColor = vec4(pixels.data_ssbo[aLoc], 1.0);
                //vec3 color2;
                //getColorByFragCoord(color2);
                //fragColor.xyz -= color2;
            }
        )").AttachAndLink();
        assert(RenderProgram != (GLuint)-1);
    }

    assert(glGetError() == GL_NO_ERROR);

    // create a compute program change texture data
    GLuint ComputeProgram;
    {
        GLProgramGenerator pg;
        ComputeProgram = pg.AppendShader(glVersion, GL_COMPUTE_SHADER, R"(
            layout(rgba32f, binding=0) writeonly highp uniform image2D image;
            layout(local_size_x = 32, local_size_y = 32) in;
            uniform uint iFrame;
            layout (std430, binding = 1) buffer Pixels
            {
                vec3 data_ssbo[320 * 320];
            } pixels;
            void main()
            {
                float tex_max = 320.;
                float rate = 0.01;
                uvec2 p = gl_GlobalInvocationID.xy;
                vec2 q = vec2(p) / 320. * 2. - 1.;
                float d = length(q) - 0.2;
                vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
                color.y *= (sin(d + float(iFrame) * rate) + 1.0)/ 2.0;
                uint idx = p.x * uint(320) + p.y;
                pixels.data_ssbo[idx] = color.xyz;
            }
        )").AttachAndLink();
        assert(ComputeProgram != (GLuint)-1);
    }

    assert(glGetError() == GL_NO_ERROR);
    
    int iFrameLoc = glGetUniformLocation(ComputeProgram, "iFrame");
    // assert(iFrameLoc >= 0);
    GLuint iFrame = 0;
    glfwHelper.Render([&]()
        {
            iFrame += 1;
            glUseProgram(ComputeProgram);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);
            glUniform1ui(iFrameLoc, iFrame);
            assert(glGetError() == GL_NO_ERROR);
            glDispatchCompute(10, 10, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            glClearColor(0.2, 0.2, 0.2, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
            glUseProgram(RenderProgram);
            glUniform1ui(glGetUniformLocation(RenderProgram, "iFrame"), iFrame);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        });

    return 0;
}