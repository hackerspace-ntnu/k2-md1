#include "clinit.hpp"
#include <sys/time.h>
#include <iostream>
using std::cout;
using std::endl;

#include "identifyclerror.hpp"
#include <cmath>
#include <stack>

int operator-(const timeval&tb, const timeval&ta) {
	return (tb.tv_sec-ta.tv_sec)*1000000+(tb.tv_usec-ta.tv_usec);
}

const int w = 512, h = 424;

void loadZB(float*z, int w, int h, int num) {
  char name[100];
  sprintf(name, "/home/test/hackerspace/mapbot/k2-md1/src/reconstruct/teamrocket/recorded/%03dzb%dx%d.ppm", num, w, h);
  FILE*fp = fopen(name, "r");
	if (!fp) {
		cout << "File not found: " << name << endl;
		return;
	}
  int tmp;
  fscanf(fp, "P5\n%d %d\n%d\n", &w, &h, &tmp);
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      unsigned short s;
      fread(&s, 2, 1, fp);
      z[i+j*w] = s*(1.f/(1<<14));
			if (z[i+j*w] < 0.5f) z[i+j*w] = 0.f;
			//if (z[i+j*w] == -1.f) z[i+j*w] = 0.f;
    }
  }
  fclose(fp);
}

typedef unsigned int Color;

void loadColorIMG(Color*raw_colors, int w, int h, int num) {
	char name[100];
  sprintf(name, "/home/test/hackerspace/mapbot/k2-md1/src/reconstruct/teamrocket/recorded/%03dcol%dx%d.ppm", num, w, h);
  FILE*fp = fopen(name, "r");
	if (!fp) {
		cout << "File not found: " << name << endl;
		return;
	}
  int tmp;
  fscanf(fp, "P6\n%d %d\n%d\n", &w, &h, &tmp);
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      fread(&raw_colors[i+j*w], 3, 1, fp);
    }
  }
  fclose(fp);
}

#define clPrint(N, TYPE, VAR) {TYPE buffer[N];										\
		queue.enqueueReadBuffer(VAR, 0, 0, sizeof(TYPE)*N, buffer);				\
		for (int i = 0; i < N; i++) {cout << buffer[i] << ' ';}; cout << endl;}

void exportOctreeColor(int DEPTH, int*SIZE, unsigned** grid, unsigned char*leaf, std::string filename);

int main() {
	initCL();

	const int DEPTH = 2;
	int SIZE[] = {8, 8, 8};

	try {
		cl::Buffer grid[DEPTH+1];
		long long maxsize = 1, totsize = 30;

		cl::Program program = createProgram("fuse.cl");
		cl::Kernel branch_clear(program, "branch_clear");
		cl::Kernel leaf_clear(program, "leaf_clear_color");
		cl::Buffer gridlen[DEPTH+1];
		for (int i = 0; i <= DEPTH; i++) {
			long long s = SIZE[i];
			maxsize *= s*s*s;
			long long len = std::min((1000 > totsize ? 1000LL : totsize)*s*s*s, maxsize);
			if (i == DEPTH)
				cout << len*4 << endl;

			grid[i] = cl::Buffer(context, CL_MEM_READ_WRITE, 4*len);

			if (i < DEPTH) {
				gridlen[i+1] = cl::Buffer(context, CL_MEM_READ_WRITE, 4);
			
				branch_clear.setArg(0, grid[i]);
				branch_clear.setArg(1, gridlen[i+1]);
				queue.enqueueNDRangeKernel(branch_clear, cl::NullRange, cl::NDRange(len), cl::NullRange);
				totsize = totsize*(s*s*2+s*8);//*7/8;//careful with this
				cout << totsize << endl;
			} else {
				leaf_clear.setArg(0, grid[i]);
				queue.enqueueNDRangeKernel(leaf_clear, cl::NullRange, cl::NDRange(len), cl::NullRange);
			}
		}

		cl::Image2D device_depth;
		const int sw = 512, sh = 424;
		float raw[sw*sh], host_img[w*h];
		loadZB(raw, sw, sh, 0);
		for (int j = 0; j < h; j++) 
			for (int i = 0; i < w; i++) 
				host_img[w*j+i] = raw[sw*((sh-h)/2+j)+(sw-w)/2+i] *.5;//scale here

		device_depth = cl::Image2D(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_R, CL_FLOAT), w, h, 0, host_img);

		const int Iw = 960, Ih = 540;
		Color raw_colors[Iw*Ih];
		loadColorIMG(raw_colors, Iw, Ih, 0);
		cl::Image2D device_colors = cl::Image2D(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_UNORM_INT8), Iw, Ih, 0, raw_colors);

		float Iprojection[4] = {Iw/2-.5, Ih/2-.5, 1081.37/2};
		Iprojection[3] = 1./Iprojection[2];
		cl::Buffer device_Iprojection(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float)*4, Iprojection);




		float transform[12];
		for (int i = 0; i < 12; i++) transform[i] = i%5==0;
		transform[3] = .5;
		transform[7] = .5;
		transform[11] =-.3;
		cl::Buffer device_transform[DEPTH+1];
		long long tots = 1;
		for (int i = 0; i <= DEPTH; i++) {
			int s = SIZE[i];
			for (int j = 0; j < 12; j++) transform[j] *= s;
			tots *= s;
			if (i == DEPTH)
				for (int j = 0; j < 3; j++) 
					for (int k = 0; k < 3; k++) transform[j*4+k] /= tots*tots;

			device_transform[i] = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float)*12, transform);
		}

		//float projection[4] = {w/2-.5, h/2-.5, 1081.37/2};
		float projection[4] = {255.559, 207.671, 365.463};
		projection[3] = 1./projection[2];
		cl::Buffer device_projection(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float)*4, projection);

		cl::Buffer task(context, CL_MEM_READ_WRITE, 16*totsize);
		cl::Buffer nexttask(context, CL_MEM_READ_WRITE, 16*totsize);
		cl::Buffer tasks(context, CL_MEM_READ_WRITE, 4);
		cl::Buffer nexttasks(context, CL_MEM_READ_WRITE, 4);

		cl::Kernel branch_fuse(program, "branch_fuse2");
		cl::Kernel leaf_fuse(program, "leaf_fuse_color");

		timeval ta, tb;
		gettimeofday(&ta, NULL);
		
		cout << "Start" << endl;

		unsigned int zero = 0, one = 1;
		queue.enqueueWriteBuffer(tasks, 0, 0, 4, &one);
		unsigned int toptask[4] = {0, 0, 0, 0};
		queue.enqueueWriteBuffer(task, 0, 0, 16, toptask);
		for (int i = 0; i < DEPTH; i++) {
			queue.enqueueWriteBuffer(nexttasks, 0, 0, 4, &zero);
			int s = SIZE[i];
			branch_fuse.setArg(0, grid[i]);
			branch_fuse.setArg(1, gridlen[i+1]);
			branch_fuse.setArg(2, device_depth);
			branch_fuse.setArg(3, task);
			branch_fuse.setArg(4, tasks);
			branch_fuse.setArg(5, nexttask);
			branch_fuse.setArg(6, nexttasks);
			branch_fuse.setArg(7, device_transform[i]);
			branch_fuse.setArg(8, device_projection);
			branch_fuse.setArg(9, sizeof(int), &s);
			queue.enqueueNDRangeKernel(branch_fuse, cl::NullRange, 
																 cl::NDRange(s, s, s*256), cl::NullRange);
			std::swap(task, nexttask);
			std::swap(tasks, nexttasks);
			clPrint(1, unsigned int, tasks);
		}

		int s = SIZE[DEPTH];
		leaf_fuse.setArg(0, grid[DEPTH]);
		leaf_fuse.setArg(1, device_depth);
		leaf_fuse.setArg(2, task);
		leaf_fuse.setArg(3, tasks);
		leaf_fuse.setArg(4, device_transform[DEPTH]);
		leaf_fuse.setArg(5, device_projection);
		leaf_fuse.setArg(6, device_colors);
		leaf_fuse.setArg(7, device_Iprojection);
		queue.enqueueNDRangeKernel(leaf_fuse, cl::NullRange, 
															 cl::NDRange(s, s, s*256), cl::NDRange(s, s, s));

		unsigned int*host_grid[DEPTH], host_gridlen[DEPTH+1] = {1};
		for (int i = 0; i < DEPTH; i++) {
			queue.enqueueReadBuffer(gridlen[i+1], 0, 0, 4, &host_gridlen[i+1]);
			int s = SIZE[i];
			host_grid[i] = new unsigned int[host_gridlen[i]*s*s*s];
		}
		s = SIZE[DEPTH];
		unsigned char*host_leaf = new unsigned char[host_gridlen[DEPTH]*s*s*s*4];
		
		for (int i = 0; i < DEPTH; i++) {
			int s = SIZE[i];
			queue.enqueueReadBuffer(grid[i], 0, 0, 4*host_gridlen[i]*s*s*s, host_grid[i]);
		}
		queue.enqueueReadBuffer(grid[DEPTH], 0, 0, 4*host_gridlen[DEPTH]*s*s*s, host_leaf);

		exportOctreeColor(DEPTH, SIZE, host_grid, host_leaf, "color.oct");

		queue.finish();
		gettimeofday(&tb, NULL);
		cout << (tb-ta)/1000. << " ms" << endl;

	} catch (cl::Error e) {
		cout << endl << e.what() << " : " << errlist[-e.err()] << endl;
	}
}


struct TSDF_pos {
	int scale, ind;
};

int clamp(float a, float b, float c) {
	if (a < b) return b;
	if (a > c) return c;
	return a;
}

void exportOctreeColor(int DEPTH, int*SIZE, unsigned** grid, unsigned char*leaf, std::string filename) {
	FILE*fp = fopen(&filename[0], "w");
	int depth = 0, totscale = 1;
	for (int i = 0; i <= DEPTH; i++) depth += log2(SIZE[i]);
	for (int i = 0; i < DEPTH; i++) totscale *= SIZE[i];
	fprintf(fp, "OCT\n%d\n", depth);
	
	std::stack<TSDF_pos> q;
	TSDF_pos start;
	start.scale = 2;
	start.ind = 0;
	q.push(start);
	TSDF_pos next;
	while (q.size()) {
		TSDF_pos p = q.top();
		q.pop();

		//cout << p.scale << ' ' << p.ind << endl;
		if (p.scale <= totscale) {
			//cout << p.ind << endl;

			int gi = 0, rscale = p.scale;
			while (rscale > SIZE[gi]) rscale /= SIZE[gi++];
			int s = SIZE[gi];
			int w = s/rscale;
			unsigned char out = 0;
			for (int k = 1; k >= 0; k--) {
				for (int j = 1; j >= 0; j--) {
					for (int i = 1; i >= 0; i--) {
						int tot = 0;
						for (int k2 = 0; k2 < w; k2++) 
							for (int j2 = 0; j2 < w; j2++) 
								for (int i2 = 0; i2 < w; i2++) 
									tot |= grid[gi][(i*w+i2)+(j*w+j2)*s+(k*w+k2)*s*s+p.ind] != -1;
						out |= (tot<<i+j*2+k*4);
						if (tot) {
							if (w == 1) {
								int ns = SIZE[gi+1];
								next.ind = grid[gi][(i+j*s+k*s*s)*w+p.ind]*ns*ns*ns;
							} else 
								next.ind = p.ind+(i+j*s+k*s*s)*w;
							next.scale = p.scale*2;
							//if (w == 1 && gi == 1) {
								//cout << s << ' ';
								//cout << i+j*s+k*s*s+p.ind << endl;
							//} else 
								q.push(next);
						}
					}
				}
			}
			putc(out, fp);
		} else if (p.scale <= totscale*SIZE[DEPTH]) {
			int s = SIZE[DEPTH];
			int w = s*totscale/p.scale;
			unsigned char out = 0;
			for (int k = 1; k >= 0; k--) {
				for (int j = 1; j >= 0; j--) {
					for (int i = 1; i >= 0; i--) {
						int tot = 0;
						for (int k2 = 0; k2 < w; k2++) 
							for (int j2 = 0; j2 < w; j2++) 
								for (int i2 = 0; i2 < w; i2++) {
									int val = leaf[((i*w+i2)+(j*w+j2)*s+(k*w+k2)*s*s+p.ind)*2];
									tot |= (val > 1-(1<<15) && val < (1<<15)-1);
								}
						out |= (tot<<i+j*2+k*4);
						if (tot) {
							next.ind = p.ind+(i+j*s+k*s*s)*w;
							next.scale = p.scale*2;
							q.push(next);
						}
					}
				}
			}
			putc(out, fp);
		} else {
			putc(leaf[p.ind*4], fp);
			putc(leaf[p.ind*4+1], fp);
			putc(leaf[p.ind*4+2], fp);
			putc(leaf[p.ind*4+3], fp);
		}
	}
	fclose(fp);
}
