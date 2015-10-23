#include "glfw_types.h"

namespace KineBot{
namespace GLFW{

GLFWContext::GLFWContext()
{
    if(!glfwInit())
        throw std::runtime_error("Failed to create GLFW context!");

    glfwDefaultWindowHints();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
    glfwWindowHint(GLFW_OPENGL_API,GLFW_OPENGL_ES_API);

    window = glfwCreateWindow(640,480,"KinectBot!",NULL,NULL);
    glfwMakeContextCurrent(window);
}

GLFWContext::~GLFWContext()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

}
}
