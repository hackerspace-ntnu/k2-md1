#include "glfw_types.h"

#include "glcontext.h"

namespace KineBot{
namespace GLFW{

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
}

GLFWContext::~GLFWContext()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

}
}
