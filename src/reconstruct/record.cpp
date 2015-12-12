#include "../cl/myregister.hpp"
#include "../../deps/bat/bat.hpp"
#include "dataio.hpp"
#include <cmath>

int main() {
  int sw = 1920/2, sh = 1080/2;
  float color_pers = 1081/2.f;
  
  MyScreen screen(sw, sh);
  Surface sf(sw, sh);
  float*zbuf = new float[sw*sh];
  Clock clock;

  MyKinect kinect;
  float*depth;
  Color*col;

  MyRegistration reg(kinect, sw, sh);

  byte*intensity = new byte[sw*sh];
  
  int saved = 0;
  while (1) {
    while (screen.gotEvent()) {
      Event e = screen.getEvent();
      int key = e.key, type = e.type;
      if (e.type == KeyPress and e.key == K_ESCAPE) return 0;
    }

    kinect.waitForFrame();
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
    
    for (int i = 0; i < sw*sh; i++)
      sf.pixels[i] = intensity[i]*0x10101;

    screen.putSurface(sf);

    saveZB(zbuf, sw, sh, saved);
    saveIMG(intensity, sw, sh, saved);
    saved++;

    clock.tick(30);
    clock.print();
  }
}
