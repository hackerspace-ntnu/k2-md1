#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>

using namespace libfreenect2;
using namespace std;

struct MyKinect {
	SyncMultiFrameListener listener;
	FrameMap frames;
	const int depth_width = 512, depth_height = 424, color_width = 1920, color_height = 1080; // Camera resolutions
	MyKinect() {
		Freenect2 freenect2;
		freenect2.enumerateDevices();
		string serial = freenect2.getDefaultDeviceSerialNumber();
		Freenect2Device*dev = freenect2.openDevice(serial);
		listener(Frame::Depth|Frame::Color);
		dev->setColorFrameListener(&listener);
		dev->setIrAndDepthFrameListener(&listener);
		dev->start();
		frames = NULL;
	}
	void getColorAndDepth(Color*&col, float*&depth) {
		if (frames)
			listener.release(frames), frames = NULL;

		listener.waitForNewFrame(frames);
		depth = (float*)frames[Frame::Depth]->data;
		col = (Color*)frames[Frame::Color]->data;
	}
};
