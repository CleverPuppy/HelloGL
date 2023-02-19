#include <functional>
#ifdef USE_GLAD
#include "glad/glad.h"
#endif

#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#endif

#include <array>
#include <cstdio>
#include <functional>
#include <iostream>
#include <string>

#define __valid_or_ret(val)                                                    \
  if (!valid)                                                                  \
    return val;

struct vec2 {
  float x;
  float y;
};

struct vec3 {
  float x;
  float y;
  float z;
};

struct vec4 {
  float x;
  float y;
  float z;
  float w;
};


class GLProgramGenerator {
  enum SHADER_TYPE { VERTEX = 0, FRAGMENT = 1, MAX_SHADER_SIZE };

public:
  GLProgramGenerator() : valid(true) {
    shader_exists_status.fill(false);
    program = glCreateProgram();
    if (program == 0) {
      std::cerr << "create program failed." << std::endl;
      MakeInvalid();
    }
  }

  inline GLProgramGenerator &AppendShader(GLenum ShaderType,
                                          const std::string &ShaderSrc) {
    __valid_or_ret(*this);

    int nShaderIdx = GetIndexByShaderType(ShaderType);
    if (nShaderIdx == -1) {
      return *this;
    }

    if (ExistShader(nShaderIdx)) {
      std::cerr << "ignore existed shader." << std::endl;
      return *this;
    }

    shaders[nShaderIdx] = glCreateShader(ShaderType);
    if (shaders[nShaderIdx] <= 0) {
      std::cerr << "Create Shader Failed." << std::endl;
      MakeInvalid();
      return *this;
    }

    GLuint nShader = shaders[nShaderIdx];
    MakeExistShader(nShaderIdx);

    const char *strSrc = ShaderSrc.c_str();
    glShaderSource(nShader, 1, &strSrc, NULL);
    glCompileShader(nShader);

    GLint status;
    glGetShaderiv(nShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
      char buff[512];
      int length;
      glGetShaderInfoLog(nShader, sizeof(buff), &length, buff);
      std::cerr << "Compile Shader Failed. Log is " << buff << std::endl;
      MakeInvalid();
    }

    return *this;
  }

  inline GLuint AttachAndLink() {
    __valid_or_ret(-1);

    for (int idx = 0; idx < shader_exists_status.size(); ++idx) {
      bool bExist = shader_exists_status[idx];
      if (!bExist)
        continue;

      GLuint nShader = shaders[idx];
      glAttachShader(program, nShader);
    }

    glLinkProgram(program);
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
      char buff[512];
      int length;
      glGetProgramInfoLog(program, sizeof(buff), &length, buff);
      std::cerr << "Link Program Error: " << buff << std::endl;
      MakeInvalid();
      return -1;
    }

    return program;
  }

private:
  std::array<bool, MAX_SHADER_SIZE> shader_exists_status;
  std::array<GLuint, MAX_SHADER_SIZE> shaders;
  bool valid;
  GLuint program;

  inline int GetIndexByShaderType(GLenum eShaderType) {
    switch (eShaderType) {
    case GL_VERTEX_SHADER:
      return VERTEX;
      break;
    case GL_FRAGMENT_SHADER:
      return FRAGMENT;
      break;
    default:
      std::cerr << "Invalid Enum " << eShaderType << std::endl;
      return -1;
    }
  }

  inline void MakeInvalid() { valid = false; }
  inline bool ExistShader(int idx) { return shader_exists_status[idx]; }
  inline void MakeExistShader(int idx) { shader_exists_status[idx] = true; }
};

enum class ContextType { OpenGL, OpenGLES };

#ifdef USE_GLFW
class GLFWHelper {
public:
  GLFWHelper() = default;
  ~GLFWHelper() = default;

  inline bool InitWindow(int width, int height, const std::string &windowTitle,
                         ContextType type, int major, int minor) {
    this->width = width;
    this->height = height;

    if (glfwInit() == GL_FALSE) {
      std::cerr << "glfwInit failed." << std::endl;
      return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // for mac only

    window =
        glfwCreateWindow(width, height, windowTitle.c_str(), nullptr, nullptr);
    if (!window) {
      std::cerr << "glfwCreateWindow failed." << std::endl;
      glfwTerminate();
      return false;
    }

    glfwMakeContextCurrent(window);

#ifdef USE_GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
      std::cerr << "gladLoadGLLoader failed." << std::endl;
      glfwTerminate();
      return false;
    }
#endif

    /* setup callbacks */
    glfwSetFramebufferSizeCallback(this->window,
                                   GLFWHelper::FramebufferSizeCallback);

    return true;
  }

  inline void Render(std::function<void()> &&funcRender) {
    while (!glfwWindowShouldClose(window)) {
      // handle input
      GLFWHelper::process_input(window);

      // do rendering
      funcRender();

      // swap buffers
      glfwSwapBuffers(window);
      glfwPollEvents();
    }
    glfwTerminate();
  }

  static void FramebufferSizeCallback(GLFWwindow *window, int width,
                                      int height) {
    glViewport(0, 0, width, height);
  }

  static void process_input(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, GL_TRUE);
    }
  }

  int width;
  int height;
  GLFWwindow *window;
};
#endif