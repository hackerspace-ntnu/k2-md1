#include "context/freenect_types.h"
#include "graphics/glcontext.h"
#include "graphics/glfw_types.h"

int main(int argv, char** argc) try {
    KineBot::GLFW::GLFWContext gctxt;
    glfwShowWindow(gctxt.window);
    glfwMakeContextCurrent(gctxt.window);

    if(!gladLoadGL())
        throw std::runtime_error("Failed to load Glad!");

    glad_glClearColor(1.f,0.f,0.f,1.f);
    while(!glfwWindowShouldClose(gctxt.window))
    {
        glad_glClear(GL_COLOR_BUFFER_BIT);
        glfwPollEvents();
        glfwSwapBuffers(gctxt.window);
    }

    glfwHideWindow(gctxt.window);

    KineBot::FreenectContext ctxt;

    //  Freenect2 freenect2;
    //  freenect2.enumerateDevices();
    //  std::string serial = freenect2.getDefaultDeviceSerialNumber();
    //  Freenect2Device *dev = freenect2.openDevice(serial);
    //  SyncMultiFrameListener listener(Frame::Depth|Frame::Color);
    //  dev->setColorFrameListener(&listener);
    //  dev->setIrAndDepthFrameListener(&listener);
    //  dev->start();

    //  FrameMap frames;
    //  while (1) {
    //    listener.waitForNewFrame(frames);
    //    Frame*depth = frames[Frame::Depth];

    //    //unsigned int col = ((unsigned int*)&depth->data[(depth->width*(depth->height/2)+depth->width/2)*depth->bytes_per_pixel])[0];
    //    //cout << (col&255) << ' '<< (col>>8&255) << ' ' << (col>>16&255) << endl;

    //    std::cout << ((float*)&depth->data[(depth->width*(depth->height/2)+depth->width/2)*depth->bytes_per_pixel])[0] << std::endl;

    //    listener.release(frames);
    //  }
    return 0;
}
catch(std::exception v)
{
    fprintf(stderr,"--------------------------\n"
                   "Whoops! %s\n",v.what());
    return 1;
}
