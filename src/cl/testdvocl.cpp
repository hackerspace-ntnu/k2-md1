#include "clinit.hpp"
#include <sys/time.h>
#include "visualizer.hpp"

void openimg(const char*name, byte*img, int wh) {
	FILE*fp = fopen(name, "r");
	int t1, t2;
	fscanf(fp, "%s %d %d", img, &t1, &t2);
	getc(fp);
	for (int i = 0; i < wh; i++) 
		img[i] = (fgetc(fp)+fgetc(fp)+fgetc(fp))/3;
	fclose(fp);
}

void openzbuf(const char*name, float*zbuf, int wh) {
	FILE*fp = fopen(name, "r");
	for (int i = 0; i < wh; i++)
		fscanf(fp, "%f", zbuf+i);
	fclose(fp);
}

byte plotimg[1920*1080*4*6];
void plotGpu(cl::Image2D img, int w, int h) {
	cl::size_t<3> origo, size;
	origo[0] = origo[1] = origo[2] = 0;
	size[0] = w, size[1] = h, size[2] = 1;
	queue.enqueueReadImage(img, 1, origo, size, 0, 0, plotimg);
	plot(plotimg, w, h);
}

void plotR(cl::Buffer img, int w, int h) {
	float*fimg = (float*)plotimg;
	queue.enqueueReadBuffer(img, 1, 0, w*h*sizeof(float), plotimg);
	for (int i = 0; i < w*h; i++) 
		if (fimg[i] == 12345.f) fimg[i] = -1;
		else fimg[i] = fabs(fimg[i])*3-1;
	plot(fimg, w, h, 255);
}

void plotJ(cl::Buffer img, int w, int h) {
	float*fimg = (float*)plotimg;
	queue.enqueueReadBuffer(img, 1, 0, w*h*sizeof(float)*6, plotimg);
	for (int i = 0; i < w*h; i++) fimg[i] = fimg[i*6+4];
	plot(fimg, w, h, 160*.4);
}

void rotate(float*rotmat, float*x, float*r) {
	r[0] = rotmat[0]*x[0]+rotmat[1]*x[1]+rotmat[2]*x[2];
	r[1] = rotmat[4]*x[0]+rotmat[5]*x[1]+rotmat[6]*x[2];
	r[2] = rotmat[8]*x[0]+rotmat[9]*x[1]+rotmat[10]*x[2];
}
void updaterotmatAfter(float*rotmat, float*x) {
	//Calculate the rotation matrix of x (rot = exp(x))
	float theta = sqrt(x[3]*x[3]+x[4]*x[4]+x[5]*x[5]);
	float ilen = theta > 1e-7?1./theta:1;
	float kx = x[3]*ilen, ky = x[4]*ilen, kz = x[5]*ilen;
	float sa = sin(theta), ca = 1-cos(theta);
	float rot[12] = {1-ca+kx*kx*ca, kx*ky*ca-kz*sa, kx*kz*ca+ky*sa, x[0], 
									 kx*ky*ca+kz*sa, 1-ca+ky*ky*ca, ky*kz*ca-kx*sa, x[1], 
									 kx*kz*ca-ky*sa, ky*kz*ca+kx*sa, 1-ca+kz*kz*ca, x[2]};

	//Update the transformation (rotmat = rot*rotmat)
	float oldrotx[3], oldroty[3], oldrotz[3], oldtrans[3];
	float newrotx[3], newroty[3], newrotz[3], newtrans[3];
	for (int i = 0; i < 3; i++) {
		oldrotx[i] = rotmat[i*4];
		oldroty[i] = rotmat[i*4+1];
		oldrotz[i] = rotmat[i*4+2];
		oldtrans[i] = rotmat[i*4+3];
	}
	rotate(rot, oldtrans, newtrans);
	rotate(rot, oldrotx, newrotx);
	rotate(rot, oldroty, newroty);
	rotate(rot, oldrotz, newrotz);
	
	for (int i = 0; i < 3; i++) {
		rotmat[i*4] = newrotx[i];
		rotmat[i*4+1] = newroty[i];
		rotmat[i*4+2] = newrotz[i];
		rotmat[i*4+3] = newtrans[i]+x[i];
	}
}

int main() {
	int w = 1920, h = 1080;
	initVisual(800, 600);

	byte*I1 = new byte[w*h];
	byte*I2 = new byte[w*h];
	float*Z1 = new float[w*h];
	openimg("../tracker/dump1.ppm", I1, w*h);
	openimg("../tracker/dump2.ppm", I2, w*h);
	openzbuf("../tracker/zbufdump1.txt", Z1, w*h);

	float Icx_ = w*.5-.5, Icy_ = h*.5-.5, Ifx_ = 1081, iIfx_ = 1.f/Ifx_;

	initCL();
	cl::Program program = createProgram("dvo.cl");
	try {

	float rotmat[12];

	timeval ta, tb;
	gettimeofday(&ta, NULL);

		
	cl::Image2D gpu_I1(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_INTENSITY, CL_UNORM_INT8), w, h, 0, I1);
	cl::Image2D gpu_I2(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_INTENSITY, CL_UNORM_INT8), w, h, 0, I2);
	cl::Image2D gpu_Z1(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_INTENSITY, CL_FLOAT), w, h, 0, Z1);

	cl::Image2D gpu_mipmapI1(context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_INTENSITY, CL_UNORM_INT8), w/2, h/2, 0, NULL);
	cl::Image2D gpu_mipmapI2(context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_INTENSITY, CL_UNORM_INT8), w/2, h/2, 0, NULL);
	cl::Image2D gpu_mipmapZ1(context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_INTENSITY, CL_FLOAT), w/2, h/2, 0, NULL);

	cl::Image2D gpu_dI2(context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RG, CL_SNORM_INT8), w, h, 0, NULL);

	cl::Buffer gpu_residual(context, CL_MEM_READ_WRITE, sizeof(float)*w*h);
	cl::Buffer gpu_jacobian(context, CL_MEM_READ_WRITE, sizeof(float)*w*h*6);
	float ivar = 1e-2;
	cl::Buffer gpu_ivar(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float), &ivar);
	
	int groupsize = 16, groups = 16;
	cl::Buffer gpu_partsums(context, CL_MEM_READ_WRITE, sizeof(float)*groups);
	cl::Buffer gpu_partcount(context, CL_MEM_READ_WRITE, sizeof(float)*groups);

	cl::Buffer gpu_partA(context, CL_MEM_READ_WRITE, sizeof(float)*groups*21);
	cl::Buffer gpu_partb(context, CL_MEM_READ_WRITE, sizeof(float)*groups*6);

	cl::Buffer gpu_A(context, CL_MEM_READ_WRITE, sizeof(float)*36);
	cl::Buffer gpu_b(context, CL_MEM_READ_WRITE, sizeof(float)*6);
	cl::Buffer gpu_dXi(context, CL_MEM_READ_WRITE, sizeof(float)*6);
	
	for (int i = 0; i < 12; i++) rotmat[i] = i%5==0;

	cl::Buffer gpu_rotmat(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float)*12, rotmat);

	cl::Kernel downSample(program, "downSample");
	cl::Kernel downSampleZ(program, "downSampleSkipUndefined");

	cl::Kernel calcGradient(program, "calcGradient");
	cl::Kernel calcRAJ(program, "calcResidualAndJacobian");

	cl::Kernel calcVariance(program, "calcVariance1");
	cl::Kernel calcVariance2(program, "calcVariance2");

	cl::Kernel calcLinEq(program, "calcLinEq1");
	cl::Kernel calcLinEq2(program, "calcLinEq2");
	cl::Kernel solveLinEq(program, "solveLinEq");


	for (int scale = 6; scale >= 0; scale--) {
		float s = 1./(1<<scale);
		float Ifx = Ifx_*s, iIfx = 1.f/Ifx;
		float Icx = (Icx_+.5)*s-.5;
		float Icy = (Icy_+.5)*s-.5;

		if (scale) {
			downSample.setArg(0, gpu_I1);
			downSample.setArg(1, gpu_mipmapI1);
			downSample.setArg(2, sizeof(int), &scale);
			queue.enqueueNDRangeKernel(downSample, cl::NullRange, cl::NDRange(w>>scale, h>>scale), cl::NullRange);

			downSample.setArg(0, gpu_I2);
			downSample.setArg(1, gpu_mipmapI2);
			downSample.setArg(2, sizeof(int), &scale);
			queue.enqueueNDRangeKernel(downSample, cl::NullRange, cl::NDRange(w>>scale, h>>scale), cl::NullRange);

			downSampleZ.setArg(0, gpu_Z1);
			downSampleZ.setArg(1, gpu_mipmapZ1);
			downSampleZ.setArg(2, sizeof(int), &scale);
			queue.enqueueNDRangeKernel(downSampleZ, cl::NullRange, cl::NDRange(w>>scale, h>>scale), cl::NullRange);
		}

		//plotGpu(gpu_mipmapI1, w>>scale, h>>scale);

		calcGradient.setArg(0, scale?gpu_mipmapI2:gpu_I2);
		calcGradient.setArg(1, gpu_dI2);
	
		queue.enqueueNDRangeKernel(calcGradient, cl::NullRange, cl::NDRange(w>>scale, h>>scale), cl::NullRange);

		int wh = (w>>scale)*(h>>scale);

		float lasterror = 1e20;
		float idXi[6];
		for (int iter = 0; iter < 10; iter++) {

			calcRAJ.setArg(0, scale?gpu_mipmapI1:gpu_I1);
			calcRAJ.setArg(1, scale?gpu_mipmapZ1:gpu_Z1);
			calcRAJ.setArg(2, scale?gpu_mipmapI2:gpu_I2);
			calcRAJ.setArg(3, gpu_dI2);
			calcRAJ.setArg(4, gpu_rotmat);
			calcRAJ.setArg(5, gpu_residual);
			calcRAJ.setArg(6, gpu_jacobian);
			calcRAJ.setArg(7, sizeof(float), &Icx);
			calcRAJ.setArg(8, sizeof(float), &Icy);
			calcRAJ.setArg(9, sizeof(float), &Ifx);
			calcRAJ.setArg(10, sizeof(float), &iIfx);

			queue.enqueueNDRangeKernel(calcRAJ, cl::NullRange, cl::NDRange(w>>scale, h>>scale), cl::NullRange);

			//plotR(gpu_residual, w>>scale, h>>scale);

			calcVariance.setArg(0, gpu_residual);
			calcVariance.setArg(1, sizeof(int), &wh);
			calcVariance.setArg(2, gpu_ivar);
			calcVariance.setArg(3, sizeof(float)*groupsize, NULL);
			calcVariance.setArg(4, sizeof(int)*groupsize, NULL);
			calcVariance.setArg(5, gpu_partsums);
			calcVariance.setArg(6, gpu_partcount);

			calcVariance2.setArg(0, gpu_ivar);
			calcVariance2.setArg(1, gpu_partsums);
			calcVariance2.setArg(2, gpu_partcount);
			calcVariance2.setArg(3, sizeof(int), &groups);
	
			float lastivar = ivar;
			for (int i = 0; i < 10; i++) {
				queue.enqueueNDRangeKernel(calcVariance, cl::NullRange, cl::NDRange(groups*groupsize), cl::NDRange(groupsize));				
				queue.enqueueTask(calcVariance2);

				queue.enqueueReadBuffer(gpu_ivar, 1, 0, sizeof(float), &ivar);
				if (i == 0 and ivar < lastivar) break;
				if (fabs(ivar-lastivar) < fabs(1e-2*ivar)) break;
				lastivar = ivar;
			}

			float error = 1.f/ivar;
			if (error >= lasterror*.99) {
				//cout << "reset" << endl;
				updaterotmatAfter(rotmat, idXi);
				break;
			}
			lasterror = error;

			calcLinEq.setArg(0, gpu_residual);
			calcLinEq.setArg(1, gpu_jacobian);
			calcLinEq.setArg(2, gpu_ivar);
			calcLinEq.setArg(3, sizeof(int), &wh);
			calcLinEq.setArg(4, sizeof(float)*groupsize*21, NULL);
			calcLinEq.setArg(5, sizeof(float)*groupsize*6, NULL);
			calcLinEq.setArg(6, gpu_partA);
			calcLinEq.setArg(7, gpu_partb);

			calcLinEq2.setArg(0, gpu_partA);
			calcLinEq2.setArg(1, gpu_partb);
			calcLinEq2.setArg(2, sizeof(int), &groups);
			calcLinEq2.setArg(3, gpu_A);
			calcLinEq2.setArg(4, gpu_b);

			solveLinEq.setArg(0, gpu_A);
			solveLinEq.setArg(1, gpu_dXi);
			solveLinEq.setArg(2, gpu_b);

			queue.enqueueNDRangeKernel(calcLinEq, cl::NullRange, cl::NDRange(groups*groupsize), cl::NDRange(groupsize));
			queue.enqueueTask(calcLinEq2);
			queue.enqueueTask(solveLinEq);

			float dXi[6];
			queue.enqueueReadBuffer(gpu_dXi, 1, 0, sizeof(float)*6, &dXi);
			for (int i = 0; i < 6; i++) idXi[i] = -dXi[i];

			cout << error*1e4 << endl;
			//for (int i = 0; i < 6; i++) 
			//	cout << dXi[i] << ' ';
			//cout << endl;

			updaterotmatAfter(rotmat, dXi);
			queue.enqueueWriteBuffer(gpu_rotmat, 1, 0, sizeof(float)*12, rotmat);
		}
	}

	gettimeofday(&tb, NULL);
	double deltime = (tb.tv_sec-ta.tv_sec+(tb.tv_usec-ta.tv_usec)*.000001);

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 4; j++) 
			cout << rotmat[i*4+j] << ", ";
		cout << endl;
	}

	cout << "Time: " << deltime << endl;
		
	} catch (cl::Error e) {
		//cout << CL_INVALID_MEM_OBJECT << endl;
		if (e.err() == CL_INVALID_ARG_SIZE) cout << "Invalid arg size" << endl;
		cout << endl << e.what() << " : " << e.err() << endl;
	}
}
