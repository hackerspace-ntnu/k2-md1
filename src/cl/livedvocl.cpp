#include "myregister.hpp"
#include "cldvo.hpp"
#include "../../deps/bat/bat.hpp"
#include "../../deps/bat/vec3.hpp"
#include <cmath>

void drawSphere(Surface&sf, float*&depth, float px, float py, float pz, float r, float pers) {
  for (int i = 0; i < sf.w; i++) {
    for (int j = 0; j < sf.h; j++) {
      float dx = i-sf.w*.5f, dy = j-sf.h*.5f, dz = pers;
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

int main() {
	int sw = 1920/2, sh = 1080/2;
  MyScreen screen(sw, sh);
  Surface sf(sw, sh);
  float*zbuf = new float[sw*sh];
  float*zbuf2 = new float[sw*sh];
  Clock clock;
	
	float spherepos[3] = {0, 0, 1e9}, newspherepos[3];
	
	float color_pers = 1081/2;

	while (1) {
		//for (int i = 0; i < sw*sh; i++)
    //  sf.pixels[i] = col[i];

    drawSphere(sf, zbuf, spherepos[0], spherepos[1], spherepos[2], 100, color_pers);

    screen.putSurface(sf);
    clock.tick(30);
    clock.print();
	}
}
