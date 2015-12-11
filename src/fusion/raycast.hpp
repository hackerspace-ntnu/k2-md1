
float idealRaycast(float x, float y, float z, float dx, float dy, float dz) {
	float px = .5-x, py = .5-y, pz = .5-z, r = .4;
	float pp = px*px+py*py+pz*pz;
	float pd = px*dx+py*dy+pz*dz;
	float dd = dx*dx+dy*dy+dz*dz;
	float sq = pd*pd+dd*(r*r-pp);
	if (sq < 0) return 0;
	return (pd-sqrt(sq))/dd;
}

float recRaycast(TSDF&world, float&t, float&x, float&y, float&z, float&dx, float&dy, float&dz, float&xpad, float&ypad, float&zpad, int d, int gridi = 0) {
	float iw = 1;
	for (int i = 0; i <= d; i++) iw *= world.size[i];
	float w = 1.f/iw;

	int idx = dx>=0?1:-1, idy = dy>=0?1:-1, idz = dz>=0?1:-1;
	t += 1e-5*w;
	int ix = floor((t*dx+x)*iw);
	int iy = floor((t*dy+y)*iw);
	int iz = floor((t*dz+z)*iw);
	float xdist = (ix*w+w*(dx>=0)-x)*xpad;
	float ydist = (iy*w+w*(dy>=0)-y)*ypad;
	float zdist = (iz*w+w*(dz>=0)-z)*zpad;

	const int&ww = world.size[d];
	if (d) {
		ix &= ww-1;
		iy &= ww-1;
		iz &= ww-1;
	}

	while (ix >= 0 and iy >= 0 and iz >= 0 and ix < ww and iy < ww and iz < ww) {
		if (d < world.depth) {
			TSDF_Branch&cur = world.grid[d][ix+ww*(iy+ww*(iz+ww*gridi))];
			if (cur.nearSurface) {
				float f = recRaycast(world, t, x, y, z, dx, dy, dz, xpad, ypad, zpad, d+1, cur.child);
				if (f) return f;
			}
		} else {
			TSDF_Leaf&cur = world.leaf[ix+ww*(iy+ww*(iz+ww*gridi))];
			if (cur.d < 0) 
				return t;
		}
		if (xdist < ydist and xdist < zdist) {
			t = xdist;
			xdist += idx*xpad*w;
			ix += idx;
		} else if (ydist < zdist) {
			t = ydist;
			ydist += idy*ypad*w;
			iy += idy;
		} else {
			t = zdist;
			zdist += idz*zpad*w;
			iz += idz;
		}
	}
	return 0;
}

float raycast(TSDF&world, float x, float y, float z, float dx, float dy, float dz) {
	float xpad = 1.f/dx;
	float ypad = 1.f/dy;
	float zpad = 1.f/dz;

	float tocube = 0;
	if (x < 0) tocube = max(tocube, -x*xpad);
	if (y < 0) tocube = max(tocube, -y*ypad);
	if (z < 0) tocube = max(tocube, -z*zpad);
	if (x > 1) tocube = max(tocube, (1-x)*xpad);
	if (y > 1) tocube = max(tocube, (1-y)*ypad);
	if (z > 1) tocube = max(tocube, (1-z)*zpad);

	return recRaycast(world, tocube, x, y, z, dx, dy, dz, xpad, ypad, zpad, 0);
}

/*inline void innerloop(int&j, float&x1, float&f1, float&x2, float&f2, float z0, float&zfx, float*&zbuf) {
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
    float na = nx*ax+ny*ay+nz*az, inz = 1./nz;
    nx *= -inz;
    ny *= -inz;
    nz = na*inz;

    //if (nx*nx > 100 or ny*ny > 100) return;

    float ly = mymin(by, cy), ly2 = mymax(by, cy);
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
};

void rasterizeQuad(float*&minbuf, float*&maxbuf, int&w, int&h, Projection&camera, float px, float py, float pz, float&ax, float&ay, float&az, float&bx, float&by, float&bz, float&nx, float&ny, float&nz) {
	float na = nx*x+ny*y+nz*z, ina = 1.f/na;
	float&f = camera.fx, &cx = camera.cx, &cy = camera.cy;
	float x[4] = {px*f/pz+cx, 
								(px+ax)*f/(pz+az)+cx, 
								(px+bx)*f/(pz+bz)+cx, 
								(px+ax+bx)*f/(pz+az+bz)+cx};
	float y[4] = {py*f/pz+cy, 
								(py+ay)*f/(pz+az)+cy, 
								(py+by)*f/(pz+bz)+cy, 
								(py+ay+by)*f/(pz+az+bz)+cy};
	
}


struct worldpos {
	int off, d, x, y, z;
	worldpos(int off, int d, int x, int y, int z) : off(off), d(d), x(x), y(y), z(z) {}
};


void fullRaycast(TSDF&world, float px, float py, float pz, float*rot, Projection&camera, float*zbuf, float*minbuf, float*maxbuf, int w, int h) {
	for (int i = 0; i < w*h; i++) minbuf[i] = 1e9;

	queue<worldpos> q;
	q.push(worldpos(0, 0, 0, 0, 0));
	while (!q.empty()) {
		worldpos p = q.front();
		q.pop();
		const int&ww = world.size[p.d];
		TSDF_Branch*&grid = world.grid[p.d];
		for (int k = 0; k < ww; k++) {
			for (int j = 0; j < ww; j++) {
				for (int i = 0; i < ww; i++) {
					TSDF_Branch&cur = grid[i+ww*(j+ww*(k+ww*p.off))];
					if (cur.nearSurface) {
						if (p.d < world.d-1) {
							q.push(worldpos(cur.child, d+1, p.x*ww+i, p.y*ww+j, p.z*ww+k));
						} else {
							float x = p.x*ww+i-px, y = p.y*ww+j-py, z = p.z*ww+k-pz;
							float tx = x*rot[0]+y*rot[3]+z*rot[6];
							float ty = x*rot[1]+y*rot[4]+z*rot[7];
							float tz = x*rot[2]+y*rot[5]+z*rot[8];
							for (int n = 0; n < 9; n += 3) {
								int m = (n+3)%9;
								int l = (n+6)%9;
								rasterizeQuad(minbuf, maxbuf, w, h, camera, 
															tx, ty, tz, 
															rot[n], rot[n+1], rot[n+2], 
															rot[m], rot[m+1], rot[m+2], 
															rot[l], rot[l+1], rot[l+2]);
								int l = (6-n-m)%3*3;
								rasterizeQuad(minbuf, maxbuf, w, h, camera, 
															tx+rot[l], ty+rot[l+1], tz+rot[l+2], 
															rot[n], rot[n+1], rot[n+2], 
															rot[m], rot[m+1], rot[m+2], 
															rot[l], rot[l+1], rot[l+2]);
								}
							}
						}
					}
				}
			}
		}
	}
	
	for (int i = 0; i < w*h; i++) zbuf[i] = minbuf[i];	
}
*/
