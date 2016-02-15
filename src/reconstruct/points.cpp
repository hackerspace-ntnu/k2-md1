#include <iostream>

#include "../../deps/bat/bat.hpp"
#include "trackball.hpp"
//#include "ssao.hpp"

using namespace std;

#include "dataio.hpp"

struct Point {
  Color col;
  float x, y, z;
  Point() {}
  Point(float x_, float y_, float z_, Color&col) : x(x_),y(y_),z(z_),col(col) {}
};

int showall = 0, showframe = 0, frame = 0;

void renderPoints(Surface&sf, float*zbuf, Point*point, int points, vec3 pos, vec3 side, vec3 up, vec3 viewdir, float&pers) {
  for (int i = 0; i < sf.w*sf.h; i++) {
    sf.pixels[i] = 0x503080;
    zbuf[i] = 1e9;
  }

	float ccx = sf.w*.5-.5, ccy = sf.h*.5-.5;
	int inc = !showall*19+1;
  for (int i = 0; i < points; i += inc) {
    float px = point[i].x, py = point[i].y, pz = point[i].z;

    px -= pos.x;
    py -= pos.y;
    pz -= pos.z;
    float x = side.x*px+side.y*py+side.z*pz;
    float y = up.x*px+up.y*py+up.z*pz;
    float z = viewdir.x*px+viewdir.y*py+viewdir.z*pz, iz = 1.f/z;
    int ix = x*pers*iz+ccx;
    int iy = y*pers*iz+ccy;
    if (z > 0 and ix >= 0 and iy >= 0 and ix < sf.w and iy < sf.h and zbuf[ix+iy*sf.w] > z) {
      zbuf[ix+iy*sf.w] = z;
      sf.pixels[ix+iy*sf.w] = point[i].col;
    }
  }
}

int main() {
	int sw = 1920/2, sh = 1080/2;
	int Iw = 1920/2, Ih = 1080/2;
	int Dw = 512, Dh = 424;
	MyScreen screen(sw, sh);
	Surface sf(sw, sh);
	Clock clock;

	float*zbuf = new float[sw*sh];
	byte*I = new byte[Iw*Ih];

	float ccx = Iw*.5-.5, ccy = Ih*.5-.5, pers = 1081.37f/2;
	float Dcx = 255.559, Dcy = 207.671, Dpers = 365.463;

	float rotmat[9];
	float px, py, pz;

	FILE*fp = fopen("teamviews.txt", "r");
	
	float ifx = 1.f/Dpers;
	int points = 0;
	Point*point = new Point[300000000];
	int pstart[478+1];
	for (int i = 0; i < 478; i++) {
		float dx, dy, dz;
		fscanf(fp, "%f%f%f", &dx, &dy, &dz);
		for (int j = 0; j < 9; j++)
			fscanf(fp, "%f", &rotmat[j]);
		
		//if (i != 0 and i != 70) continue;

		//cout << dx << ' ' << dy << ' ' << dz << endl;
		//for (int k = 0; k < 9; k++) cout << rotmat[k] << endl;

		loadZB(zbuf, Dw, Dh, i);
		//loadIMG(I, Iw, Ih, i);

		pstart[i] = points;
		for (int l = 0; l < Dh; l++) {
			for (int k = 0; k < Dw; k++) {
				float tz = zbuf[k+l*Dw];
				//if (tz < 500 && tz > 10) cout << tz << endl;
				if (tz > 1e8) continue;
				float tx = (k-Dcx)*tz*ifx, ty = (l-Dcy)*tz*ifx;
				float x = tx*rotmat[0]+ty*rotmat[1]+tz*rotmat[2]+dx*1000.f;
				float y = tx*rotmat[3]+ty*rotmat[4]+tz*rotmat[5]+dy*1000.f;
				float z = tx*rotmat[6]+ty*rotmat[7]+tz*rotmat[8]+dz*1000.f;
				Color col = 0xffffff;//I[k+l*Iw]*0x10101;
				if (rand()%10 == 0) 
				point[points++] = Point(x, y, z, col);
			}
		}
	}
	pstart[478] = points;
	//cout << points << endl;
	fclose(fp);

	MyTrackball track(screen);
	track.speed = 30;
	
	while (1) {
		while (screen.gotEvent()) {
      Event e = screen.getEvent();
			if (e.type == KeyPress and e.key == K_ESCAPE) return 0;
			else if (e.type == KeyPress) {
				if (e.key == K_SPACE) showall ^= 1;
				else if (e.key == K_TAB) showframe ^= 1;
				else if (e.key == K_LEFT) frame -= 1;
				else if (e.key == K_RIGHT) frame += 1;
			}
			track.handleEvent(e);
    }
		track.update();
		vec3 side = up^viewdir, pos = track.pos;

		if (showframe) {
			frame = (frame+478)%478;
			renderPoints(sf, zbuf, point+pstart[frame], pstart[frame+1]-pstart[frame], pos, side, up, viewdir, pers);
		} else
			renderPoints(sf, zbuf, point, points, pos, side, up, viewdir, pers);
		//for (int i = 0; i < sw*sh; i += 2)
		//	sf.pixels[i] = I[i]*0x10101;
		screen.putSurface(sf);
		clock.tick(30);
	}
}
