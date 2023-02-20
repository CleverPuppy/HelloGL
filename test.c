#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

void process_input(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GL_TRUE);
  }
}

GLuint drawATriangle() {
  GLuint VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  float vertices[] = {-0.5f, -0.5f, 0.0f, 0.5, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f};

  unsigned int VBO;
  glGenBuffers(1, &VBO);              // create buffer
  glBindBuffer(GL_ARRAY_BUFFER, VBO); // bind

  // copy user-defined data to current bound gl buffer object
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 3 * sizeof(float) , (void*)0);
  glEnableVertexAttribArray(0);

  return VAO;
}

GLuint drawRectangle() {
  GLuint VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);
  
  GLuint VBO;
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  float vertices[] = {
    -0.5f, 0.5f, 0.0f, // top-left
    0.5f, 0.5f, 0.0f,  // top-right
    0.5f, -0.5f, 0.0f, // bottom-right,
    -0.5f, -0.5f, 0.0f, // bottom-left
  };
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  GLuint EBO; // element array buffer
  glGenBuffers(1, &EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  unsigned int  indices[] = {
    0,1,2,
    1,3,0,
  }; // indices of current bound VBO
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
  glEnableVertexAttribArray(0);

  return VAO;
}

unsigned int setupVertexShader() {
  const char *vertexShaderSource =
      "#version 330 core\n"
      "layout (location = 0) in vec3 aPos;\n"
      "void main()\n"
      "{\n"
      "  vec4 offset = vec4(sin(gl_InstanceID), cos(gl_InstanceID), 0, 0);"
      "  gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0) + offset;\n"
      "}\0";

  unsigned int vertexShader = 0;
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  int success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    fprintf(stderr, "Compile Vertex Shader failed. Info : %s", infoLog);
  }

  return vertexShader;
}

unsigned int setupFragmentShader() {
    const char* fragmentShaderSource = 
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "  FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
        "}\0";
    
    unsigned int fragmentShader = 0;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    int success;
    char infoLog[512];
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        fprintf(stderr, "Compile Fragment Shader failed : %s", infoLog);
    }

    return fragmentShader;
}

void setupProgram(int vertex_shader, int fragment_shader)
{
  unsigned int program = 0;
  program = glCreateProgram();

  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  // check if link success;
  int success = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  char buffer[512];
  if(!success)
  {
    glGetProgramInfoLog(program, 512, NULL, buffer);
    fprintf(stderr, "Link Program Error  : %s\n", buffer);
  }

  glUseProgram(program);

  glDeleteProgram(vertex_shader);
  glDeleteShader(fragment_shader);
}

int main() {
  if (glfwInit() == GL_FALSE) {
    fprintf(stderr, "glfwInit failed");
    return -1;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // for mac only

  GLFWwindow *window = glfwCreateWindow(800, 600, "test_window", NULL, NULL);
  if (!window) {
    fprintf(stderr, "Failed to create glfwWindow.");
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    fprintf(stderr, "glad failed to init");
    glfwTerminate();
    return -1;
  }

  // window resize callback
  glViewport(0, 0, 800, 600);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // setup opengl program
  GLuint vertex_shader = setupVertexShader();
  GLuint fragment_shader = setupFragmentShader();
  setupProgram(vertex_shader, fragment_shader);

  // get data
  // GLuint VAO = drawATriangle();
  GLuint VAO = drawRectangle();

  // main loop
  while (!glfwWindowShouldClose(window)) {
    // handle input
    process_input(window);

    // do rendering
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(VAO);
    // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, 6);
    glBindVertexArray(0);
    // swap buffer and check and call events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}