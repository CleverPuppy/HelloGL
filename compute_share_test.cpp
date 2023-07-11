#include "opengl_utils.hpp"
#include <cassert>
#include <cstddef>
#include <cmath>

int main()
{
    GLProgramVersion glVersion{ API_TYPE::GLES, 3, 2 };
    // GLProgramVersion glVersion{ API_TYPE::OGL, 4, 3 };
    GLFWHelper glfwHelper;
    constexpr int tex_width = 32 * 10;
    constexpr int tex_height = 32 * 10;
    glfwHelper.InitWindow(tex_width, tex_height, "test compute", glVersion);

    std::vector<float> input_data;
    for(int i = 0; i < 10; ++i)
    {
        input_data.emplace_back((float)i);
    }

    GLuint ssbo_input, ssbo_output;
    glGenBuffers(1, &ssbo_input);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_input);
    glBufferData(GL_SHADER_STORAGE_BUFFER, input_data.size() * sizeof(float), input_data.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &ssbo_output);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_output);
    glBufferData(GL_SHADER_STORAGE_BUFFER, input_data.size() * sizeof(float), input_data.data(), GL_DYNAMIC_DRAW);

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
            layout(local_size_x = 10) in;
            uniform uint iFrame;
            shared uint g_shared_data[128];
            layout (std430, binding = 1) buffer block2
            {
                float output_data[gl_WorkGroupSize.x];
            };
            void main()
            {
                uint id = gl_LocalInvocationID.x;
                g_shared_data[(id + 1u) % 10u] = id;
                groupMemoryBarrier();
                barrier();
                g_shared_data[id] = g_shared_data[id] + id;
                output_data[id] = float(g_shared_data[id]);
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
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_input);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_input);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_output);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_output);
            glUniform1ui(iFrameLoc, iFrame);
            assert(glGetError() == GL_NO_ERROR);
            glDispatchCompute(1, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            glFinish();
            std::vector<float> out;
            std::vector<float> reference = {9, 1, 3, 5, 7, 9, 11, 13, 15, 17};
            out.resize(10);

            void* mappedData = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, out.size() * sizeof(float), GL_MAP_READ_BIT);
            printf("%p\n", mappedData);
            if (mappedData) {
                // 使用映射后得到的指针读取缓冲区的内容
                // 例如: memcpy(data.data(), mappedData + offset, size);

                // 解除映射
                for (int i = 0; i < 10; ++i)
                {
                    out[i] =  *((float*)mappedData + i);
                }

                glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
            }

            // 解绑缓冲区对象
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

            // glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float) * out.size(), out.data());
            std::cout << "Output : ";
            for (auto v : out)
            {
                std::cout << v << " ";
            }
            std::cout << std::endl;

            auto float_equal = [](float a, float b) {
                return fabs(a - b) < 1e-10;
            };

            bool bPass = true;
            for(int i = 0; i < 10; ++i)
            {
                if (!float_equal(out[i], reference[i]))
                {
                    bPass = false;
                    break;
                }
            }

            if (!bPass)
            {
                std::cerr << "Not Match" << std::endl;
            }
            else
            {
                std::cout << "Pass" << std::endl;
            }

            exit(0);
        });

    return 0;
}