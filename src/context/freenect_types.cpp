#include "freenect_types.h"
#include <libfreenect2/logger.h>

#include <thread>
#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/registration.h>

#include <stdexcept>
#include <atomic>

#include <stdio.h>
#include <future>

#include <string>
#include <mutex>

namespace KineBot
{

void super_register_points(const libfreenect2::Registration* reg, libfreenect2::Frame* dframe, )
{

}

struct FreenectContext
{
    FreenectContext();
    ~FreenectContext();

    libfreenect2::Freenect2 manager;
    libfreenect2::Freenect2Device* device;

    libfreenect2::SyncMultiFrameListener* listener;

    libfreenect2::FrameMap frames;

    libfreenect2::Registration *reg;

    std::mutex frame_mutex;

    libfreenect2::Frame** kframes;

    std::atomic_bool active;
    std::atomic_bool new_frame;

    std::future<void> exit_fun;
};

FreenectContext::FreenectContext() :
    device(NULL)
{
    libfreenect2::setGlobalLogger(
                libfreenect2::createConsoleLogger(libfreenect2::Logger::Debug));
    int devices = manager.enumerateDevices();
    if(devices <= 0)
        throw std::runtime_error("No devices detected! Cannot continue!");
    fprintf(stderr,"Number of Kinect 2 devices detected: %i\n",devices);
    std::string serial = manager.getDefaultDeviceSerialNumber();
    fprintf(stderr,"Serial of device: %s\n",serial.c_str());
    device = manager.openDevice(serial);
    listener = new libfreenect2::SyncMultiFrameListener(libfreenect2::Frame::Color|libfreenect2::Frame::Depth);

    device->setColorFrameListener(listener);
    device->setIrAndDepthFrameListener(listener);

    reg = new libfreenect2::Registration(device->getIrCameraParams(),device->getColorCameraParams());

    device->start();
}

FreenectContext::~FreenectContext()
{
    delete reg;
    delete listener;
    device->stop();
    device->close();
}

FreenectContext *freenect_alloc()
{
    return new FreenectContext;
}

void freenect_process_frame(FreenectContext *context, FreenectFrameProcessFunction fun, int numFrames)
{
    if(!context->active.load())
        return;
    for(size_t i=0;i<numFrames;i++)
        if(!context->kframes[i])
            return;

    if(context->new_frame.load()){
        context->frame_mutex.lock();


        fun(context->kframes,numFrames);

        context->new_frame.store(false);
        context->frame_mutex.unlock();

        //We launch an async task to delete the frames, using a mutex to ensure we don't stomp on other data
        std::async(std::launch::async,[=](){
            context->frame_mutex.lock();
            context->listener->release(context->frames);
            context->frame_mutex.unlock();
        });
    }
}

void freenect_launch_async(FreenectContext *kctxt)
{
    kctxt->kframes = (libfreenect2::Frame**)calloc(2,sizeof(libfreenect2::Frame*));
    kctxt->active.store(true);

    kctxt->exit_fun = std::async(std::launch::async,[=](){
        fprintf(stderr,"Starting Kinect loop!\n");
        while(kctxt->active.load())
        {
            kctxt->listener->waitForNewFrame(kctxt->frames);
            kctxt->frame_mutex.lock();
            kctxt->kframes[0] = (kctxt->frames)[libfreenect2::Frame::Depth];
            kctxt->kframes[1] = (kctxt->frames)[libfreenect2::Frame::Color];
            kctxt->new_frame.store(true);
            fprintf(stderr,"Created new frame\n");
            kctxt->frame_mutex.unlock();
        }
    });
}

void freenect_exit_async(FreenectContext *kctxt)
{
    kctxt->active.store(false);
    kctxt->exit_fun.get();
}

void freenect_free(FreenectContext *kctxt)
{
    delete kctxt;
}

}
