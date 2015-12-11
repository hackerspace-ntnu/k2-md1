struct Projection {
	float cx, cy, fx;
	Projection() {}
	Projection(float x, float y, float f) {
		cx = x;
		cy = y;
		fx = f;
	}
};

int recfuse(TSDF&world, float*&zbuf, int&w, int&h, float*rotmat, Projection&cam, float px, float py, float pz, int d = 0, int p = 0) {
	const int&ww = world.size[d];
	const float treshold = .01;
	int ret = 0;
	float totsize = 1;
	for (int i = 0; i <= d; i++) totsize *= world.size[i];
	if (d == world.depth) {
		float itotsize = 1.f/totsize;

		for (int k = 0; k < ww; k++) {
			for (int j = 0; j < ww; j++) {
				for (int i = 0; i < ww; i++) {
					float dx = i-px*ww+.5f;
					float dy = j-py*ww+.5f;
					float dz = k-pz*ww+.5f;
					float x = rotmat[0]*dx+rotmat[3]*dy+rotmat[6]*dz;
					float y = rotmat[1]*dx+rotmat[4]*dy+rotmat[7]*dz;
					float z = rotmat[2]*dx+rotmat[5]*dy+rotmat[8]*dz;

					int ix = x*cam.fx/z+cam.cx, iy = y*cam.fx/z+cam.cy;
					if (ix >= 0 and iy >= 0 and ix < w and iy < h and zbuf[ix+w*iy]) {
						float sz = zbuf[ix+w*iy]*cam.fx-z*itotsize;
						if (fabs(sz) < treshold) {
							TSDF_Leaf&leaf = world.leaf[i+ww*(j+ww*(k+ww*p))];
							leaf.d = (leaf.d*leaf.w+sz)/(leaf.w+1);
							ret |= leaf.d<0;
							leaf.w++;
						}
					}
				}
			}
		}
	} else {
		for (int k = 0; k < ww; k++) {
			for (int j = 0; j < ww; j++) {
				for (int i = 0; i < ww; i++) {
					TSDF_Branch&cur = world.grid[d][i+ww*(j+ww*(k+ww*p))];

					float dx = i-px*ww;
					float dy = j-py*ww;
					float dz = k-pz*ww;
					float xmax = 0, xmin = w, ymax = 0, ymin = h;
					for (int l = 0; l < 8; l++) {
						int lx = l&1, ly = l>>1&1, lz = l>>2;
						float x = rotmat[0]*(dx+lx)+rotmat[3]*(dy+ly)+rotmat[6]*(dz+lz);
						float y = rotmat[1]*(dx+lx)+rotmat[4]*(dy+ly)+rotmat[7]*(dz+lz);
						float z = rotmat[2]*(dx+lx)+rotmat[5]*(dy+ly)+rotmat[8]*(dz+lz);

						float ix = x*cam.fx/z+cam.cx, iy = y*cam.fx/z+cam.cy;
						xmax = max(xmax, ix);
						xmin = min(xmin, ix);
						ymax = max(ymax, iy);
						ymin = min(ymin, iy);
					}
					xmax = min(xmax, (float)w);
					xmin = max(xmin, 0.f);
					ymax = min(ymax, (float)h);
					ymin = max(ymin, 0.f);
					for (int iy = ymin; iy < ymax; iy++) {
						for (int ix = xmin; ix < xmax; ix++) {
							float frontp = 0, frontq = 1, backp = 1, backq = 0;
							float vx = ix-cam.cx, vy = iy-cam.cy;
							float arr[3] = {dx, dy, dz};
							for (int l = 0; l < 3; l++) {
								float nvx = vx*rotmat[3*l]+vy*rotmat[1+3*l]+cam.fx*rotmat[2+3*l];
								if (nvx > 0) {
									if (frontp*nvx < (arr[l])*frontq) 
										frontp = arr[l], frontq = nvx;
									if (backp*nvx > (arr[l]+1)*backq) 
										backp = arr[l]+1, backq = nvx;
								} else {
									if (frontp*nvx > (arr[l]+1)*frontq) 
										frontp = -arr[l]-1, frontq = -nvx;
									if (backp*nvx < (arr[l])*backq) 
										backp = -arr[l], backq = -nvx;
								}
							}

							/*float nvx = vx*rotmat[0]+vy*rotmat[1]+cam.fx*rotmat[2];
							float nvy = vx*rotmat[3]+vy*rotmat[4]+cam.fx*rotmat[5];
							float nvz = vx*rotmat[6]+vy*rotmat[7]+cam.fx*rotmat[8];
							if (frontp*nvx < (dx+(nvx<0))*frontq) 
								frontp = dx+(nvx<0), frontq = nvx;
							if (backp*nvx > (dx+(nvx>0))*backq) 
								backp = dx+(nvx>0), backq = nvx;

							if (frontp*nvy < (dy+(nvy<0))*frontq) 
								frontp = dy+(nvy<0), frontq = nvy;
							if (backp*nvy > (dy+(nvy>0))*backq) 
								backp = dy+(nvy>0), backq = nvy;

							if (frontp*nvz < (dz+(nvz<0))*frontq) 
								frontp = dz+(nvz<0), frontq = nvz;
							if (backp*nvz > (dz+(nvz>0))*backq) 
							backp = dz+(nvz>0), backq = nvz;*/

							if (frontp*backq > backp*frontq) continue;
							float z = zbuf[ix+iy*w]*totsize;
							//cout << zfront << ' ' << zback << ' ' << z << endl;
							if (frontp < (z+treshold)*frontq and 
									backp > (z-treshold)*backq) goto pass;
						}
					}
					if (!cur.nearSurface) cout << "no" << endl;
					
					continue;
				pass:
					if (cur.child == -1) {
						cur.child = world.avail[d][--world.alen[d]];
						int nw = world.size[d+1], off = cur.child*nw*nw*nw;
						if (d+1 < world.depth)
							for (int l = 0; l < nw*nw*nw; l++) {
								world.grid[d+1][off+l].nearSurface = 0;
								world.grid[d+1][off+l].child = -1;
							}
						else {
							for (int l = 0; l < nw*nw*nw; l++) {
								world.leaf[off+l].w = 0;
								world.leaf[off+l].d = 1;
							}
						}
					}
					cur.nearSurface |= recfuse(world, zbuf, w, h, rotmat, cam, px*ww-i, py*ww-j, pz*ww-k, d+1, cur.child);
					ret |= cur.nearSurface;
				}
			}
		}
	}
	return ret;
}

/*void fuse(float*zbuf, int w, int h, float*rotmat, Projection*camera) {
	queue<int2> q;
	for (int i = 0; i < (1<<size0*3); i++) {
		float x = (i&(1<<size0)-1);
		float y = (i>>size0&(1<<size0)-1);
		float z = (i>>size0*2&(1<<size0)-1);
		rotate(rotmat, x, y, z);
		if (crossany(zbuf, x, y, z)) 
			q.push(grid0[i].child, i);
	}
	queue<int2> q2;
	while (!q.empty()) {
		int j, p = q.front();
		q.pop();
		for (int i = 0; i < (1<<size1*3); i++) {
			float x = j+(i&(1<<size1)-1);
			float y = j+(i>>size1&(1<<size1)-1);
			float z = j+(i>>size1*2&(1<<size1)-1);
			rotate(rotmat, x, y, z);
			if (crossany(zbuf, x, y, z)) 
				q2.push(grid1[j*(1<<size1*3)+i].child, i+j);
		}
	}
	while (!q2.empty()) {
		int j, p = q2.front();
		q2.pop();
		for (int i = 0; i < (1<<size2*3); i++) {
			float x = j+(i&(1<<size2)-1);
			float y = j+(i>>size2&(1<<size2)-1);
			float z = j+(i>>size2*2&(1<<size2)-1);
			rotate(rotmat, x, y, z);
			float d = sample(zbuf, x, y, z);
			TSDF_Leaf&leaf = grid2[j*(1<<size2*3)+i];
			leaf.d = (leaf.d*leaf.w+d)/(leaf.w+1);
			leaf.w += 1;
		}
	}
	}*/
