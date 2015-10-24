#ifndef GLFW_TYPES
#define GLFW_TYPES

#include <stdexcept>
#include <cstdio>
#include <malloc.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace KineBot{
namespace GLFW{

struct GLFWContext
{
    GLFWContext(int w, int h, const char *title);
    ~GLFWContext();

    GLFWwindow* window;
};

}
}

#endif
