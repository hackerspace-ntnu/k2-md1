#include "glfw_types.h"

#include "glcontext.h"

namespace KineBot{
namespace GLFW{

/*!
 * \brief GLFW callback for resizing the GL viewport automatically
 * \param w
 * \param h
 */
void glfw_fb_resize(GLFWwindow*,int w,int h)
{
    glViewport(0,0,w,h);
}

GLFWContext::GLFWContext(int w, int h, const char* title)
{
    if(!glfwInit())
        throw std::runtime_error("Failed to create GLFW context!");

    glfwDefaultWindowHints();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_API,GLFW_OPENGL_ES_API);

    window = glfwCreateWindow(w,h,title,NULL,NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    if(!flextInit(window))
        throw std::runtime_error("Failed to load flext!");

    glfwSetFramebufferSizeCallback(window,glfw_fb_resize);
}

GLFWContext::~GLFWContext()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

}
}
