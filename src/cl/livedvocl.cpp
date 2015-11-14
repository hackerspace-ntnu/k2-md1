#include "myregister.hpp"
#include "cldvo.hpp"
#include "../../deps/bat/bat.hpp"
#include "../../deps/bat/vec3.hpp"
#include <cmath>

void drawSphere(Surface&sf, float*&depth, float px, float py, float pz, float r, float pers) {
  for (int i = 0; i < sf.w; i++) {
    for (int j = 0; j < sf.h; j++) {
      float dx = i-(sf.w-1)*.5f, dy = j-(sf.h-1)*.5f, dz = pers;
      float pp = px*px+py*py+pz*pz;
      float dd = dx*dx+dy*dy+dz*dz;
      float dp = dx*px+dy*py+dz*pz;
      float sq = dp*dp+(r*r-pp)*dd;
      if (sq < 0) continue;
      float t = (dp-sqrt(sq))/dd;
      float nx = dx*t-px, ny = dy*t-py, nz = dz*t-pz;
      if (dz*t < depth[i+j*sf.w]) {
	depth[i+j*sf.w] = dz*t;
	float diff = (-nz*3+nx+ny*.5)*.25/r;
	if (diff < .2) diff = .2;
	if (diff > 1) diff = 1;
	sf.pixels[i+j*sf.w] = int(diff*255)*0x10101;
      }
    }
  }
}

void irotate(float*rotmat, float*x, float*r) {
  float px = x[0]-rotmat[3];
  float py = x[1]-rotmat[7];
  float pz = x[2]-rotmat[11];
  r[0] = rotmat[0]*px+rotmat[4]*py+rotmat[8]*pz;
  r[1] = rotmat[1]*px+rotmat[5]*py+rotmat[9]*pz;
  r[2] = rotmat[2]*px+rotmat[6]*py+rotmat[10]*pz;
}

int main() {
  int sw = 1920/2, sh = 1080/2;
  float color_pers = 1081/2.;
  
  MyScreen screen(sw, sh);
  Surface sf(sw, sh);
  float*zbuf = new float[sw*sh];
  Clock clock;

  MyKinect kinect;
  float*depth;
  Color*col;

  MyRegistration reg(kinect, sw, sh);
  Tracker tracker(sw, sh);
  
  float spherepos[3] = {0, 0, 1000}, newspherepos[3];
  byte*intensity = new byte[sw*sh];
  
  while (1) {
    while (screen.gotEvent()) {
      Event e = screen.getEvent();
      int key = e.key, type = e.type;
      if (e.type == KeyPress and e.key == K_ESCAPE) return 0;
      else if (e.type == ButtonPress) {
	int mx, my;
	screen.getCursorPos(mx, my);
	float z = zbuf[mx+my*sw];
	spherepos[0] = (mx-sw*.5)*z/color_pers;
	spherepos[1] = (my-sh*.5)*z/color_pers;
	spherepos[2] = z;
	tracker.reset();
      }
    }

    kinect.getColorAndDepth((uint**)&col, &depth);
    reg.projectDepth(zbuf, depth);

    for (int j = 0; j < sh; j++) {
      for (int i = 0; i < sw; i++) {
	int sum = 0;
	for (int l = 0; l < 2; l++) {
	  for (int k = 0; k < 2; k++) {
	    Color&c = col[i*2+k+(j*2+l)*1920];
	    sum += c.r+c.g+c.b;
	  }
	}
	intensity[i+j*sw] = sum/12;
      }
    }
    tracker.update(intensity, zbuf);
    
    for (int i = 0; i < sw*sh; i++)
      sf.pixels[i] = intensity[i]*0x10101;

    irotate(tracker.result, spherepos, newspherepos);
    
    drawSphere(sf, zbuf, newspherepos[0], newspherepos[1], newspherepos[2], 100, color_pers);

    screen.putSurface(sf);
    clock.tick(30);
    clock.print();
  }
}
