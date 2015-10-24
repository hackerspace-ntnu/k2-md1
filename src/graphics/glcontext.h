#ifndef GLCONTEXT_INCLUDE
#define GLCONTEXT_INCLUDE

#include "../general/fileio.h"
#include "deps/flextGL/flextGL.h"

namespace KineBot
{
namespace GL
{
static GLuint create_program(const char* vshader, const char* fshader)
{
    char* vsrc = KineBot::read_text_file(vshader);
    char* fsrc = KineBot::read_text_file(fshader);

    GLuint vsh = glCreateShader(GL_VERTEX_SHADER);
    GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);

    {
        const char* v = vsrc;
        const char* f = fsrc;

        glShaderSource(vsh,1,&v,NULL);
        glShaderSource(fsh,1,&f,NULL);
    }

    glCompileShader(vsh);
    glCompileShader(fsh);

    GLuint program = glCreateProgram();

    glAttachShader(program,vsh);
    glAttachShader(program,fsh);

    glLinkProgram(program);

    glDetachShader(program,vsh);
    glDetachShader(program,fsh);

    glDeleteShader(vsh);
    glDeleteShader(fsh);

    return program;
}


}
}

#endif
