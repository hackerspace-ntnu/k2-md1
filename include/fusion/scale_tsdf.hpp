#include <cmath>

typedef unsigned char byte;

struct TSDF_Branch {
	int child;
	byte hasSurface, nearSurface;
};

struct TSDF_Leaf {
	float w, d;
};

float dist(float x, float y, float z) {
	return sqrt(x*x+y*y+z*z);
}

struct TSDF {
	static const int size[];
	static const int depth = 2;
	TSDF_Branch*grid[depth];
	TSDF_Leaf*leaf;
	int*avail[depth], alen[depth];
	TSDF() {
		int s = 1;
		for (int i = 0; i < depth; i++) {
			s *= size[i]*size[i]*size[i];
			grid[i] = new TSDF_Branch[s];
			avail[i] = new int[s];
			for (int j = 0; j < s; j++) avail[i][j] = j;
			alen[i] = s;
			if (i == 0) 
				for (int j = 0; j < s; j++) {
					grid[0][j].child = -1;
					grid[0][j].nearSurface = 0;
				}
		}
		leaf = new TSDF_Leaf[s*size[depth]*size[depth]*size[depth]];
	}
	void recInitSphere(int p, float cx, float cy, float cz, float r, float tr, int d) {
		cx *= size[d];
		cy *= size[d];
		cz *= size[d];
		r *= size[d];
		tr *= size[d];
		for (int k = 0; k < size[d]; k++) {
			for (int j = 0; j < size[d]; j++) {
				for (int i = 0; i < size[d]; i++) {
					if (d == depth) {
						TSDF_Leaf&cur = leaf[i+size[d]*(j+size[d]*(k+size[d]*p))];
						float d = dist(i-cx+.5, j-cy+.5, k-cz+.5)-r;
						cur.d = d;
					} else {
						int intersect = 0;
						for (int l = 0; l < 8; l++) {
							float d = dist(i+(l&1)-cx, j+(l>>1&1)-cy, k+(l>>2)-cz)-r;
							if (fabs(d) < tr) intersect = 1;
						}
						TSDF_Branch&cur = grid[d][i+size[d]*(j+size[d]*(k+p*size[d]))];
						if (intersect) {
							cur.nearSurface = 1;
							cur.child = avail[d][--alen[d]];
							recInitSphere(cur.child, cx-i, cy-j, cz-k, r, tr, d+1);
						} else {
							cur.nearSurface = 0;
						}
					}
				}
			}
		}
	}
	void initSphere() {
		recInitSphere(0, .5, .5, .5, .4, .05, 0);
	}
};
