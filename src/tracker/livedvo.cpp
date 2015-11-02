#include "../wrapper.hpp"
#include "../../deps/bat/bat.hpp"
#include <libfreenect2/registration.h>
#include "../../deps/bat/vec3.hpp"
#include "dvo.cpp"

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
      if (dz*t < depth[i+j*color_width]) {
	depth[i+j*color_width] = dz*t;
	float diff = (-nz*3+nx+ny*.5)*.25/r;
	if (diff < .2) diff = .2;
	if (diff > 1) diff = 1;
	sf.pixels[i+j*sw] = int(diff*255)*0x10101;
      }
    }
  }
}

void irotate(float*rotmat, float*x, float*r) {
  x[0] -= rotmat[3];
  x[1] -= rotmat[7];
  x[2] -= rotmat[11];
  r[0] = rotmat[0]*x[0]+rotmat[4]*x[1]+rotmat[8]*x[2];
  r[1] = rotmat[1]*x[0]+rotmat[5]*x[1]+rotmat[9]*x[2];
  r[2] = rotmat[2]*x[0]+rotmat[6]*x[1]+rotmat[10]*x[2];
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

  char*imageNow = new char[color_width*color_height];
  char*imagePrevious = new char[color_width*color_height];

  float spherepos[3] = {0, 0, 1e9}, newspherepos[3];

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
      }
    }

    kinect.getColorAndDepth((uint**)&col, &depth);
    
    for (int i = 0; i < sw*sh; i++) zbuf[i] = 1e9;

    getDepth(zbuf, lastx, lasty, thisx, thisy, depth, reg);

    swap(imageNow, imagePrevious);
    for (int i = 0; i < sw*sh; i++) 
      imageNow[i] = (col[i].r+col[i].g+col[i].b)/3;

    float rotmat[12];
    findRotation(imageNow, zbuf, imagePrevious, rotmat);

    irotate(rotmat, spherepos, newspherepos);
    swap(spherepos, newspherepos);

    for (int i = 0; i < color_width*color_height; i++)
      sf.pixels[i] = col[i];

    drawSphere(sf, zbuf, spherepos[0], spherepos[1], spherepos[2], 100, color_pers);

    screen.putSurface(sf);
    clock.tick(30);
    clock.print();
  }
}
