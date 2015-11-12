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
extern GLuint create_program(const char* vshader, const char* fshader);

struct KineTexture
{
    KineTexture();
    ~KineTexture();

    GLuint handle;
    int w;
    int h;
    unsigned char* data;
};

extern KineTexture* load_texture(const char* filename);

extern void bind_texture(const KineTexture* t, int unit = 0);

}
}

#endif
