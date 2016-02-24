#include "cl/myregister.hpp"
#include "cl/cldvo.hpp"
#include "bat/bat.hpp"
#include "bat/vec3.hpp"
#include <cmath>

void irotate(float*rotmat, float*x, float*r) {
  float px = x[0]-rotmat[3];
  float py = x[1]-rotmat[7];
  float pz = x[2]-rotmat[11];
  r[0] = rotmat[0]*px+rotmat[4]*py+rotmat[8]*pz;
  r[1] = rotmat[1]*px+rotmat[5]*py+rotmat[9]*pz;
  r[2] = rotmat[2]*px+rotmat[6]*py+rotmat[10]*pz;
}

struct Point {
  Color col;
  float x, y, z;
  Point() {}
  Point(float x_, float y_, float z_, Color&col) : x(x_),y(y_),z(z_),col(col) {}
};

void renderPoints(Surface&sf, float*zbuf, Point*point, int points, vec3 pos, vec3 side, vec3 up, vec3 viewdir, float pers) {
  for (int i = 0; i < sf.w*sf.h; i++) {
    sf.pixels[i] = 0x503080;
    zbuf[i] = 1e9;
  }

  float ccx = (sf.w-1)*.5, ccy = (sf.h-1)*.5;
  for (int i = 0; i < points; i++) {
    float px = point[i].x, py = point[i].y, pz = point[i].z;

    px -= pos.x;
    py -= pos.y;
    pz -= pos.z;
    float x = side.x*px+side.y*py+side.z*pz;
    float y = up.x*px+up.y*py+up.z*pz;
    float z = viewdir.x*px+viewdir.y*py+viewdir.z*pz;
    int ix = x*pers/z+ccx;
    int iy = y*pers/z+ccy;
    if (z > 0 and ix >= 0 and iy >= 0 and ix < sf.w and iy < sf.h and zbuf[ix+iy*sf.w] > z) {
      zbuf[ix+iy*sf.w] = z;
      sf.pixels[ix+iy*sf.w] = point[i].col;
    }
  }
}

int main() {
  int sw = 800, sh = 600;
  int Iw = 1920/2, Ih = 1080/2;
  float pers = 600;
  
  MyScreen screen(sw, sh);
  Surface sf(sw, sh);
  float*zbuf = new float[sw*sh];
  Clock clock;
  
  MyKinect kinect;
  float*depth;
  Color*col;

  MyRegistration reg(kinect, Iw, Ih);
  Tracker tracker(Iw, Ih);
  byte*intensity = new byte[Iw*Ih];
  float*track_zbuf = new float[Iw*Ih];

  
  vec3 pos(0, 0, 0);
  int lx = -1, ly = -1;
  double dx = 0, dy = 0, dz = 0;


  int maxpoints = 1000000;
  Point*point = new Point[maxpoints];
  int points = 0;
  int fase = 0;
  while (1) {
    while (screen.gotEvent()) {
      Event e = screen.getEvent();
      int key = e.key, type = e.type;
      if (e.type == KeyPress) {
	if (e.key == K_ESCAPE) return 0;
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
	screen.getCursorPos(lx, ly);
      } else if (type == ButtonRelease)
	lx = -1;
    }
    if (lx != -1) {
      int mx, my;
      screen.getCursorPos(mx, my);
      screen.setCursorPos(sw/2, sh/2);
      viewdir.rotatePlane(up, (mx-lx)*.2);

      vec3 side = up^viewdir;
      up.rotatePlane(side, (ly-my)*.2);
      viewdir.rotatePlane(side, (ly-my)*.2);
      lx = sw/2;
      ly = sh/2;
    }
    vec3 side = up^viewdir;
	
    float speed = 30;
    pos += side*dx*speed;
    pos += up*dy*speed;
    pos += viewdir*dz*speed;




    kinect.getColorAndDepth((uint**)&col, &depth);
    if (!kinect.gotFrame) {
      clock.tick(30);
      cout << "skip" << endl;
      continue;
    }
    
    reg.projectDepth(track_zbuf, depth);

    for (int j = 0; j < Ih; j++) {
      for (int i = 0; i < Iw; i++) {
	int sum = 0;
	for (int l = 0; l < 2; l++) {
	  for (int k = 0; k < 2; k++) {
	    Color&c = col[i*2+k+(j*2+l)*1920];
	    sum += c.r+c.g+c.b;
	  }
	}
	intensity[i+j*Iw] = sum/12;
      }
    }
    tracker.update(intensity, track_zbuf);

    //cout << tracker.counter1 << ' ' << tracker.counter2 << "    " << endl;
    float ipers = 1.f/tracker.Ifx, ccx = tracker.Icx, ccy = tracker.Icy;
    float*rot = tracker.result;
    fase++;
    for (int j = fase%4; j < Ih; j+=4) {
      for (int i = fase/4%4; i < Iw; i+=4) {
	float pz = track_zbuf[i+j*Iw];
	if (pz >= 1e9) continue;
	float px = (i-ccx)*pz*ipers;
	float py = (j-ccy)*pz*ipers;

	Point&cur = point[points];
	cur.x = rot[0]*px+rot[1]*py+rot[2]*pz+rot[3];
	cur.y = rot[4]*px+rot[5]*py+rot[6]*pz+rot[7];
	cur.z = rot[8]*px+rot[9]*py+rot[10]*pz+rot[11];
  
	cur.col = col[i+j*1920<<1];
	points++;
	if (points >= maxpoints) {
	  points -= maxpoints;
	}
      }
    }
    renderPoints(sf, zbuf, point, maxpoints, pos, side, up, viewdir, pers);
    
    screen.putSurface(sf);
    clock.tick(30);
    //clock.print();
  }
}
