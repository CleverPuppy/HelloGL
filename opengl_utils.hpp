#pragma once

#include <functional>
#ifdef USE_GLAD
#include "glad/glad.h"
#endif

#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#endif

#ifdef __linux__
#include <X11/Xlib.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "unistd.h"
#endif

#include <array>
#include <vector>
#include <cstdio>
#include <functional>
#include <iostream>
#include <string>
#include <sstream>
#include <cassert>

#define Assert(x) assert(x)

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

enum class API_TYPE
{
    OGL,
    GLES
};

class GLProgramVersion
{
public:
    GLProgramVersion(API_TYPE eType, int major, int minor):
        eType(eType), major(major), minor(minor) {
        std::stringstream ss;
        ss << "#version " << GetGLSLVersion() <<
            (eType == API_TYPE::OGL ? " core" : " es") << "\n";
        strGLSLVersionMacroLine = ss.str();
    }

    const API_TYPE eType;
    const int major;
    const int minor;

    const std::string& GLSLVersionMacroLine() const{ return strGLSLVersionMacroLine; }
private:
    int GetGLSLVersion() { return major * 100 + minor * 10; }
    std::string strGLSLVersionMacroLine;
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

  inline GLProgramGenerator& AppendShader(const GLProgramVersion &glver, GLenum ShaderType,
      const std::string &ShaderSrc)
  {
      std::string strAppendVersionSrc = glver.GLSLVersionMacroLine() + ShaderSrc;
      return AppendShader(ShaderType, strAppendVersionSrc);
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

class XHelper
{
  public:
    virtual bool InitWindow(int width, int height, const std::string& windowTitle,
        const GLProgramVersion& glVersion) = 0;
    virtual void Render(std::function<void()>&& xFunc) = 0;
};

#ifdef USE_GLFW
class GLFWHelper : public XHelper{
public:
  GLFWHelper() = default;
  ~GLFWHelper() = default;

  virtual bool InitWindow(int width, int height, const std::string& windowTitle,
      const GLProgramVersion& glVersion) override
  {
      return InitWindow(width, height, windowTitle,
          glVersion.eType,
          glVersion.major,
          glVersion.minor);
  }

  inline bool InitWindow(int width, int height, const std::string &windowTitle,
      API_TYPE type, int major, int minor) {
    this->width = width;
    this->height = height;

    if (glfwInit() == GL_FALSE) {
      std::cerr << "glfwInit failed." << std::endl;
      return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
    if (type == API_TYPE::OGL)
    {
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    }
    else
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    }

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // for mac only
#endif

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

  virtual void Render(std::function<void()> &&funcRender) override {
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

#ifdef __linux__

class EglAttrBuilder
{
public:
    EglAttrBuilder(): bFinalised(false) {}
    ~EglAttrBuilder() = default;

    inline void AppendAttr(EGLint param, EGLint value)
    {
        if(bFinalised){
            Assert(0 && "AppendAttr in finalized attr.");
            return;
        }

        data.push_back(param);
        data.push_back(value);
    }

    inline EGLint* GetAttr()
    {
        if(!bFinalised)
        {
            makeFinalize();
        }
        return data.data();
    }

private:
    bool bFinalised;
    std::vector<EGLint> data;

    inline void makeFinalize()
    {
        if(bFinalised) return;
        data.push_back(EGL_NONE);
        bFinalised = true;
    }
};

struct X11Window
{
    Display* dpy;
    Window wnd;
    Atom WM_PROTOCOLS;
    Atom WM_DELETE_WINDOW;
};

struct EglInfo
{
    EGLConfig config;
    EGLDisplay *eglDisplay;
    EGLSurface *eglSurface;
    EGLContext *eglContext;
    int vsync;
};

static X11Window InitX11(uint width, uint height, const char* strWindowName)
{
    Display* dpy = XOpenDisplay(NULL);
    Assert(dpy && "Cannot open X display");

    XSetWindowAttributes attributes =
    {
        .event_mask = StructureNotifyMask,
    };

    // create window

    Window window = XCreateWindow(
        dpy, DefaultRootWindow(dpy),
        0, 0, width, height,
        0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask,
        &attributes);
    Assert(window && "Failed to create window");

    // uncomment in case you want fixed size window
    //XSizeHints* hints = XAllocSizeHints();
    //Assert(hints);
    //hints->flags |= PMinSize | PMaxSize;
    //hints->min_width  = hints->max_width  = width;
    //hints->min_height = hints->max_height = height;
    //XSetWMNormalHints(dpy, window, hints);
    //XFree(hints);

    // set window title
    XStoreName(dpy, window, strWindowName);

    // subscribe to window close notification
    Atom WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
    Atom WM_DELETE_WINDOW = XInternAtom(dpy , "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, window, &WM_DELETE_WINDOW, 1);

    X11Window ret;
    ret.wnd = window;
    ret.dpy = dpy;
    ret.WM_DELETE_WINDOW = WM_DELETE_WINDOW;
    ret.WM_PROTOCOLS = WM_PROTOCOLS;
    return ret;
}

static EglInfo InitEgl(const X11Window& x11Window, API_TYPE type, int major, int minor)
{
    std::cout << "Creating EGL " << (type == API_TYPE::OGL ? "GL" : " GLES") << std::endl;

    Display *dpy = x11Window.dpy;
    // initialize EGL
    EGLDisplay* display;
    {
        display = (EGLDisplay*)eglGetDisplay((EGLNativeDisplayType)dpy);
        Assert(display != EGL_NO_DISPLAY && "Failed to get EGL display");

        EGLint major, minor;
        if (!eglInitialize(display, &major, &minor))
        {
            Assert(0 && "Cannot initialize EGL display");
        }
        if (major < 1 || (major == 1 && minor < 5))
        {
            Assert(0 && "EGL version 1.5 or higher required");
        }
    }

    // Bind API
    EGLBoolean ok = eglBindAPI( (type == API_TYPE::OGL ?  EGL_OPENGL_API : EGL_OPENGL_ES_API));
    Assert(ok && "Failed to eglBindAPI for EGL");

    // choose EGL configuration
    EGLConfig config;
    {
        EglAttrBuilder attrBuilder;

        attrBuilder.AppendAttr( EGL_SURFACE_TYPE, EGL_WINDOW_BIT );
        if(type == API_TYPE::OGL)
        {
            attrBuilder.AppendAttr(EGL_CONFORMANT, EGL_OPENGL_BIT);
            attrBuilder.AppendAttr(EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT);
        }
        else
        {
            attrBuilder.AppendAttr(EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT);
        }
        attrBuilder.AppendAttr(EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER);
        attrBuilder.AppendAttr(EGL_RED_SIZE, 8);
        attrBuilder.AppendAttr(EGL_GREEN_SIZE, 8);
        attrBuilder.AppendAttr(EGL_BLUE_SIZE, 8);
        attrBuilder.AppendAttr(EGL_DEPTH_SIZE, 24);
        attrBuilder.AppendAttr(EGL_STENCIL_SIZE, 8);
        //     // uncomment for multisampled framebuffer
        //     //EGL_SAMPLE_BUFFERS, 1,
        //     //EGL_SAMPLES,        4, // 4x MSAA

        EGLint count;
        if (!eglChooseConfig(display, attrBuilder.GetAttr(), &config, 1, &count) || count != 1)
        {
            Assert(0 && "Cannot choose EGL config");
        }
    }

    // create EGL surface
    EGLSurface* surface;
    {
        EglAttrBuilder attrBuilder;
        attrBuilder.AppendAttr(EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_LINEAR);
        attrBuilder.AppendAttr(EGL_RENDER_BUFFER, EGL_BACK_BUFFER);

        // EGLint attr[] =
        // {
        //     EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_LINEAR, // or use EGL_GL_COLORSPACE_SRGB for sRGB framebuffer
        //     EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
        //     EGL_NONE,
        // };

        surface = (EGLSurface*)eglCreateWindowSurface(display, config, x11Window.wnd, attrBuilder.GetAttr());
        if (surface == EGL_NO_SURFACE)
        {
            Assert(0 && "Cannot create EGL surface");
        }
    }

    // create EGL context
    EGLContext* context;
    {
        EglAttrBuilder attrBuilder;
        attrBuilder.AppendAttr(EGL_CONTEXT_MAJOR_VERSION, major);
        attrBuilder.AppendAttr(EGL_CONTEXT_MINOR_VERSION, minor);
        if(type == API_TYPE::OGL)
        {
            attrBuilder.AppendAttr(EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT);
        }
#ifndef NDEBUG
        attrBuilder.AppendAttr(EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE);
#endif

        context = (EGLContext*)eglCreateContext(display, config, EGL_NO_CONTEXT, attrBuilder.GetAttr());
        Assert(context != EGL_NO_CONTEXT && "Cannot create EGL context, Version not supported?");
    }

    ok = eglMakeCurrent(display, surface, surface, context);
    Assert(ok && "Failed to make context current");

    // use 0 to disable vsync
    int vsync = 1;
    ok = eglSwapInterval(display, vsync);
    Assert(ok && "Failed to set vsync for EGL");

    EglInfo ret;
    ret.config = config;
    ret.eglSurface = surface;
    ret.eglDisplay = display;
    ret.eglContext = context;
    ret.vsync = vsync;

    return ret;
}

class X11EglHelper: public XHelper
{
public:
  X11EglHelper() = default;

  virtual bool InitWindow(int width, int height, const std::string& windowTitle,
      const GLProgramVersion& glVersion) override
  {
    x11Window = InitX11(width, height, windowTitle.c_str());
    eglInfo = InitEgl(x11Window, glVersion.eType, glVersion.major, glVersion.minor);
#ifdef USE_GLAD
    if (!gladLoadGLLoader((GLADloadproc)eglGetProcAddress)) {
      std::cerr << "gladLoadGLLoader failed." << std::endl;
      eglTerminate(eglInfo.eglDisplay);
      return false;
    }
#endif
    return true;
  }

  virtual void Render(std::function<void()>&& xFunc) override
  {
    DoCommonRender(std::forward<std::function<void()>&&>(xFunc));
  }

  X11Window x11Window;
  EglInfo eglInfo;
private:
  void DoCommonRender(std::function<void()>&& xRenderFunc)
  {
      Display *dpy = x11Window.dpy;
      Window window = x11Window.wnd;
      EGLSurface *surface = eglInfo.eglSurface;
      EGLDisplay *display = eglInfo.eglDisplay;
      int vsync = eglInfo.vsync;

      // show the window
      XMapWindow(dpy, window);
      for (;;)
      {
          // process all incoming X11 events
          if (XPending(dpy))
          {
              XEvent event;
              XNextEvent(dpy, &event);
              if (event.type == ClientMessage)
              {
                  if (event.xclient.message_type == x11Window.WM_PROTOCOLS)
                  {
                      Atom protocol = event.xclient.data.l[0];
                      if (protocol == x11Window.WM_DELETE_WINDOW)
                      {
                          // window closed, exit the for loop
                          break;
                      }
                  }
              }
              continue;
          }

          // get current window size
          XWindowAttributes attr;
          Status status = XGetWindowAttributes(dpy, window, &attr);
          Assert(status && "Failed to get window attributes");

          int width = attr.width;
          int height = attr.height;

          // render only if window size is non-zero
          if (width != 0 && height != 0)
          {
              xRenderFunc();

              // swap the buffers to show output
              if (!eglSwapBuffers(display, surface))
              {
                  Assert(0 && "Failed to swap OpenGL buffers!");
              }
          }
          else
          {
              // window is minimized, cannot vsync - instead sleep a bit
              if (vsync)
              {
                  usleep(10 * 1000);
              }
          }
      }
  }
};

class SurfaceLessHelper
{
public:
  SurfaceLessHelper() = default;

  bool Init(const GLProgramVersion& glVersion) {
    bool ok = InitEglSurfaceless(glVersion.eType, glVersion.major, glVersion.minor);
    if(!ok) {
      std::cerr << "InitEglSurfaceless failed." << std::endl;
      return false;
    }

    #ifdef USE_GLAD
    if (!gladLoadGLLoader((GLADloadproc)eglGetProcAddress)) {
      std::cerr << "gladLoadGLLoader failed." << std::endl;
      eglTerminate(display);
      return false;
    }
#endif
    return true;
  }

private:
  EGLDisplay* display;
  EGLConfig config;
  EGLContext* context;

  bool InitEglSurfaceless(API_TYPE type, int major, int minor) {
    std::cout << "Creating EGL Surfaceless " << (type == API_TYPE::OGL ? "GL" : " GLES") << std::endl;

    // initialize EGL
    {
        display = (EGLDisplay*)eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA,
                    EGL_DEFAULT_DISPLAY, NULL);
        Assert(display != EGL_NO_DISPLAY && "Failed to get EGL display");

        EGLint major, minor;
        if (!eglInitialize(display, &major, &minor))
        {
            Assert(0 && "Cannot initialize EGL display");
        }
        if (major < 1 || (major == 1 && minor < 5))
        {
            Assert(0 && "EGL version 1.5 or higher required");
        }
    }
    EGLBoolean ok = EGL_TRUE;
    // Bind API
    // ok = eglBindAPI( (type == API_TYPE::OGL ?  EGL_OPENGL_API : EGL_OPENGL_ES_API));
    // Assert(ok && "Failed to eglBindAPI for EGL");

    // choose EGL configuration
    {
        EglAttrBuilder attrBuilder;

        // attrBuilder.AppendAttr( EGL_SURFACE_TYPE, EGL_WINDOW_BIT );
        if(type == API_TYPE::OGL)
        {
            attrBuilder.AppendAttr(EGL_CONFORMANT, EGL_OPENGL_BIT);
            attrBuilder.AppendAttr(EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT);
        }
        else
        {
            attrBuilder.AppendAttr(EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT);
        }

        EGLint count;
        if (!eglChooseConfig(display, attrBuilder.GetAttr(), &config, 1, &count))
        {
            Assert(0 && "Cannot choose EGL config");
        }
    }

    // create EGL context
    {
        EglAttrBuilder attrBuilder;
        attrBuilder.AppendAttr(EGL_CONTEXT_MAJOR_VERSION, major);
        attrBuilder.AppendAttr(EGL_CONTEXT_MINOR_VERSION, minor);
        if(type == API_TYPE::OGL)
        {
            attrBuilder.AppendAttr(EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT);
        }
#ifndef NDEBUG
        attrBuilder.AppendAttr(EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE);
#endif

        context = (EGLContext*)eglCreateContext(display, config, EGL_NO_CONTEXT, attrBuilder.GetAttr());
        Assert(context != EGL_NO_CONTEXT && "Cannot create EGL context, Version not supported?");
    }

    ok = eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);
    Assert(ok && "Failed to make context current");

    return true;
  }
};

#endif