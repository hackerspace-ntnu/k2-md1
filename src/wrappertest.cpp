#include "wrapper.h"
#include "../deps/bat/bat.hpp"

int main() {
	int sw = 800, sh = 600;
	MyScreen screen(sw, sh);
	Surface sf(sw, sh);
	Clock clock;

	MyKinect kinect;
	float*depth;
	Color*col;

	while (1) {
		while (screen.gotEvent()) {
			Event e = screen.getEvent();
			if (e.type == KeyPress and e.key == K_ESCAPE) return 0;
		}

		kinect.getColorAndDepth(col, depth);
		
		for (int j = 0; j < sh; j++) 
			for (int i = 0; i < sw; i++)
				sf.pixels[i+sf.w*j] = col[i+kinect.color_width*j];

		screen.putSurface(sf);
		clock.tick(30);
	}
}
