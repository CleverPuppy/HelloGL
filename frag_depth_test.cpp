#include "opengl_utils.hpp"
#include <cassert>
#include <cstddef>

int main()
{
    GLProgramVersion glVersion{ API_TYPE::OGL, 3, 3 };
    GLFWHelper glfwHelper;
    GLuint window_width = 500;
    GLuint window_height = 500;
    glfwHelper.InitWindow(window_width, window_width, "test", glVersion);

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

    glViewport(0, 0, window_width, window_height);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangles), triangles, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Triangle), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Triangle), (void*)sizeof(vec2));
    glEnableVertexAttribArray(1);

    GLuint FBO;
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    GLuint buffers[] ={GL_COLOR_ATTACHMENT0,
                      GL_COLOR_ATTACHMENT1,
                      GL_COLOR_ATTACHMENT2,
                      GL_COLOR_ATTACHMENT3,
                      GL_COLOR_ATTACHMENT4,
                      GL_COLOR_ATTACHMENT5,
                      GL_COLOR_ATTACHMENT6,
                      GL_COLOR_ATTACHMENT7,
                      };
    glDrawBuffers(8, buffers);

    GLuint depthBuffer;
    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window_width, window_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

    constexpr int texNum = 8;
    GLuint Tex[texNum];
    glGenTextures(texNum, Tex);
    GLenum target = GL_TEXTURE_2D;
    for(int i = 0; i < texNum; ++i)
    {
        glBindTexture(target, Tex[i]);
        glTexImage2D(target, 0, GL_RGBA, window_width, window_height, 0, GL_RGBA, GL_FLOAT, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, target, Tex[i], 0);
    }

    GLProgramGenerator programGen;
    /*
        If linker treat gl_FragDepth as color buffer,
        then the linker will fail when uses all color buffer and gl_FragDepth.
    */
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
            out vec4 FragColor1;
            out vec4 FragColor2;
            out vec4 FragColor3;
            out vec4 FragColor4;
            out vec4 FragColor5;
            out vec4 FragColor6;
            out vec4 FragColor7;
            out vec4 FragColor8;
            void main()
            {
                FragColor1 = vec4(1.0f);
                FragColor2 = vec4(0.5f);
                FragColor3 = vec4(0.5f);
                FragColor4 = vec4(0.5f);
                FragColor5 = vec4(0.5f);
                FragColor6 = vec4(0.5f);
                FragColor7 = vec4(0.5f);
                FragColor8 = vec4(0.5f);
                gl_FragDepth = 0.5f;
            }
        )").AttachAndLink();
    assert(program != (GLuint)-1);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(program);
    glEnable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    float glDepthValue = 2.0f;
    float colors[4];
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(window_width / 2,  window_height / 2, 1, 1, GL_RGBA, GL_FLOAT, colors);
    glReadPixels(window_width / 2,  window_height / 2, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &glDepthValue);

    std::cout << "glDepthValue of triangle center is " << glDepthValue << std::endl;
    std::cout << "Center color is [" << colors[0] << ", "
                                     << colors[1] << ", "
                                     << colors[2] << ", "
                                     << colors[3] << "]" << std::endl;

    glfwHelper.Render([&glVersion, program](){
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.2, 0.3, 0.4, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(program);
        glDrawArrays(GL_TRIANGLES, 0, 3);

    });

    return 0;
}