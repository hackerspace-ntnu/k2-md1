#include "wrapper.hpp"
#include "../deps/bat/bat.hpp"
#include <libfreenect2/registration.h>
#include "../deps/bat/vec3.hpp"

int sw = 1920, sh = 1080;
inline void innerloop(int&j, float&x1, float&f1, float&x2, float&f2, float z0, float&zfx, float*&zbuf) {
  for (int i = x1+j*f1; i < x2+j*f2; i++) {
    float z = z0+i*zfx;
    if (z < zbuf[i+j*sw]) {
      zbuf[i+j*sw] = z;
    }
  }
}

void cross(float ax, float ay, float az, float bx, float by, float bz, float&nx, float&ny, float&nz) {
  nx = ay*bz-az*by;
  ny = az*bx-ax*bz;
  nz = ax*by-ay*bx;
}
//Assumes
//ay < by, cy
//bx < cx
void renderTri(float*&zbuf, float&ax, float&ay, float&az, float&bx, float&by, float&bz, float&cx, float&cy, float&cz) {
  //if (ay > by or ay > cy or bx < cx) return;
  
  float ab = (bx-ax)/(by-ay), ac = (cx-ax)/(cy-ay), bc = (cx-bx)/(cy-by);
  float x1 = ax-ay*ac+1-1e-6, f1 = ac, x2 = ax-ay*ab-1e-6, f2 = ab;
  
  float nx, ny, nz;
  cross(ax-bx, ay-by, az-bz, ax-cx, ay-cy, az-cz, nx, ny, nz);
  float na = nx*ax+ny*by+nz*bz, inz = 1./nz;
  nx *= -inz;
  ny *= -inz;
  nz = na*inz;

  //if (nx*nx > 100 or ny*ny > 100) return;

  float ly = min(by, cy), ly2 = max(by, cy);
  for (int j = ceil(ay); j < ly; j++) 
    innerloop(j, x1, f1, x2, f2, nz+j*ny, nx, zbuf);
  if (by < cy) {
    x2 = bx-by*bc;
    f2 = bc;
  } else {
    x1 = bx-by*bc;
    f1 = bc;
  }
  for (int j = ceil(ly); j < ly2; j++) 
    innerloop(j, x1, f1, x2, f2, nz+j*ny, nx, zbuf);
}

void getDepth(float*zbuf, float*lastx, float*lasty, float*thisx, float*thisy, float*depth, Registration&reg) {
  for (int j = 0; j < depth_height; j++) {
    for (int i = 0; i < depth_width; i++) {
      float&z = depth[i+j*512];
      reg.apply(i, j, z, thisx[i], thisy[i]);
      if (thisx[i] < 0 or thisx[i] >= sw or thisy[i] < 0 or thisy[i] >= sh) z = 0;
      if (!i or !j) continue;
      float&z1 = depth[i-1+j*512-512];
      float&z2 = depth[i+j*512-512];
      float&z3 = depth[i-1+j*512];
      float&z4 = depth[i+j*512];
      if (!z1 or !z2 or !z3 or !z4) continue;
      float mi = min(min(z1, z2), min(z3, z4));
      float ma = max(max(z1, z2), max(z3, z4));
      if (ma-mi > 100) continue;
      renderTri(zbuf, 
		lastx[i-1], lasty[i-1], z1, 
		thisx[i], thisy[i], z4, 
		thisx[i-1], thisy[i-1], z3);
      renderTri(zbuf, 
		lastx[i-1], lasty[i-1], z1, 
		lastx[i], lasty[i], z2, 
		thisx[i], thisy[i], z4);
    }
    swap(lastx, thisx);
    swap(lasty, thisy);
  }
}

void store(Color*col, float*zbuf) {
  FILE*fcol = fopen("dump.ppm", "w");
  fprintf(fcol, "P6\n%d %d 255\n", color_width, color_height);
  for (int i = 0; i < color_width*color_height; i++)
    fputc(col[i].r, fcol), fputc(col[i].g, fcol), fputc(col[i].b, fcol);
  fclose(fcol);
  FILE*fz = fopen("zbufdump.txt", "w");
  fprintf(fz, "%d %d\n", color_width, color_height);
  for (int i = 0; i < color_width*color_height; i++)
    fprintf(fz, "%f ", zbuf[i]);
  fclose(fz);
}

int main() {
  MyScreen screen(sw, sh);
  Surface sf(sw, sh);
  float*zbuf = new float[sw*sh];
  float*zbuf2 = new float[sw*sh];
  Clock clock;

  MyKinect kinect;
  float*depth;
  Color*col;

  Registration reg(kinect.dev->getIrCameraParams(), kinect.dev->getColorCameraParams());
  
  float*lastx = new float[depth_width];
  float*lasty = new float[depth_width];
  float*thisx = new float[depth_width];
  float*thisy = new float[depth_width];

  float ccx = kinect.dev->getColorCameraParams().cx;
  float ccy = kinect.dev->getColorCameraParams().cy;
  float color_pers = kinect.dev->getColorCameraParams().fx;
  float icolor_pers = 1./color_pers;

  float pers = 1000;
  float scx = sw/2, scy = sh/2;

  vec3 pos(0, 0, 0);
  int lx = -1, ly = -1;
  double dx = 0, dy = 0, dz = 0;

  while (1) {
    while (screen.gotEvent()) {
      Event e = screen.getEvent();
      int key = e.key, type = e.type;
      if (e.type == KeyPress) {
	if (e.key == K_ESCAPE) return 0;
	else if (e.key == K_SPACE) {
	  store(col, zbuf);
	}
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
    
    for (int i = 0; i < sw*sh; i++) sf.pixels[i] = 0, zbuf[i] = 1e9, zbuf2[i] = 1e9;

    getDepth(zbuf, lastx, lasty, thisx, thisy, depth, reg);

    for (int j = 0; j < color_height; j++) {
      for (int i = 0; i < color_width; i++) {
	float&pz = zbuf[i+j*color_width];
	if (pz > 1e8) continue;
	float px = (i-ccx)*icolor_pers, py = (j-ccy)*icolor_pers;
	px *= pz;
	py *= pz;

	px -= pos.x;
	py -= pos.y;
	pz -= pos.z;
	float x = side.x*px+side.y*py+side.z*pz;
	float y = up.x*px+up.y*py+up.z*pz;
	float z = viewdir.x*px+viewdir.y*py+viewdir.z*pz, iz = 1./z;
	int ix = x*pers*iz+scx;
	int iy = y*pers*iz+scy;
	if (z > 0 and ix >= 0 and iy >= 0 and ix < sw and iy < sh and z < zbuf2[ix+iy*sw]) {
	  zbuf2[ix+iy*sw] = z;
	  sf.pixels[ix+iy*sw] = col[i+j*color_width];
	}
      }
    }
    

    screen.putSurface(sf);
    clock.tick(30);
    clock.print();
  }
}
