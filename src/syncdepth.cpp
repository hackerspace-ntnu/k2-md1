#include "wrapper.hpp"
#include "../deps/bat/bat.hpp"
#include <libfreenect2/registration.h>

float depth_pers, color_pers;

int main() {
  int sw = depth_width, sh = depth_height;
  MyScreen screen(sw, sh);
  Surface sf(sw, sh);
  Clock clock;

  MyKinect kinect;
  float*depth;
  Color*col;

  Registration reg(kinect.dev->getIrCameraParams(), kinect.dev->getColorCameraParams());

  //float depth_pers = 365.463; // 361~367 -ish 
  //float color_pers = 1081.37; // 1060~1070 -ish 

  depth_pers = kinect.dev->getIrCameraParams().fx;
  color_pers = kinect.dev->getColorCameraParams().fx;

  float ccx = kinect.dev->getColorCameraParams().cx;
  float ccy = kinect.dev->getColorCameraParams().cy;
  float dcx = kinect.dev->getIrCameraParams().cx;
  float dcy = kinect.dev->getIrCameraParams().cy;

  Frame undistorted(depth_width, depth_height, 4), registered(depth_width, depth_height, 4);

  int show_depth = 0;

  while (1) {
    while (screen.gotEvent()) {
      Event e = screen.getEvent();
      if (e.type == KeyPress and e.key == K_ESCAPE) return 0;
      if (e.type == KeyPress and e.key == K_SPACE) show_depth ^= 1;
    }

    kinect.getColorAndDepth((uint**)&col, &depth);

    reg.apply(kinect.frames[Frame::Color], kinect.frames[Frame::Depth], &undistorted, &registered);
    col = (Color*)registered.data;

    for (int j = 0; j < depth_height; j++) {
      for (int i = 0; i < depth_width; i++) {
	if (col[i+j*depth_width])
	  sf.pixels[i+sf.w*j] = col[i+j*depth_width];
	//else 
	//  sf.pixels[i+sf.w*j] = 0;
	continue;
	float d = depth[i+depth_width*j];
	if (!d) continue;

	/*float x = i-dcx, y = j-dcy;
	//x += 55*depth_pers/d; //55mm -ish between cameras
	int ix = x*color_pers/depth_pers+ccx;
	int iy = y*color_pers/depth_pers+ccy;*/
	float cx, cy;
	reg.apply(i, j, d, cx, cy);
	int ix = cx, iy = cy;
	
	if (ix >= 0 and iy >= 0 and ix < color_width and iy < color_height)
	  sf.pixels[i+sw*j] = col[ix+iy*color_width];
      }
    }

    float r = 300., px = 200, py = -100, pz = 1100;
    for (int i = 0; i < depth_width; i++) {
      for (int j = 0; j < depth_height; j++) {
	float dx = i-depth_width*.5f, dy = j-depth_height*.5f, dz = depth_pers;
	float pp = px*px+py*py+pz*pz;
	float dd = dx*dx+dy*dy+dz*dz;
	float dp = dx*px+dy*py+dz*pz;
	float sq = dp*dp+(r*r-pp)*dd;
	if (sq < 0) continue;
	float t = (dp-sqrt(sq))/dd;
	float nx = dx*t-px, ny = dy*t-py, nz = dz*t-pz;
	if (dz*t < depth[i+j*depth_width] or !depth[i+j*depth_width]) {
	  depth[i+j*depth_width] = dz*t;
	  float diff = (-nz*3+nx+ny*.5)*.35/r;
	  if (diff < 0) diff = 0;
	  if (diff > 1) diff = 1;
	  sf.pixels[i+j*sw] = int(diff*255);
	}
      }
    }
    if (show_depth) {
      for (int i = 0; i < depth_width*depth_height; i++) {
	float d = int(256*5-(depth[i]-500)/3);
	if (d >= 256*5) sf.pixels[i] = 0;
	else if (d < 256)
	  sf.pixels[i] = d;
	else if (d < 256*2)
	  sf.pixels[i] = Color(0, d-256, 255);
	else if (d < 256*3)
	  sf.pixels[i] = Color(0, 255, 256*3-1-d);
	else if (d < 256*4)
	  sf.pixels[i] = Color(d-256*3, 255, 0);
	else if (d < 256*5)
	  sf.pixels[i] = Color(255, 256*5-1-d, 0);
	else 
	  sf.pixels[i] = Color(255, 255, 255);
	  
      }
    }
    screen.putSurface(sf);
    clock.tick(30);
    clock.print();
  }
}
