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

static glm::quat mouse_rotation;
static glm::vec3 camera_pos;

void glfw_fb_resize(GLFWwindow*,int w,int h)
{
    //This resizes the GL viewport automatically on a callback
    glViewport(0,0,w,h);
}

void glfw_mouse_move(GLFWwindow*,double x,double y)
{
    mouse_rotation.w = 2;
    mouse_rotation.y = x*0.01;
    mouse_rotation.x = y*0.01;
}

void glfw_keyboard_event(GLFWwindow* win,int key,int/*scancode*/,int action,int/*modifier*/)
{
    if(action&GLFW_REPEAT|GLFW_PRESS)
    {
        switch(key)
        {
        case GLFW_KEY_KP_4:{
            camera_pos += glm::vec3(-0.1,0,0);
            break;
        }
        case GLFW_KEY_KP_6:{
            camera_pos += glm::vec3(0.1,0,0);
            break;
        }
        case GLFW_KEY_KP_2:{
            camera_pos += glm::vec3(0,0,-0.1);
            break;
        }
        case GLFW_KEY_KP_8:{
            camera_pos += glm::vec3(0,0,0.1);
            break;
        }
        case GLFW_KEY_UP:{
            camera_pos += glm::vec3(0,0.1,0);
            break;
        }
        case GLFW_KEY_DOWN:{
            camera_pos += glm::vec3(0,-0.1,0);
            break;
        }
        case GLFW_KEY_KP_9:{
            mouse_rotation = glm::normalize(glm::quat(2,0,0.1,0)*mouse_rotation);
            break;
        }
        case GLFW_KEY_KP_3:{
            mouse_rotation = glm::normalize(glm::quat(2,0,-0.1,0)*mouse_rotation);
            break;
        }
        case GLFW_KEY_ESCAPE:{
            glfwSetWindowShouldClose(win,1);
            break;
        }
        }
    }
}

int main(int argv, char** argc) try {
    KineBot::GLFW::GLFWContext gctxt(1280,720,"KinectBot");
    glfwSwapInterval(0);
//    glfwSetInputMode(gctxt.window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
    glfwSetFramebufferSizeCallback(gctxt.window,glfw_fb_resize);
//    glfwSetCursorPosCallback(gctxt.window,glfw_mouse_move);
    glfwSetKeyCallback(gctxt.window,glfw_keyboard_event);

    if(!flextInit(gctxt.window))
        throw std::runtime_error("Failed to load Glad!");


//    KineBot::FreenectContext ctxt;

//    libfreenect2::SyncMultiFrameListener listener(libfreenect2::Frame::Depth|libfreenect2::Frame::Color);
//    ctxt.device->setColorFrameListener(&listener);
//    ctxt.device->setIrAndDepthFrameListener(&listener);
//    ctxt.device->start();

//    libfreenect2::FrameMap frames;

//    listener.waitForNewFrame(frames);
//    d = frames[libfreenect2::Frame::Depth];
//    listener.release(frames);

    glClearColor(0.0f,0.f,0.f,1.f);

    //Load sample texture from file
    int w,h,dep;
    FILE* depthtext = fopen("depthtexture.jpg","r");
    unsigned char* img = stbi_load_from_file(depthtext,&w,&h,&dep,3);

    GLuint program = KineBot::GL::create_program("shaders/vertex.vsh","shaders/fragment.fsh");

    glUseProgram(program);

    //Create resources
    GLuint texture;
    glGenTextures(1,&texture);

    GLuint vertexbuffers[2];
    glGenBuffers(2,vertexbuffers);

    GLuint vao;
    glGenVertexArrays(1,&vao);

    //Create some data to use as vertex data
    int coord_w = 1920,coord_h = 1080;
    size_t coord_size = coord_w*coord_h*sizeof(glm::vec2);
    glm::vec2* coords = (glm::vec2*)malloc(coord_size);
    glm::vec2* texcoords = (glm::vec2*)malloc(coord_size);

    for(int y=0;y<coord_h;y++)
        for(int x=0;x<coord_w;x++)
        {
            glm::vec2 v(x/50.0,y/50.0);
            glm::vec2 t((float)x/(float)coord_w,(float)y/(float)coord_h);
            memcpy(&coords[y*coord_w+x],&v,sizeof(glm::vec2));
            memcpy(&texcoords[y*coord_w+x],&t,sizeof(glm::vec2));
        }

    glBindVertexArray(vao);

    //Fill vertex buffer with vertex positions, screen-space-ish
    glBindBuffer(GL_ARRAY_BUFFER,vertexbuffers[0]);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(glm::vec2),(void*)0);

    glBufferData(GL_ARRAY_BUFFER,coord_size,coords,GL_STATIC_DRAW);

    //Fill vertex buffer with texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER,vertexbuffers[1]);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(glm::vec2),(void*)0);

    glBufferData(GL_ARRAY_BUFFER,coord_size,texcoords,GL_STATIC_DRAW);

    //TODO: Add uniform buffer for matrices

    //Bind and load sample texture
    glBindTexture(GL_TEXTURE_2D,texture);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);

    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,img);

    //Define uniforms
    glm::vec3 pos(-20,-15,-15);
    glm::quat rot(2,0,0,0);

    glm::mat4 cam = glm::perspective(60.f,8.f/5.f,0.1f,100.f);
    cam = glm::scale(cam,glm::vec3(1,1,1));
    glm::mat4 cam_ready = glm::translate(cam,pos)*glm::mat4_cast(rot);

    GLuint transform_uniform = glGetUniformLocation(program,"transform");

    glUniformMatrix4fv(transform_uniform,1,GL_FALSE,(GLfloat*)&cam);
    glUniform1i(glGetUniformLocation(program,"depthtex"),0);

    glfwShowWindow(gctxt.window);
    double frametime = glfwGetTime();
    libfreenect2::Frame *d;
    while(!glfwWindowShouldClose(gctxt.window))
    {
        glClear(GL_COLOR_BUFFER_BIT);
//        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,img);

        rot = mouse_rotation;
        cam_ready = glm::translate(cam,camera_pos)*glm::mat4_cast(rot);
        glUniformMatrix4fv(transform_uniform,1,GL_FALSE,(GLfloat*)&cam_ready);

        glDrawArrays(GL_POINTS,0,1920*1080);

        //Event handling goes below here
        glfwPollEvents();
        glfwSwapBuffers(gctxt.window);

//        fprintf(stderr,"Frames: %f\n",1.0/(glfwGetTime()-frametime));
        frametime = glfwGetTime();

    }

    glDeleteTextures(1,&texture);
    return 0;
}
catch(std::exception v)
{
    fprintf(stderr,"--------------------------\n"
                   "Whoops! %s\n",v.what());
    return 1;
}
