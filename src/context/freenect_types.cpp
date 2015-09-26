#include "freenect_types.h"

namespace KineBot
{
FreenectContext::FreenectContext() :
    device(NULL)
{
    int devices = manager.enumerateDevices();
    if(devices <= 0)
        throw std::runtime_error("No devices detected! Cannot continue!");
    fprintf(stderr,"Number of Kinect 2 devices detected: %i\n",devices);
    device = manager.openDefaultDevice();
    device->start();
}

FreenectContext::~FreenectContext()
{
    device->stop();
    device->close();
}

bool FreenectListener::onNewFrame(libfreenect2::Frame::Type type, libfreenect2::Frame *frame)
{
	return true;
}

}
