#include <iostream>
#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/packet_pipeline.h>

using namespace libfreenect2;

static const int depth_width = 512, depth_height = 424, color_width = 1920, color_height = 1080; // Camera resolutions

struct MyKinect {
  Freenect2 freenect2;
  SyncMultiFrameListener*listener;
  FrameMap frames;
  int gotFrame;
  Freenect2Device*dev;
  MyKinect() {
    listener = new SyncMultiFrameListener(SyncMultiFrameListener(Frame::Depth|Frame::Color));
    freenect2.enumerateDevices();
    std::string serial = freenect2.getDefaultDeviceSerialNumber();
    dev = freenect2.openDevice(serial);
    dev->setColorFrameListener(listener);
    dev->setIrAndDepthFrameListener(listener);
    dev->start();
    gotFrame = 0;
  }
  ~MyKinect() {
    dev->stop();
    dev->close();
    delete listener;
  }
  void getColorAndDepth(unsigned int**col, float**depth) {
    if (gotFrame)
      listener->release(frames);

    listener->waitForNewFrame(frames);
    gotFrame = 1;

    *depth = (float*)frames[Frame::Depth]->data;
    *col = (unsigned int*)frames[Frame::Color]->data;
  }
};
