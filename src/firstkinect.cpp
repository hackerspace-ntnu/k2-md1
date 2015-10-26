#include "graphics/glcontext.h"
#include "graphics/glfw_types.h"
#include "context/freenect_types.h"

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "deps/stb/stb_image.h"
#include <malloc.h>
#include <string.h>
#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/packet_pipeline.h>
#include <libfreenect2/registration.h>
#include <libfreenect2/logger.h>

#include "deps/glm/glm/glm.hpp"
#include "deps/glm/glm/gtc/type_ptr.hpp"
#include "deps/glm/glm/gtc/matrix_transform.hpp"

static glm::quat mouse_rotation;
static glm::vec3 camera_pos;
static float     screen_gamma;

/*!
 * \brief GLFW callback for resizing the GL viewport automatically
 * \param w
 * \param h
 */
void glfw_fb_resize(GLFWwindow*,int w,int h)
{
    glViewport(0,0,w,h);
}

/*!
 * \brief GLFW callback for handling keyboard events
 * \param win
 * \param key
 * \param action
 */
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

        case GLFW_KEY_KP_ADD:{
            screen_gamma += 0.1f;
            break;
        }
        case GLFW_KEY_KP_SUBTRACT:{
            screen_gamma -= 0.1f;
            break;
        }
        }
        if(screen_gamma <= 0.f)
            screen_gamma = 0.1f;
    }
}

int main(int argv, char** argc) try {
    KineBot::GLFW::GLFWContext gctxt(1280,720,"KinectBot");
    glfwSwapInterval(0);
    glfwSetFramebufferSizeCallback(gctxt.window,glfw_fb_resize);
    glfwSetKeyCallback(gctxt.window,glfw_keyboard_event);

	libfreenect2::setGlobalLogger(libfreenect2::createConsoleLogger(libfreenect2::Logger::Debug));
    libfreenect2::Freenect2 manager;
	fprintf(stderr,"Devices connected: %i\n",manager.enumerateDevices());
	libfreenect2::PacketPipeline *pipeline = 0;
	libfreenect2::Freenect2Device* device = 0;
	pipeline = new libfreenect2::CpuPacketPipeline();
	libfreenect2::SyncMultiFrameListener listener(libfreenect2::Frame::Color 
		| libfreenect2::Frame::Depth);
	fprintf(stderr,"Enumerating devices\n");

	std::string serial = manager.getDefaultDeviceSerialNumber();
	printf("Serial: %s\n",serial.c_str());
    device = manager.openDevice(serial);
	device->setColorFrameListener(&listener);
	device->setIrAndDepthFrameListener(&listener);
    device->start();

  	std::cout << "device serial: " << device->getSerialNumber() << std::endl;
  	std::cout << "device firmware: " << device->getFirmwareVersion() << std::endl;


    if(!flextInit(gctxt.window))
        throw std::runtime_error("Failed to load Glad!");

	fprintf(stderr,"Hello\n");

    glClearColor(0.0f,0.f,0.f,1.f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(0x8642); //GL_VERTEX_PROGRAM_POINT_SIZE

    //Load sample texture from file
    int w = 0,h = 0;
    FILE* depthtext = fopen("depthtexture.jpg","r");
    unsigned char* img = stbi_load_from_file(depthtext,&w,&h,NULL,3);
    fclose(depthtext);

    int c_w = 0,c_h = 0;
    FILE* coltext = fopen("colortexture.jpg","r");
    unsigned char* c_img = stbi_load_from_file(coltext,&c_w,&c_h,NULL,3);
    fclose(coltext);

    //Create a shader program
    GLuint program = KineBot::GL::create_program("shaders/vertex.vsh","shaders/fragment.fsh");

    glUseProgram(program);

    //Create resources
    GLuint textures[2];
    glGenTextures(2,textures);

    GLuint vertexbuffers[2];
    glGenBuffers(2,vertexbuffers);

    GLuint vertexarrays[1];
    glGenVertexArrays(1,vertexarrays);

    GLuint transformfeedbacks[1];
    glGenTransformFeedbacks(1,transformfeedbacks);

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

    glBindVertexArray(vertexarrays[0]);

    //Fill vertex buffers and define some vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER,vertexbuffers[0]);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(glm::vec2),(void*)0);

    glBufferData(GL_ARRAY_BUFFER,coord_size,coords,GL_STATIC_DRAW);
    free(coords),

    glBindBuffer(GL_ARRAY_BUFFER,vertexbuffers[1]);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(glm::vec2),(void*)0);

    glBufferData(GL_ARRAY_BUFFER,coord_size,texcoords,GL_STATIC_DRAW);
    free(texcoords);

    //TODO: Add uniform buffer for matrices, we might need it

    //Bind and load sample texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,textures[0]);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);

    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,img);
    free(img);

    //Diffusion texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,textures[1]);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);

    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,c_w,c_h,0,GL_RGB,GL_UNSIGNED_BYTE,c_img);
    free(c_img);

    //Define uniforms
    glm::vec3 pos(0,0,0);
    glm::quat rot(2,0,0,0);

//    glm::mat4 cam = glm::perspective(60.f,8.f/5.f,0.1f,100.f);
	glm::mat4 cam = glm::ortho(0.f,30.f,20.f,0.f,0.f,100.f);
    cam = glm::scale(cam,glm::vec3(1,1,1));
    glm::mat4 cam_ready;

    GLuint transform_uniform = glGetUniformLocation(program,"transform");
    GLuint gamma_uniform = glGetUniformLocation(program,"gamma");

    glUniformMatrix4fv(transform_uniform,1,GL_FALSE,(GLfloat*)&cam);
    glUniform1i(glGetUniformLocation(program,"depthtex"),0);
    glUniform1i(glGetUniformLocation(program,"colortex"),1);


    glfwShowWindow(gctxt.window);
    double frametime = glfwGetTime();
    libfreenect2::Frame *dep = 0;
    libfreenect2::Frame *col = 0;
	libfreenect2::FrameMap frames;
	fprintf(stderr,"Now looping\n");
    while(!glfwWindowShouldClose(gctxt.window))
    {
        glClear(GL_COLOR_BUFFER_BIT);
		
		listener.waitForNewFrame(frames);
		dep = frames[libfreenect2::Frame::Depth];
		col = frames[libfreenect2::Frame::Color];
		if(dep&&col){
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D,textures[0]);
			glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,dep->width,dep->height,0,
				GL_RGBA,GL_UNSIGNED_BYTE,dep->data);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D,textures[1]);
			glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,col->width,col->height,0,
				GL_RGBA,GL_UNSIGNED_BYTE,col->data);
		}
		dep = 0;
		col = 0;

        rot = mouse_rotation;
        cam_ready = glm::translate(cam,camera_pos) * glm::mat4_cast(mouse_rotation);
        glUniformMatrix4fv(transform_uniform,1,GL_FALSE,(GLfloat*)&cam_ready);
        glUniform1f(gamma_uniform,screen_gamma);

        glDrawArrays(GL_POINTS,0,1920*1080);

        //Event handling goes below here
        glfwPollEvents();
        glfwSwapBuffers(gctxt.window);

        fprintf(stderr,"Frames: %f\n",1.0/(glfwGetTime()-frametime));
        frametime = glfwGetTime();
		listener.release(frames);
    }

    glDeleteTextures(2,textures);
    glDeleteBuffers(2,vertexbuffers);
    glDeleteProgram(program);
    glDeleteVertexArrays(1,vertexarrays);
    return 0;
}
catch(std::exception v)
{
    fprintf(stderr,"--------------------------\n"
                   "Whoops! %s\n",v.what());
    return 1;
}
