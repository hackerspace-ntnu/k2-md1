#ifndef GLCONTEXT_INCLUDE
#define GLCONTEXT_INCLUDE

#include "../general/fileio.h"
#include "deps/flextGL/flextGL.h"

namespace KineBot
{
namespace GL
{
/*!
 * \brief Create a GL shader program, complete with compilation and linking from shader source files.
 * \param vshader Path to a valid vertex shader file
 * \param fshader Path to a valid fragment shader file
 * \return A handle for a GL shader program
 */
static GLuint create_program(const char* vshader, const char* fshader)
{
    char* vsrc = KineBot::read_text_file(vshader);
    char* fsrc = KineBot::read_text_file(fshader);

    GLuint vsh = glCreateShader(GL_VERTEX_SHADER);
    GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);

    {
        //We do this to avoid ambiguity with const char**
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

    free(vsrc);
    free(fsrc);

    return program;
}

}
}

#endif
