#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

typedef struct {
	float cx, cy, focal, ifocal;
} Projection;

__constant sampler_t nearest = CLK_NORMALIZED_COORDS_FALSE |
	CLK_ADDRESS_CLAMP |
	CLK_FILTER_NEAREST;
__constant sampler_t bilinear = CLK_NORMALIZED_COORDS_FALSE |
	CLK_ADDRESS_CLAMP |
	CLK_FILTER_LINEAR;

__kernel void branch_clear(__global unsigned int grid[], 
													__global unsigned int* gridlen) {
	int i = get_global_id(0);
	grid[i] = -1;
	if (i == 0) *gridlen = 0;
}

__kernel void branch_fuse(__global unsigned int grid[], 
													volatile __global unsigned int* gridlen, 
													__read_only image2d_t depth, 
													__global uint4 task[], 
													__global unsigned int*tasks,
													__global uint4 outtask[], 
													volatile __global unsigned int* outtasks, 
													__constant float transform[12], 
													__constant Projection* projection, 
													const int s) {

	int i = get_global_id(0)&s-1, j = get_global_id(1)&s-1, k = get_global_id(2)&s-1;
	int counter = 0;
	for (int ti = get_global_id(2)/s; counter++ < 100 && ti < *tasks; ti += get_global_size(2)/s) {
		float x = task[ti].x*s+i, y = task[ti].y*s+j, z = task[ti].z*s+k;
		int ind = i+s*(j+s*(k+s*task[ti].w));
		
		//Check intersection

		//Intersection found:
		if (grid[ind] == -1) grid[ind] = atomic_inc(gridlen);
		outtask[atomic_inc(outtasks)] = (uint4)(x, y, z, grid[ind]);
	}

}

__constant float threshold = .03;

__kernel void branch_fuse_old(__global unsigned int grid[], 
															volatile __global unsigned int* gridlen, 
															__read_only image2d_t depth, 
															__global uint4 task[], 
															__global unsigned int*tasks,
															__global uint4 outtask[], 
															volatile __global unsigned int* outtasks, 
															__constant float transform[12], 
															__constant Projection* proj, 
															const int s) {

	int i = get_global_id(0)&s-1, j = get_global_id(1)&s-1, k = get_global_id(2)&s-1;
	for (int ti = get_global_id(2)/s; ti < *tasks; ti += get_global_size(2)/s) {
		float dx = task[ti].x*s+i-transform[3];
		float dy = task[ti].y*s+j-transform[7];
		float dz = task[ti].z*s+k-transform[11];
		int ind = i+s*(j+s*(k+s*task[ti].w));

		const float w = 512, h = 424;
		float xmax = 0, xmin = w, ymax = 0, ymin = h;
#pragma unroll
		for (int l = 0; l < 8; l++) {
			int lx = l&1, ly = l>>1&1, lz = l>>2;
			float x = transform[0]*(dx+lx)+transform[4]*(dy+ly)+transform[8]*(dz+lz);
			float y = transform[1]*(dx+lx)+transform[5]*(dy+ly)+transform[9]*(dz+lz);
			float z = transform[2]*(dx+lx)+transform[6]*(dy+ly)+transform[10]*(dz+lz);
			float iz = proj->focal/z;
			float ix = x*iz+proj->cx, iy = y*iz+proj->cy;
			xmax = max(xmax, ix);
			xmin = min(xmin, ix);
			ymax = max(ymax, iy);
			ymin = min(ymin, iy);
		}
		//if (xmax-1 < xmin) xmin -= .5, xmax += .5;
		//if (ymax-1 < ymin) ymin -= .5, ymax += .5;
		xmax = min(xmax, w);
		xmin = max(xmin, 0.);
		ymax = min(ymax, h);
		ymin = max(ymin, 0.);
		int good = 0, bad = 0;
		float arr[3] = {dx, dy, dz};
		for (int iy = ceil(ymin); iy < ymax; iy++) {
			for (int ix = ceil(xmin); ix < xmax; ix++) {
				float frontp = 0, frontq = 1, backp = 1, backq = 0;
				float vx = ix-proj->cx, vy = iy-proj->cy;
#pragma unroll
				for (int l = 0; l < 3; l++) {
					float nvx = vx*transform[l*4]+vy*transform[1+l*4]+proj->focal*transform[2+l*4];

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

				if (frontp*backq >= backp*frontq) continue;
				frontp *= proj->focal;
				backp *= proj->focal;
				float z = read_imagef(depth, nearest, (int2)(ix, iy)).x;
				if (z != 0.f) {
					if (frontp < (z+threshold)*frontq && 
							backp > (z-threshold)*backq) good++;
					else bad++;
				}
			}
		}

		if (!good) continue;
		//pass:;

		//Intersection found:
		if (grid[ind] == -1) grid[ind] = atomic_inc(gridlen);
		outtask[atomic_inc(outtasks)] = (uint4)(task[ti].x*s+i,
																						task[ti].y*s+j, 
																						task[ti].z*s+k, 
																						grid[ind]);
	}

}

__constant float w = 512, h = 424;

__kernel void branch_fuse2(__global unsigned int grid[], 
															volatile __global unsigned int* gridlen, 
															__read_only image2d_t depth, 
															__global uint4 task[], 
															__global unsigned int*tasks,
															__global uint4 outtask[], 
															volatile __global unsigned int* outtasks, 
															__constant float transform[12], 
															__constant Projection* proj, 
															const int s) {

	int i = get_global_id(0)&s-1, j = get_global_id(1)&s-1, k = get_global_id(2)&s-1;
	for (int ti = get_global_id(2)/s; ti < *tasks; ti += get_global_size(2)/s) {
		float dx = task[ti].x*s+i-transform[3];
		float dy = task[ti].y*s+j-transform[7];
		float dz = task[ti].z*s+k-transform[11];
		//goto pass;
		float xmax = 0, xmin = w, ymax = 0, ymin = h, x, y, z, iz, ix, iy;
#pragma unroll
		for (int l = 0; l < 8; l++) {
			int lx = l&1, ly = l>>1&1, lz = l>>2;
			x = transform[0]*(dx+lx)+transform[4]*(dy+ly)+transform[8]*(dz+lz);
			y = transform[1]*(dx+lx)+transform[5]*(dy+ly)+transform[9]*(dz+lz);
			z = transform[2]*(dx+lx)+transform[6]*(dy+ly)+transform[10]*(dz+lz);
			iz = proj->focal/z;
			ix = x*iz+proj->cx, iy = y*iz+proj->cy;
			xmax = max(xmax, ix);
			xmin = min(xmin, ix);
			ymax = max(ymax, iy);
			ymin = min(ymin, iy);
		}
		//if (xmax-1 < xmin) xmin -= .5, xmax += .5;
		//if (ymax-1 < ymin) ymin -= .5, ymax += .5;
		xmax = min(xmax, w);
		xmin = max(xmin, 0.);
		ymax = min(ymax, h);
		ymin = max(ymin, 0.);
		int good = 0, bad = 0;
		//float arr[6] = {1./dx, 1./(dx+1), 1./dy, 1./(dy+1), 1./dz, 1./(dz+1)};
		float front, back;
		for (float iy = ceil(ymin); iy < ymax; iy+=1) {
			for (float ix = ceil(xmin); ix < xmax; ix+=1) {
				float3 dn = 
					(float3)(1./((ix-proj->cx)*transform[0]+
											 (iy-proj->cy)*transform[1]+
											 proj->focal*transform[2]), 
									 1./((ix-proj->cx)*transform[4]+
											 (iy-proj->cy)*transform[5]+
											 proj->focal*transform[6]), 
									 1./((ix-proj->cx)*transform[8]+
											 (iy-proj->cy)*transform[9]+
											 proj->focal*transform[10]));
				front = max(max((dx+(dn.x<0))*dn.x, (dy+(dn.y<0))*dn.y), (dz+(dn.z<0))*dn.z);
				back = min(min((dx+(dn.x>=0))*dn.x, (dy+(dn.y>=0))*dn.y), (dz+(dn.z>=0))*dn.z);

				if (front >= back) continue;
				float z = read_imagef(depth, nearest, (float2)(ix, iy)).x;
				if (z != 0.f) {
					if (proj->focal*front < (z+threshold) && 
							proj->focal*back > (z-threshold)) good++;
					else bad++;
				}
			}
		}

		if (!good) continue;
		pass:;
		int ind = i+s*(j+s*(k+s*task[ti].w));
		//Intersection found:
		if (grid[ind] == -1) grid[ind] = atomic_inc(gridlen);
		outtask[atomic_inc(outtasks)] = (uint4)(task[ti].x*s+i,
																						task[ti].y*s+j, 
																						task[ti].z*s+k, 
																						grid[ind]);
	}
}

typedef struct {
	unsigned char d, r, g, b;
} ColorLeaf;

typedef struct {
	short d, w;
} Leaf;

__kernel void leaf_clear(__global Leaf grid[]) {
	int i = get_global_id(0);
	grid[i].d = 0;
	grid[i].w = 0;
}

__kernel void leaf_clear_color(__global ColorLeaf grid[]) {
	int i = get_global_id(0);
	grid[i].d = 0;
	grid[i].r = 0;
	grid[i].g = 0;
	grid[i].b = 255;
}

__kernel void leaf_fuse_color(__global ColorLeaf grid[], 
												__read_only image2d_t depth, 
												__global uint4 task[], 
												__global unsigned int* tasks, 
												__constant float transform[12], 
												__constant Projection* proj, 
												__read_only image2d_t colors, 
												__constant Projection* Iproj) {
	int i = get_local_id(0), j = get_local_id(1), k = get_local_id(2);
	int s = get_local_size(0);
	for (int ti = get_group_id(2); ti < *tasks; ti += get_num_groups(2)) {
		float dx = task[ti].x*s+i+.5-transform[3];
		float dy = task[ti].y*s+j+.5-transform[7];
		float dz = task[ti].z*s+k+.5-transform[11];
		float x = transform[0]*dx+transform[4]*dy+transform[8]*dz;
		float y = transform[1]*dx+transform[5]*dy+transform[9]*dz;
		float z = transform[2]*dx+transform[6]*dy+transform[10]*dz;
		int ind = i+s*(j+s*(k+s*task[ti].w));
		
		float iz = 1./z;
		float2 p = (float2)(x, y)*proj->focal*iz+(float2)(proj->cx, proj->cy);
		float pz = read_imagef(depth, nearest, (int2)(p.x, p.y)).x;
		if (pz != 0.f && pz-z > -threshold) {
			grid[ind].d = clamp((pz-z)*1e6, -32000.f, 32000.f)/256+127-20;
			//grid[ind].w += 1;
			float2 Ip = (float2)(x+0.025*0.5, y)*Iproj->focal*iz+(float2)(Iproj->cx, Iproj->cy);
			//Ip = (float2)(10, 10);
			float4 pz = read_imagef(colors, bilinear, Ip);
			grid[ind].r = pz.x*255;
			grid[ind].g = pz.y*255;
			grid[ind].b = pz.z*255;
		}
		//grid[ind].d = -30000.f;
	}

}

__kernel void leaf_fuse(__global Leaf grid[], 
												__read_only image2d_t depth, 
												__global uint4 task[], 
												__global unsigned int* tasks, 
												__constant float transform[12], 
												__constant Projection* proj) {
	int i = get_local_id(0), j = get_local_id(1), k = get_local_id(2);
	int s = get_local_size(0);
	for (int ti = get_group_id(2); ti < *tasks; ti += get_num_groups(2)) {
		float dx = task[ti].x*s+i+.5-transform[3];
		float dy = task[ti].y*s+j+.5-transform[7];
		float dz = task[ti].z*s+k+.5-transform[11];
		float x = transform[0]*dx+transform[4]*dy+transform[8]*dz;
		float y = transform[1]*dx+transform[5]*dy+transform[9]*dz;
		float z = transform[2]*dx+transform[6]*dy+transform[10]*dz;
		int ind = i+s*(j+s*(k+s*task[ti].w));
		
		float iz = proj->focal/z;
		float2 p = (float2)(x, y)*iz+(float2)(proj->cx, proj->cy);
		float pz = read_imagef(depth, nearest, (int2)(p.x, p.y)).x;
		if (pz != 0.f && pz-z > -threshold) {
			grid[ind].d = clamp((pz-z)*1e6, -32000.f, 32000.f);
			//grid[ind].w += 1;
		}
		//grid[ind].d = -30000.f;
	}

}
