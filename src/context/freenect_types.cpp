#include "freenect_types.h"

KineBot::FreenectContext::FreenectContext() :
    device(NULL),
    listener(NULL)
{
    int devices = manager.enumerateDevices();
    fprintf(stderr,"Number of Kinect 2 devices detected: %i",devices);
    device = manager.openDefaultDevice();
}

KineBot::FreenectContext::~FreenectContext()
{
    device->close();
}
