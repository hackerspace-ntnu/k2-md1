#include "context/freenect_types.h"
#include "graphics/glcontext.h"
#include "graphics/glfw_types.h"

#define STB_IMAGE_IMPLEMENTATION
#include "deps/stb/stb_image.h"
#include <malloc.h>
#include <string.h>

#include "deps/glm/glm/glm.hpp"
#include "deps/glm/glm/gtc/type_ptr.hpp"
#include "deps/glm/glm/gtc/matrix_transform.hpp"

int main(int argv, char** argc) try {
    KineBot::GLFW::GLFWContext gctxt;
    glfwShowWindow(gctxt.window);
    glfwMakeContextCurrent(gctxt.window);

    if(!gladLoadGLES2Loader(get_exts))
        throw std::runtime_error("Failed to load Glad!");

    glfwSwapInterval(0);

//    KineBot::FreenectContext ctxt;

//    libfreenect2::SyncMultiFrameListener listener(libfreenect2::Frame::Depth|libfreenect2::Frame::Color);
//    ctxt.device->setColorFrameListener(&listener);
//    ctxt.device->setIrAndDepthFrameListener(&listener);
//    ctxt.device->start();

//    libfreenect2::FrameMap frames;

    glClearColor(1.f,0.f,0.f,1.f);

    int w,h,dep;
    FILE* depthtext = fopen("depthtexture.jpg","r");
    unsigned char* img = stbi_load_from_file(depthtext,&w,&h,&dep,3);

    char* vsrc = KineBot::GLFW::read_text_file("shaders/vertex.vsh");
    char* gsrc = KineBot::GLFW::read_text_file("shaders/geometry.gsh");
    char* fsrc = KineBot::GLFW::read_text_file("shaders/fragment.fsh");

    GLuint vsh = glCreateShader(GL_VERTEX_SHADER);
    GLuint gsh = glCreateShader(GL_GEOMETRY_SHADER_OES);
    GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);

    {
        const char* v = vsrc;
        const char* g = gsrc;
        const char* f = fsrc;

        glShaderSource(vsh,1,&v,NULL);
        glShaderSource(fsh,1,&f,NULL);
        glShaderSource(gsh,1,&g,NULL);
    }

    glCompileShader(vsh);
    glCompileShader(gsh);
    glCompileShader(fsh);

    GLuint program = glCreateProgram();

    glAttachShader(program,vsh);
    glAttachShader(program,gsh);
    glAttachShader(program,fsh);

    glLinkProgram(program);

    glUseProgram(program);

    GLuint texture;
    glGenTextures(1,&texture);

    GLuint vertices;
    glGenBuffers(1,&vertices);

    GLuint vao;
    glGenVertexArrays(1,&vao);

    int coord_w = 1080,coord_h = 1920;
    size_t coord_size = 1920*1080*sizeof(glm::vec2);
    GLfloat* coords = (GLfloat*)malloc(coord_size);

    for(int x=0;x<coord_w;x++)
        for(int y=0;y<coord_h;y++)
        {
            glm::vec2 v(x,y);
            memcpy(&coords[y*coord_w+x],&v,sizeof(glm::vec2));
        }

    glBindBuffer(GL_ARRAY_BUFFER,vertices);

    glBindVertexArray(vao);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(glm::vec2),(void*)0);

    glBufferData(GL_ARRAY_BUFFER,coord_size,coords,GL_STATIC_DRAW);

    glBindTexture(GL_TEXTURE_2D,texture);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);

    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,img);

    glm::vec3 pos(0,0,-10);

    glm::mat4 cam = glm::perspective(90.f,4.f/3.f,0.1f,100.f);
    cam = glm::scale(cam,glm::vec3(1,1,1));
    cam = glm::translate(cam,pos);

    glUniformMatrix4fv(glGetUniformLocation(program,"transform"),1,GL_FALSE,(GLfloat*)&cam);
    glUniform3fv(glGetUniformLocation(program,"camera_pos"),1,(GLfloat*)&pos);

    double frametime = glfwGetTime();
    libfreenect2::Frame *d;
    while(!glfwWindowShouldClose(gctxt.window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

//        listener.waitForNewFrame(frames);
//        d = frames[libfreenect2::Frame::Depth];

//        glad_glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,d->width,d->height,0,GL_RGB8,GL_UNSIGNED_BYTE,d->data);

        glDrawArrays(GL_POINTS,0,1920*1080);

        glfwPollEvents();
        glfwSwapBuffers(gctxt.window);

//        listener.release(frames);
        fprintf(stderr,"Frames: %f\n",1.0/(glfwGetTime()-frametime));
        frametime = glfwGetTime();
    }

    glad_glDeleteTextures(1,&texture);
    return 0;
}
catch(std::exception v)
{
    fprintf(stderr,"--------------------------\n"
                   "Whoops! %s\n",v.what());
    return 1;
}
