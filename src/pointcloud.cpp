#include "wrapper.hpp"
#include "../deps/bat/bat.hpp"
#include <libfreenect2/registration.h>
#include "../deps/bat/vec3.hpp"

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


	vec3 pos(0, 0, 0);
	int lx = -1, ly = -1;
	double dx = 0, dy = 0, dz = 0;

  while (1) {
    while (screen.gotEvent()) {
      Event e = screen.getEvent();
      if (e.type == KeyPress and e.key == K_ESCAPE) return 0;
 				else if (key == K_q) dy = 1;
				else if (key == K_e) dy =-1;
				else if (key == K_w) dz = 1;
				else if (key == K_s) dz =-1;
				else if (key == K_d) dx = 1;
				else if (key == K_a) dx =-1;
		} else if (type == KeyRelease) {
			if (key == K_e or key == K_q) dy = 0;
			if (key == K_w or key == K_s) dz = 0;
			if (key == K_a or key == K_d) dx = 0;
		} else if (type == ButtonPress) {
			screen.getCursorPos(&lx, &ly);
		} else if (type == ButtonRelease)
			lx = -1;
	}
	if (lx != -1) {
		int mx, my;
		screen.getCursorPos(&mx, &my);
		screen.setCursorPos(sw/2, sh/2);
		viewdir.rotatePlane(up, (mx-lx)*.2);

		vec3 side = up^viewdir;
		up.rotatePlane(side, (ly-my)*.2);
		viewdir.rotatePlane(side, (ly-my)*.2);
		lx = sw/2;
		ly = sh/2;
	}
	vec3 side = up^viewdir;
	
	float speed = .03;
	pos += side*dx*speed;
	pos += up*dy*speed;
	pos += viewdir*dz*speed;


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
