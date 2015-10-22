#include <iostream>

#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>

using namespace libfreenect2;
using namespace std;

int main() {
  Freenect2 freenect2;
  freenect2.enumerateDevices();
  string serial = freenect2.getDefaultDeviceSerialNumber();
  Freenect2Device *dev = freenect2.openDevice(serial);
  SyncMultiFrameListener listener(Frame::Depth|Frame::Color);
  dev->setColorFrameListener(&listener);
  dev->setIrAndDepthFrameListener(&listener);
  dev->start();

  FrameMap frames;
  while (1) {
    listener.waitForNewFrame(frames);
    Frame*depth = frames[Frame::Depth];

    //unsigned int col = ((unsigned int*)&depth->data[(depth->width*(depth->height/2)+depth->width/2)*depth->bytes_per_pixel])[0];
    //cout << (col&255) << ' '<< (col>>8&255) << ' ' << (col>>16&255) << endl;

    cout << ((float*)&depth->data[(depth->width*(depth->height/2)+depth->width/2)*depth->bytes_per_pixel])[0] << endl;

    listener.release(frames);
  }
}
bla