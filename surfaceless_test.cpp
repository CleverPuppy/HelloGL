#include "opengl_utils.hpp"

int main()
{
    auto helper = SurfaceLessHelper();
    // auto glVersion = GLProgramVersion{API_TYPE::OGL, 3, 3};
    auto glVersion = GLProgramVersion{API_TYPE::GLES, 3, 2};

    Assert(helper.Init(glVersion));

    GLuint VAO, VBO;

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    struct Triangle {
        vec2 pos;
        vec3 color;
    } triangles[3] = {
        {-0.7f, -0.2, 1.0f, 0.0f, 0.0f},
        {0.7f, -0.2, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.5, 1.0f, 0.0f, 0.0f},
    };

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangles), triangles, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Triangle), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Triangle), (void*)sizeof(vec2));
    glEnableVertexAttribArray(1);

    GLProgramGenerator programGen;
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
            precision highp float;
            in vec4 out_Color;
            out vec4 FragColor;
            void main()
            {
                FragColor = out_Color;
            }
        )").AttachAndLink();
    glUseProgram(program);

    GLuint FBO;
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    GLenum buffers[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, buffers);

    int vwidth = 500;
    int vheight = 500;
    glViewport(0, 0, vwidth, vheight);

    GLuint Tex;
    glGenTextures(1, &Tex);
    glBindTexture(GL_TEXTURE_2D, Tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, vwidth, vheight, 0, GL_RGBA, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // bind texture to fbo
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Tex, 0);
    glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    glClearColor(0.2, 0.1, 0.6, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    float buff[4] = {0};
    glReadBuffer(GL_FRAMEBUFFER);
    glReadPixels(vwidth / 2, vheight / 2, 1, 1, GL_RGBA, GL_FLOAT, buff);
    std::cout << "central pixel is : " << buff[0] << "," << buff[1] << "," << buff[2] << "," << buff[3] << std::endl;


    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    /* create a second surface */
    auto helper2  = X11EglHelper();
    helper2.InitWindow(500, 500, "test egl", glVersion);
    glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    return 0;
}