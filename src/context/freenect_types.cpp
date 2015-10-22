#include "freenect_types.h"

KineBot::FreenectContext::FreenectContext() :
    device(NULL)
{
    int devices = manager.enumerateDevices();
    if(devices <= 0)
        throw std::runtime_error("No devices detected! Cannot continue!");
    fprintf(stderr,"Number of Kinect 2 devices detected: %i",devices);
    device = manager.openDefaultDevice();
}

KineBot::FreenectContext::~FreenectContext()
{
    device->close();
}
