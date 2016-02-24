#include "clinit.hpp"
#include <sys/time.h>
#include <cmath>

void rotate(float*rotmat, float*x, float*r) {
  r[0] = rotmat[0]*x[0]+rotmat[1]*x[1]+rotmat[2]*x[2];
  r[1] = rotmat[4]*x[0]+rotmat[5]*x[1]+rotmat[6]*x[2];
  r[2] = rotmat[8]*x[0]+rotmat[9]*x[1]+rotmat[10]*x[2];
}

void updaterotmatAfter(float*rotmat, float*rot) {
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
    rotmat[i*4+3] = newtrans[i]+rot[i*4+3];
  }
}

void updaterotmatBefore(float*rotmat, float*rot) {
  //Update the transformation (rotmat = rotmat*rot)
   float oldrotx[3], oldroty[3], oldrotz[3], oldtrans[3];
   float newrotx[3], newroty[3], newrotz[3], newtrans[3];
   for (int i = 0; i < 3; i++) {
     oldrotx[i] = rot[i*4];
     oldroty[i] = rot[i*4+1];
     oldrotz[i] = rot[i*4+2];
     oldtrans[i] = rot[i*4+3];
   }
   rotate(rotmat, oldtrans, newtrans);
   rotate(rotmat, oldrotx, newrotx);
   rotate(rotmat, oldroty, newroty);
   rotate(rotmat, oldrotz, newrotz);
   for (int i = 0; i < 3; i++) {
     rotmat[i*4] = newrotx[i];
     rotmat[i*4+1] = newroty[i];
     rotmat[i*4+2] = newrotz[i];
     rotmat[i*4+3] += newtrans[i];
   }
}
void to6DOF(float*rotmat, float*x) {
  float trace = 0;
  for (int i = 0; i < 3; i++) x[i] = rotmat[i*4+3], trace += rotmat[i*5];
  if (trace > 3) trace = 3;
  float theta = acos((trace-1)*.5f);
  float f = .5*theta/sin(theta);
  if (theta < 1e-7) f = 0;
  x[3] = f*(rotmat[3*4+2]-rotmat[3+2*4]);
  x[4] = f*(rotmat[1*4+3]-rotmat[1+3*4]);
  x[5] = f*(rotmat[2*4+1]-rotmat[2+1*4]);
  for (int i = 0; i < 6; i++) 
    if (x[i] != x[i]) {
      cout << "Error in convertion to 6DOF" << endl;
      for (int i = 0; i < 3; i++) {
	for (int j = 0; j < 4; j++)
	  cout << rotmat[i*4+j] << ' ';
	cout << endl;
      }
      cout << endl;
      return;
    }
}

void updaterotmat6DOF(float*rotmat, float*x) {
  //Calculate the rotation matrix of x (rot = exp(x))
  float theta = sqrt(x[3]*x[3]+x[4]*x[4]+x[5]*x[5]);
  float ilen = theta > 1e-7?1./theta:1;
  float kx = x[3]*ilen, ky = x[4]*ilen, kz = x[5]*ilen;
  float sa = sin(theta), ca = 1-cos(theta);
  float rot[12] = {1-ca+kx*kx*ca, kx*ky*ca-kz*sa, kx*kz*ca+ky*sa, x[0], 
		   kx*ky*ca+kz*sa, 1-ca+ky*ky*ca, ky*kz*ca-kx*sa, x[1], 
		   kx*kz*ca-ky*sa, ky*kz*ca+kx*sa, 1-ca+kz*kz*ca, x[2]};

  updaterotmatAfter(rotmat, rot);
}

struct Tracker {
  int w, h;
  float rotmat[12];
  float result[12];
  cl::Image2D gpu_I1, gpu_I2, gpu_Z1, gpu_mipmapI1, gpu_mipmapI2, gpu_mipmapZ1, gpu_dI2;
  cl::Buffer gpu_residual, gpu_jacobian, gpu_ivar, gpu_partsums, gpu_partcount, gpu_part, gpu_A, gpu_b, gpu_dXi, gpu_rotmat, gpu_estimate, gpu_confidence;

  cl::Kernel downSample, downSampleZ, calcGradient, calcRAJ, calcVariance1, calcVariance2, calcLinEq1, calcLinEq2, solveLinEq, useEstimate;

  int groups, groupsize;
  float Icx, Icy, Ifx;	
  cl::Program program;
  int notfirst;
  float confidence[6];
  float lastmovement[6];
  int counter1, counter2;
  Tracker(int w_, int h_) {
    w = w_, h = h_;
    Icx = w*.5-.5, Icy = h*.5-.5, Ifx = 1081*w/1920.f;

    counter1 = counter2 = 0;
    
    for (int i = 0; i < 6; i++) confidence[i] = 0, lastmovement[i] = 0;

    notfirst = 0;
    groups = 1<<5, groupsize = 1<<6;

    initCL();
    program = createProgram("dvo.cl");
    try {

      cl::ImageFormat unorm8(CL_INTENSITY, CL_UNORM_INT8), 
	halffloat(CL_INTENSITY, CL_FLOAT);
      gpu_I1 = cl::Image2D(context, CL_MEM_READ_ONLY, unorm8, w, h);
      gpu_I2 = cl::Image2D(context, CL_MEM_READ_ONLY, unorm8, w, h);
      gpu_Z1 = cl::Image2D(context, CL_MEM_READ_ONLY, halffloat, w, h);

      gpu_mipmapI1 = cl::Image2D(context, CL_MEM_READ_WRITE, unorm8, w/2, h/2);
      gpu_mipmapI2 = cl::Image2D(context, CL_MEM_READ_WRITE, unorm8, w/2, h/2);
      gpu_mipmapZ1 = cl::Image2D(context, CL_MEM_READ_WRITE, halffloat, w/2, h/2);

      gpu_dI2 = cl::Image2D(context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RG, CL_SNORM_INT8), w, h);

      gpu_residual = createBuffer(w*h);
      gpu_jacobian = createBuffer(w*h*8);

      gpu_ivar = createBuffer(1);
	
      gpu_partsums = createBuffer(groups);
      gpu_partcount = createBuffer(groups);

      gpu_part = createBuffer(groups*32);

      gpu_A = createBuffer(36);
      gpu_b = createBuffer(6);
      gpu_dXi = createBuffer(6);
	
      gpu_rotmat = createBuffer(12);

      gpu_estimate = createBuffer(6);
      gpu_confidence = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float)*6, confidence);


      downSample = cl::Kernel(program, "downSample");
      downSampleZ = cl::Kernel(program, "downSampleSkipUndefined");
			
      calcGradient = cl::Kernel(program, "calcGradient");
      calcRAJ = cl::Kernel(program, "calcResidualAndJacobian");
			
      calcVariance1 = cl::Kernel(program, "calcVariance1");
      calcVariance2 = cl::Kernel(program, "calcVariance2");
			
      calcLinEq1 = cl::Kernel(program, "calcLinEq1");
      calcLinEq2 = cl::Kernel(program, "calcLinEq2");
      solveLinEq = cl::Kernel(program, "solveLinEq");

      useEstimate = cl::Kernel(program, "useEstimate");
			
      for (int i = 0; i < 12; i++) result[i] = i%5==0;	
			
    } catch (cl::Error e) {
      cout << endl << e.what() << " : " << e.err() << endl;
    }
  }
  ~Tracker() {
    
  }
  cl::Buffer createBuffer(int floats, int mode = CL_MEM_READ_WRITE) {
    return cl::Buffer(context, mode, sizeof(float)*floats);
  }
  void reset() {
    notfirst = 0;
    for (int i = 0; i < 12; i++) result[i] = i%5==0;
    for (int i = 0; i < 6; i++) lastmovement[i] = 0;
  }
  void update(unsigned char*img, float*zbuf) {
    counter1++;
    try {
      swap(gpu_I1, gpu_I2);

      cl::size_t<3> origo, size;
      origo[0] = origo[1] = origo[2] = 0;
      size[0] = w, size[1] = h, size[2] = 1;
      queue.enqueueWriteImage(gpu_I1, 1, origo, size, 0, 0, img);
      if (notfirst++ == 0) return;

      queue.enqueueWriteImage(gpu_Z1, 1, origo, size, 0, 0, zbuf);

      float ivar = .01;
      queue.enqueueWriteBuffer(gpu_ivar, 1, 0, sizeof(float), &ivar);
 
      for (int i = 0; i < 12; i++) rotmat[i] = i%5==0;

      for (int scale = 6; scale >= 0; scale--) {

	float s = 1./(1<<scale);
	float Ifx_ = Ifx*s, iIfx = 1.f/Ifx_;
	float Icx_ = (Icx+.5)*s-.5;
	float Icy_ = (Icy+.5)*s-.5;

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

	  queue.enqueueWriteBuffer(gpu_rotmat, 1, 0, sizeof(float)*12, rotmat);

	  calcRAJ.setArg(0, scale?gpu_mipmapI1:gpu_I1);
	  calcRAJ.setArg(1, scale?gpu_mipmapZ1:gpu_Z1);
	  calcRAJ.setArg(2, scale?gpu_mipmapI2:gpu_I2);
	  calcRAJ.setArg(3, gpu_dI2);
	  calcRAJ.setArg(4, gpu_rotmat);
	  calcRAJ.setArg(5, gpu_residual);
	  calcRAJ.setArg(6, gpu_jacobian);
	  calcRAJ.setArg(7, sizeof(float), &Icx_);
	  calcRAJ.setArg(8, sizeof(float), &Icy_);
	  calcRAJ.setArg(9, sizeof(float), &Ifx_);
	  calcRAJ.setArg(10, sizeof(float), &iIfx);

	  queue.enqueueNDRangeKernel(calcRAJ, cl::NullRange, cl::NDRange(w>>scale, h>>scale), cl::NullRange);

	  //plotR(gpu_residual, w>>scale, h>>scale);
	  //plotJ(gpu_jacobian, w>>scale, h>>scale);

	  calcVariance1.setArg(0, gpu_residual);
	  calcVariance1.setArg(1, sizeof(int), &wh);
	  calcVariance1.setArg(2, gpu_ivar);
	  calcVariance1.setArg(3, sizeof(float)*groupsize, NULL);
	  calcVariance1.setArg(4, sizeof(int)*groupsize, NULL);
	  calcVariance1.setArg(5, gpu_partsums);
	  calcVariance1.setArg(6, gpu_partcount);

	  calcVariance2.setArg(0, gpu_ivar);
	  calcVariance2.setArg(1, gpu_partsums);
	  calcVariance2.setArg(2, gpu_partcount);
	  calcVariance2.setArg(3, sizeof(int), &groups);
	
	  float lastivar = ivar;
	  for (int i = 0; i < 10; i++) {
	    queue.enqueueNDRangeKernel(calcVariance1, cl::NullRange, cl::NDRange(groups*groupsize), cl::NDRange(groupsize));				
	    queue.enqueueTask(calcVariance2);

	    queue.enqueueReadBuffer(gpu_ivar, 1, 0, sizeof(float), &ivar);
	    if (i == 0 and ivar < lastivar) break;
	    if (fabs(ivar-lastivar) < fabs(1e-2*ivar)) break;
	    lastivar = ivar;
	  }
	  
	  float error = 1.f/ivar;
	  if (error != error) {
	    //cout << "Warning: error = nan" << endl;
	    //cout << ivar << ' ' << scale << ' ' << iter << endl;
	    return; // happens if I1 == I2
	  }

	  if (error < 1e-20 or error >= lasterror*.99) {
	    //if (error < 1e-20 and scale == 0) cout << "Perfect!" << endl;

	    //cout << "reset" << endl;
	    updaterotmat6DOF(rotmat, idXi);
	    break;
	  }
	  lasterror = error;

	  calcLinEq1.setArg(0, gpu_residual);
	  calcLinEq1.setArg(1, gpu_jacobian);
	  calcLinEq1.setArg(2, gpu_ivar);
	  calcLinEq1.setArg(3, sizeof(int), &wh);
	  calcLinEq1.setArg(4, sizeof(float)*groupsize*32, NULL);
	  calcLinEq1.setArg(5, gpu_part);

	  calcLinEq2.setArg(0, gpu_part);
	  calcLinEq2.setArg(1, sizeof(int), &groups);
	  calcLinEq2.setArg(2, gpu_A);
	  calcLinEq2.setArg(3, gpu_b);


	  float sofar[6], estimate[6];
	  to6DOF(rotmat, sofar);
	  for (int i = 0; i < 6; i++) estimate[i] = lastmovement[i]-sofar[i];
	  //for (int i = 0; i < 6; i++) cout << sofar[i] << endl;

	  queue.enqueueWriteBuffer(gpu_estimate, 1, 0, sizeof(float)*6, estimate);

	  useEstimate.setArg(0, gpu_A);
	  useEstimate.setArg(1, gpu_b);
	  useEstimate.setArg(2, gpu_estimate);
	  useEstimate.setArg(3, gpu_confidence);

	  solveLinEq.setArg(0, gpu_A);
	  solveLinEq.setArg(1, gpu_dXi);
	  solveLinEq.setArg(2, gpu_b);

	  queue.enqueueNDRangeKernel(calcLinEq1, cl::NullRange, cl::NDRange(groups*groupsize), cl::NDRange(groupsize));
	  queue.enqueueTask(calcLinEq2);

	  queue.enqueueTask(useEstimate);

	  float A[36];
	  queue.enqueueReadBuffer(gpu_A, 1, 0, sizeof(float)*36, &A);

	  //cout << error*1e4 << endl;
	  //cout << scale << ' ';
	  //for (int j = 0; j < 6; j++) cout << A[j*7] << ' ';
	  //cout << endl;

	  queue.enqueueTask(solveLinEq);

	  float dXi[6];
	  queue.enqueueReadBuffer(gpu_dXi, 1, 0, sizeof(float)*6, &dXi);
	  for (int i = 0; i < 6; i++) idXi[i] = -dXi[i];

	  /*for (int i = 0; i < 6; i++) 
	    cout << dXi[i] << ' ';
	    cout << endl;*/
	  
	  updaterotmat6DOF(rotmat, dXi);
	}
      }
    }  catch (cl::Error e) {
      cout << endl << e.what() << " : " << e.err() << endl;
    }
    //cout << endl;
    for (int i = 0; i < 12; i++) 
      if (rotmat[i] != rotmat[i]) {
	cout << "No update" << endl;
	return;
      }

    to6DOF(rotmat, lastmovement);
    updaterotmatBefore(result, rotmat);
  }
};
