#include "../cl/myregister.hpp"
#include "../../deps/bat/bat.hpp"
#include "dataio.hpp"
#include <cmath>

int main() {
  int sw = 1920/2, sh = 1080/2;
  
  MyScreen screen(sw, sh);
  Surface sf(sw, sh);
  Clock clock;

  MyKinect kinect;
  float*depth;
  Color*col;

  Surface shrink(sw, sh);
  
  int saved = 0;
  while (1) {
    while (screen.gotEvent()) {
      Event e = screen.getEvent();
      int key = e.key, type = e.type;
      if (e.type == KeyPress and e.key == K_ESCAPE) return 0;
    }

    kinect.waitForFrame();
    kinect.getColorAndDepth((uint**)&col, &depth);

    for (int j = 0; j < sh; j++) {
      for (int i = 0; i < sw; i++) {
	int sumr = 0, sumg = 0, sumb = 0;
	for (int l = 0; l < 2; l++) {
	  for (int k = 0; k < 2; k++) {
	    Color&c = col[i*2+k+(j*2+l)*1920];
	    sumr += c.r;
	    sumg += c.g;
	    sumb += c.b;
	  }
	}
	shrink.pixels[i+j*sw].r = sumr/4;
	shrink.pixels[i+j*sw].g = sumg/4;
	shrink.pixels[i+j*sw].b = sumb/4;
      }
    }
    
    screen.putSurface(shrink);

    saveIMG(shrink, sw, sh, saved);
    saveZB(depth, 512, 424, saved);
    saved++;

    clock.tick(30);
    clock.print();
  }
}
