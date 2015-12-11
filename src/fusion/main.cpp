#include <iostream>
#include <bat.hpp>
#include <clock.hpp>
using namespace std;

#include "scale_tsdf.hpp"

const int TSDF::size[] = {8, 8, 4};

#include "fusion.hpp"
#include "raycast.hpp"

int main() {
	TSDF sphere;
	//sphere.initSphere();
	int sw = 800, sh = 600;
	MyScreen screen(sw, sh);
	Surface sf(sw, sh);
	Clock clock;
	clock.print = 1;

	float*zbuf = new float[sw*sh];

	Projection camera(sw*.5-.5, sh*.5-.5, 600);

	float rotmat[9];
	for (int i = 0; i < 12; i++) rotmat[i] = i%4==0;
	float px = .48, py = .51, pz = -.51;

	float t = 0;

	int fuse = 0;
	while (1) {
		while (screen.gotEvent()) {
			int e = screen.getEvent();
			if (e%16 == KeyPress and e/16 == K_ESCAPE) return 0;
			else if (e%16 == KeyPress and e/16 == K_SPACE) fuse = 1;
		}

		for (int i = 0; i < sw*sh; i++) sf.pixels[i] = 0;

		t += .3;
		rotmat[0] = cos(t);
		rotmat[2] =-sin(t);
		rotmat[6] = sin(t);
		rotmat[8] = cos(t);

		px = sin(t)+.5;
		pz = -cos(t)+.5;

		for (int j = 0; j < sh; j++) {
			for (int i = 0; i < sw; i++) {
				float dx1 = i-camera.cx, dy1 = j-camera.cy, dz1 = camera.fx;
				float dx = dx1*rotmat[0]+dy1*rotmat[1]+dz1*rotmat[2];
				float dy = dx1*rotmat[3]+dy1*rotmat[4]+dz1*rotmat[5];
				float dz = dx1*rotmat[6]+dy1*rotmat[7]+dz1*rotmat[8];
				float t = raycast(sphere, px, py, pz, dx, dy, dz);
				if (t) {
					sf.pixels[i+j*sw] = (1.5-t*camera.fx)*255;
				}

				if (fuse)
					zbuf[i+j*sw] = idealRaycast(px, py, pz, dx, dy, dz);
			}
		}

		if (fuse)
			recfuse(sphere, zbuf, sw, sh, rotmat, camera, px, py, pz);

		fuse = 0;

		screen.putSurface(sf);
		clock.tick(30);
	}
}
