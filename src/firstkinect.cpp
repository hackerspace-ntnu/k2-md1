#include <iostream>

#include "context/freenect_types.h"

using namespace libfreenect2;

int main(int argv, char** argc) try {
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
    return 1;
}
