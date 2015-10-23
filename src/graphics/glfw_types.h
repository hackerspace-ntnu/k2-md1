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
    GLFWContext();
    ~GLFWContext();

    GLFWwindow* window;
};

static char* read_text_file(const char* file)
{
    FILE* fp = fopen(file,"r");
    fseek(fp,0,SEEK_END);
    size_t sz = ftell(fp);
    rewind(fp);

    char* text = (char*)malloc(sz+1);
    fread(text,sizeof(char),sz,fp);
    text[sz] = 0;

    fclose(fp);
    return text;
}

}
}

#endif
