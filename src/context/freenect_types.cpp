#include "freenect_types.h"
#include <libfreenect2/logger.h>

namespace KineBot
{
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

    device->start();
}

FreenectContext::~FreenectContext()
{
    delete listener;
    device->stop();
    device->close();
}

bool FreenectListener::onNewFrame(libfreenect2::Frame::Type type, libfreenect2::Frame *frame)
{
    return true;
}

}
