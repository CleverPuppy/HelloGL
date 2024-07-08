#include "opengl_utils.hpp"
#include <cassert>
#include <cstddef>

int main()
{
    GLProgramVersion glVersion{ API_TYPE::OGL, 4, 3 };
    // GLProgramVersion glVersion{ API_TYPE::GLES, 3, 2 };
    GLFWHelper glfwHelper;
    glfwHelper.InitWindow(500, 400, "test", glVersion);

    GLuint VAO, VBO;

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    struct Triangle {
        vec3 pos;
        vec4 pack_data;
    } triangles[3] = {
        {-0.7f, -0.2, 1.0, 1.0f, 0.0f, 0.0f, 100.0f,},
        {0.7f, -0.2, 1.0, 0.0f, 1.0f, 0.0f, 50.0f},
        {0.0f, 0.5, 1.0, 0.0f, 0.0f, 1.0f, 150.0f},
    };

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangles), triangles, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Triangle), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Triangle), (void*)sizeof(vec3));
    glEnableVertexAttribArray(1);

    GLProgramGenerator programGen;
    GLuint program = programGen.AppendShader(glVersion, GL_VERTEX_SHADER, R"(
            #extension GL_ARB_enhanced_layouts: enable
            in layout(location = 0) vec3 position;
            in layout(location = 1) vec4 pack_data;
            in layout(location = 1) vec3 color;
            in layout(location = 1, component = 3) float point_size; 
            out vec3 vColor;

            //vec3 color = pack_data.xyz;
            //float point_size = pack_data.w;

            void main()
            {
                gl_Position = vec4(position, 1.0);
                gl_PointSize = point_size;

                vColor = color;
            }
        )").AppendShader(glVersion, GL_FRAGMENT_SHADER, R"(
            precision highp float;
            in vec3 vColor;
            out vec4 color;
            void main()
            {
                color = vec4(vColor, 1.0);
            }
        )").AttachAndLink();
    assert(program != (GLuint)-1);
    glUseProgram(program);

    glPointSize(100.0);
    glEnable(GL_PROGRAM_POINT_SIZE);

    glfwHelper.Render([&glVersion, program](){
        glClearColor(0.2, 0.3, 0.4, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(program);
        glDrawArrays(GL_POINTS, 0, 3);
    });

    return 0;
}