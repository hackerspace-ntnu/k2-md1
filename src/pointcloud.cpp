#include "wrapper.hpp"
#include "../deps/bat/bat.hpp"
#include <libfreenect2/registration.h>

float depth_pers, color_pers;

int main() {
  int sw = 800, sh = 600;
  MyScreen screen(sw, sh);
  Surface sf(sw, sh);
  Clock clock;

  MyKinect kinect;
  float*depth;
  Color*col;

  Registration reg(kinect.dev->getIrCameraParams(), kinect.dev->getColorCameraParams());

  float pers = 600;

  Frame undistorted(depth_width, depth_height, 4), registered(depth_width, depth_height, 4);

  while (1) {
    while (screen.gotEvent()) {
      Event e = screen.getEvent();
      if (e.type == KeyPress and e.key == K_ESCAPE) return 0;
    }

    reg.apply(kinect.frames[Frame::Color], kinect.frames[Frame::Depth], &undistorted, &registered);
    col = (Color*)registered.data;

    for (int j = 0; j < depth_height; j++) {
      for (int i = 0; i < depth_width; i++) {
	float px, py, pz, rgb;
	reg.getPointXYZRGB(undistorted, registered, j, i, px, py, pz, rgb);
	if (rgb == 0) continue;
	Color col = *reinterpret_cast<Color*>(&rgb);
	px -= pos.x;
	py -= pos.y;
	pz -= pos.z;
	float x = side.x*px+side.y*py+side.z*pz;
	float y = up.x*px+up.y*py+up.z*pz;
	float z = viewdir.x*px+viewdir.y*py+viewdir.z*pz;
	int ix = x*pers/z+sw*.5;
	int iy = y*pers/z+sh*.5;
	if (ix >= 0 and iy >= 0 and ix < sw and iy < sh)
	  sf.pixels[i] = col;
      }
    }

    screen.putSurface(sf);
    clock.tick(30);
    clock.print();
  }
}
