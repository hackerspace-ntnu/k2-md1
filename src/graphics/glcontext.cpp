#include "glcontext.h"

#define STB_IMAGE_IMPLEMENTATION
#include "deps/stb/stb_image.h"

GLuint KineBot::GL::create_program(const char *vshader, const char *fshader)
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

KineBot::GL::KineTexture *KineBot::GL::load_texture(const char *filename)
{
    KineTexture* texture = new KineTexture;
    glGenTextures(1,&texture->handle);

    FILE* tfile = fopen(filename,"rb");

    texture->data = stbi_load_from_file(tfile,&texture->w,&texture->h,NULL,4);

    fclose(tfile);

    glBindTexture(GL_TEXTURE_2D,texture->handle);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,texture->w,texture->h,0,GL_RGBA,GL_UNSIGNED_BYTE,texture->data);

    glBindTexture(GL_TEXTURE_2D,0);

    return texture;
}

KineBot::GL::KineTexture::KineTexture():
    handle(0),
    w(0),
    h(0),
    data(NULL)
{
}

KineBot::GL::KineTexture::~KineTexture()
{
    if(this->data)
        free(this->data);
    if(this->handle!=0)
        glDeleteTextures(1,&this->handle);
}

void KineBot::GL::bind_texture(const KineBot::GL::KineTexture *t, int unit)
{
    glActiveTexture(GL_TEXTURE0+unit);
    glBindTexture(GL_TEXTURE_2D,t->handle);
}
