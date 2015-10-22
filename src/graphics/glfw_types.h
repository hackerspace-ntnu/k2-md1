#ifndef GLFW_TYPES
#define GLFW_TYPES

#include <stdexcept>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace KineBot{
namespace GLFW{

struct GLFWContext
{
    GLFWContext();
    ~GLFWContext();

    GLFWwindow* window;
};

}
}

#endif
