#include "../cl/clinit.hpp"
#include <sys/time.h>
#include <iostream>
using std::cout;
using std::endl;
#include "identifyclerror.hpp"
#include <cmath>
#include "../reconstruct/linalg.hpp"
#if defined DEBUG
#include "/home/test/hackerspace/mapbot/k2-md1/src/cl/visualizer.hpp"
#endif
#include <iomanip>
#include <queue>
#include <algorithm>

const int w = 512, h = w;

int N_FRAMES = 478;
const int iterframes = 100;
const char*input_dir = "../reconstruct/recorded";


#if defined DEBUG
Color plotimg[w*h];
void plotGpu(cl::Image2D&img, int w, int h) {
	cl::size_t<3> origo, size;
	origo[0] = origo[1] = origo[2] = 0;
	size[0] = w, size[1] = h, size[2] = 1;
	queue.enqueueReadImage(img, 1, origo, size, 0, 0, plotimg);
	plotColor(plotimg, w, h);
}
#endif

void loadZB(float*z, int w, int h, int num) {
  char name[100];
  sprintf(name, "%s/%03dzb%dx%d.ppm", input_dir, num, w, h);
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
      z[i+j*w] = s*(1.f/(1<<14))-1.f;
			if (z[i+j*w] == -1.f) z[i+j*w] = 0.f;
    }
  }
  fclose(fp);
}

const int sw = 512, sh = 424;
const int scales = 5;

//struct DVO {
	cl::Program program;
	cl::Kernel prepare_source;
	cl::Kernel shrink;
	cl::Kernel shrinkPacked;
	cl::Kernel prepare_dest;
	cl::Kernel wrap;
	cl::Kernel sumAb;
	cl::Buffer device_transform;
	cl::Buffer device_projection[scales];
	cl::Buffer device_Ab_part;
	cl::Buffer device_Ab;

	cl::Image2D plot_img;

	float raw[sw*sh];
	float host_img[w*h];

	void initDVO() {
		
#if defined DEBUG
		const char args[] = "-cl-denorms-are-zero -cl-finite-math-only -DDEBUG";
#else
		const char args[] = "-cl-denorms-are-zero -cl-finite-math-only";
#endif
		program = createProgram("dvo.cl", args);

		prepare_source = cl::Kernel(program, "PackSource");
		shrink = cl::Kernel(program, "shrink");
		shrinkPacked = cl::Kernel(program, "shrinkPacked");
		prepare_dest = cl::Kernel(program, "PrepareDest");
		wrap = cl::Kernel(program, "VectorWrap");
		sumAb = cl::Kernel(program, "SumAb");

		device_transform = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(float)*12);
		for (int i = 0; i < scales; i++)
			device_projection[i] = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(float)*4);
		int parts = (w/8)*(h/8);
		device_Ab_part = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float)*32*parts);
		device_Ab = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof(float)*32);
		
		plot_img = cl::Image2D(context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGBA, CL_UNORM_INT8), w, h);
	}

	struct SourceImg {
		cl::Image2D device_data[scales];
		SourceImg(int index) {
			cl::Image2D src_depth;
			loadZB(raw, sw, sh, index);
			for (int j = 0; j < sh; j++) 
				for (int i = 0; i < sw; i++) 
					host_img[sw*((h-sh)/2+j)+(w-sw)/2+i] = raw[w*j+i];

			src_depth = cl::Image2D(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_R, CL_FLOAT), w, h, 0, host_img);

			for (int i = 0; i < scales; i++) 
				device_data[i] = cl::Image2D(context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGBA, CL_HALF_FLOAT), w>>i+1, h>>i+1);

			prepare_source.setArg(0, src_depth);
			prepare_source.setArg(1, device_data[0]);
			queue.enqueueNDRangeKernel(prepare_source, cl::NullRange, cl::NDRange(w>>1, h>>1), cl::NullRange);

			for (int i = 1; i < scales; i++) {
				shrinkPacked.setArg(0, device_data[i-1]);
				shrinkPacked.setArg(1, device_data[i]);
				queue.enqueueNDRangeKernel(shrinkPacked, cl::NullRange, cl::NDRange(w>>i+1, h>>i+1), cl::NullRange);
			}
		}
	};

	struct DestImg {
		cl::Image2D device_data[scales];
		DestImg(int index) {
			cl::Image2D dest_depth[scales];
			loadZB(raw, sw, sh, index);
			for (int j = 0; j < sh; j++) 
				for (int i = 0; i < sw; i++) 
					host_img[sw*((h-sh)/2+j)+(w-sw)/2+i] = raw[w*j+i];

			dest_depth[0] = cl::Image2D(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_R, CL_FLOAT), w, h, 0, host_img);

			for (int i = 0; i < scales; i++) {
				device_data[i] = cl::Image2D(context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGBA, CL_HALF_FLOAT), w>>i, h>>i);

				if (i) 
					dest_depth[i] = cl::Image2D(context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_R, CL_HALF_FLOAT), w>>i, h>>i);
			}
			for (int i = 0; i < scales; i++) {
				if (i) {
					shrink.setArg(0, dest_depth[i-1]);
					shrink.setArg(1, dest_depth[i]);
					queue.enqueueNDRangeKernel(shrink, cl::NullRange, cl::NDRange(w>>i, h>>i), cl::NullRange);
				}
				prepare_dest.setArg(0, dest_depth[i]);
				prepare_dest.setArg(1, device_data[i]);
				queue.enqueueNDRangeKernel(prepare_dest, cl::NullRange, cl::NDRange(w>>i, h>>i), cl::NullRange);
			}
		}
		~DestImg() {
			
		}
	};
void calcGaussNewton(SourceImg&src, DestImg&dest, int scale, float*Ab, float ivar = 500.f) {
		wrap.setArg(0, src.device_data[scale]);
		wrap.setArg(1, dest.device_data[scale]);
		wrap.setArg(2, device_transform);
		wrap.setArg(3, device_projection[scale]);
		wrap.setArg(4, device_Ab_part);
		wrap.setArg(5, plot_img);
		wrap.setArg(6, ivar);

		sumAb.setArg(0, device_Ab_part);
		sumAb.setArg(1, device_Ab);
		int parts = (w*h>>6)>>scale*2+2;
		//cout << parts << endl;
		sumAb.setArg(2, sizeof(int), &parts);

		queue.enqueueNDRangeKernel(wrap, cl::NullRange, cl::NDRange(w>>scale+1, h>>scale+1), cl::NDRange(8, 8));
#if defined DEBUG
		plotGpu(plot_img, w>>scale, h>>scale);
#endif

		queue.enqueueNDRangeKernel(sumAb, cl::NullRange, cl::NDRange(32), cl::NullRange);
		queue.enqueueReadBuffer(device_Ab, 0, 0, sizeof(float)*29, Ab);
	}
//};

int operator-(const timeval&tb, const timeval&ta) {
	return (tb.tv_sec-ta.tv_sec)*1000000+(tb.tv_usec-ta.tv_usec);
}

float overlap(se3 a, se3 b) {
	se3 d = a*(b*-1);
	float trans[12];
	d.exp(trans, 0);
	float x = trans[2]+trans[3];
	float y = trans[6]+trans[7];
	float z = trans[10]+trans[11]-1;
	float dist = x*x+y*y+z*z;
	float theta = sqrt(d.nx*d.nx+d.ny*d.ny+d.nz*d.nz);
	return cos(theta)/dist;
}

int main(int argc, char**argv) {
  if (argc > 1) {
    sscanf(argv[1], "%d", &N_FRAMES);
  } else {
    cout << "Using " << N_FRAMES << " frames by default, change by running " << argv[0] << " <N frames>" << endl;
  }
  
	initCL();

#if defined DEBUG
	initVisual(w, h);
#endif

	timeval ta, tb;
	try {
		initDVO();

		std::vector<se3> path;
		path.push_back(se3(0,0,0,0,0,0));

		std::vector<SourceImg> frames;
		frames.push_back(SourceImg(0));

		float Ab[iterframes*29];

		float transform[12];

		float fx = 365.463;//1081.f/2.f;

		for (int scale = 0; scale < scales; scale++) {
			float scaledprojection[4] = {((w>>scale)-1)*0.5f, ((h>>scale)-1)*0.5f, fx/(1<<scale), (1<<scale)/fx};
			queue.enqueueWriteBuffer(device_projection[scale], 0, 0, sizeof(float)*4, scaledprojection);
		}

		int offset = 0;
		int calcframes = N_FRAMES;
		for (int newframe = 1; newframe < calcframes; newframe++) {
			cout << "New frame: " << newframe << endl;
			while (frames.size() > iterframes) {
				frames.erase(frames.begin());
				offset++;
				}
			frames.push_back(SourceImg(newframe));
			
			path.push_back(se3(path[path.size()-1].l));

			DestImg new_dest(newframe);
			
			float ivar = 2500;
			for (int scale = scales-1; scale >= 0; scale--) {
				//cout << "Next scale: " << scale << endl;
				float lasterror = 1e9;
				se3 lastview = path[newframe];

				for (int iter = 0; iter < 10; iter++) {

					int iterframe = 0;
					/*std::priority_queue<std::pair<int, int> > pq;
					for (int frame = 0; frame < newframe; frame++) {
						pq.push(std::make_pair(overlap(path[frame], path[newframe]), frame));
					}
					while (pq.size() && iterframe < iterframes) {
						int frame = pq.top().second;
						pq.pop();*/
						
					for (int frame = std::max(0, newframe-iterframes); frame < newframe; frame++) {
						se3 diff = path[newframe]*(path[frame]*-1);

						//for (int i = 0; i < 6; i++) cout << diff.l[i] << ' ';
						//cout << endl;
						diff.exp(transform,0);

						queue.enqueueWriteBuffer(device_transform, 0, 0, sizeof(float)*12, transform);
						calcGaussNewton(frames[frame-offset], new_dest, scale, Ab+iterframe++*29, ivar);
					}
					queue.finish();

					for (int frame = 1; frame < iterframe; frame++) 
						for (int i = 0; i < 29; i++)
							Ab[i] += Ab[frame*29+i];

					/*for (int i = 0; i < 29; i++) 
						cout << Ab[i] << ' ';
						cout << endl;*/

					float error = Ab[28]/Ab[27];
					//cout << Ab[27] << ' ' << Ab[28] << endl;
					//if (scale == 2)
					//cout << scale << ": " << 1./error << " " << ivar << endl;
					//ivar = 1./error;
					//cout << 1e6*error << ' ' << 1./error << ' ' << Ab[27] << endl;
					if (error != error or error > lasterror*.99) {
						path[newframe] = lastview;
						break;
					}
					lastview = se3(path[newframe].l);
					lasterror = error;

					float A[36], b[6], x[6] = {0}, tmp1[6], tmp2[6];
					int c = 0;
					for (int j = 0; j < 6; j++) 
						for (int i = 0; i <= j; i++) 
							A[i+j*6] = Ab[c++];
					for (int j = 0; j < 6; j++)
						for (int i = j+1; i < 6; i++)
							A[i+j*6] = A[j+i*6];
					for (int i = 0; i < 6; i++) b[i] = -Ab[c++];

					solve(A, b, x, tmp1, tmp2, 6);
					//for (int i = 0; i < 6; i++) cout << x[i] << ' ';
					//cout << endl;
				
					se3 updateView(x);
					//dr/d(path[newframe]*(path[frame]*-1));
					//updateView.invert();
					path[newframe] = updateView*path[newframe];
					//cout << Ab[27] << ' ' << Ab[28] << endl;
					//for (int i = 0; i < 29; i++) cout << Ab[i] << endl;

					/*for (int i = 0; i < 6; i++) {
						for (int j = 0; j < 6; j++) 
						cout << setw(8) << tA[i*6+j] << ' ';
						cout << ' ' << setw(8) << x[i+6*iterframes] << endl;
						}
						cout << endl;*/
				}
			}
		}

		FILE*fp = fopen("views.txt", "w");
		for (int i = 0; i < calcframes; i++) {
			se3 view = path[i]*-1;
			view.exp(transform, 0);
			//if (i)
			//cout << var[i]-var[i-1] << endl;
			for (int k = 0; k < 3; k++) {
			  for (int j = 0; j < 3; j++) 
			    fprintf(fp, " %f", transform[j+k*4]);
			  fprintf(fp, " %f\n", transform[3+k*4]);
			}
			fprintf(fp, " %f %f %f %f\n\n", 0.0, 0.0, 0.0, 1.0);
		}
		fclose(fp);
		return 0;
		gettimeofday(&ta, NULL);
		gettimeofday(&tb, NULL);
		cout << (tb-ta)/1000. << " ms" << endl;

	} catch (cl::Error e) {
		cout << endl << e.what() << " : " << errlist[-e.err()] << endl;
	}
}
