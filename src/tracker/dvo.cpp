#include <cmath>

int min(int a, int b) {return a<b?a:b;}
int max(int a, int b) {return a>b?a:b;}

// Conjugate gradient solver for size 6
// Solves Ax = b
int solve6(float*A, float*x, float*r) {
	const int n = 6;
	float p[6];
	float Ap[6];
	
	double rr = 0, eps = 1e-6;
	for (int i = 0; i < n; i++) rr += r[i]*r[i], x[i] = 0, p[i] = r[i];

	if (rr < eps) return 0;
	int c = 0;
	while (c++ < 6) {
		double pAp = 0;
		for (int i = 0; i < n; i++) {
			Ap[i] = 0;
			for (int j = 0; j < n; j++) 
				Ap[i] += p[j]*A[i*n+j];
			pAp += p[i]*Ap[i];
		}

		double alpha = rr/pAp, rr2 = 0;
		for (int i = 0; i < n; i++) {
			x[i] += alpha*p[i];			
			r[i] -= alpha*Ap[i];
			rr2 += r[i]*r[i];
		}
	
		if (rr2 < eps) return c;

		double beta = rr2/rr;
		rr = rr2;

		for (int i = 0; i < n; i++) p[i] = r[i]+beta*p[i];
	}
	return c;
}

void rotate(float*rotmat, float*x, float*r) {
	r[0] = rotmat[0]*x[0]+rotmat[1]*x[1]+rotmat[2]*x[2];
	r[1] = rotmat[4]*x[0]+rotmat[5]*x[1]+rotmat[6]*x[2];
	r[2] = rotmat[8]*x[0]+rotmat[9]*x[1]+rotmat[10]*x[2];
}

void updaterotmatBefore(float*rotmat, float*x) {
	//Calculate the rotation matrix of x (rot = exp(x))
	float theta = sqrt(x[3]*x[3]+x[4]*x[4]+x[5]*x[5]);
	float ilen = theta > 1e-7?1./theta:1;
	float kx = x[3]*ilen, ky = x[4]*ilen, kz = x[5]*ilen;
	float sa = sin(theta), ca = 1-cos(theta);
	float rotx[3] = {1-ca+kx*kx*ca, kx*ky*ca+kz*sa, kx*kz*ca-ky*sa}; //Columns
	float roty[3] = {kx*ky*ca-kz*sa, 1-ca+ky*ky*ca, ky*kz*ca+kx*sa};
	float rotz[3] = {kx*kz*ca+ky*sa, ky*kz*ca-kx*sa, 1-ca+kz*kz*ca};

	//Update the transformation (rotmat = rotmat*rot)
	float newrotx[3], newroty[3], newrotz[3], newtrans[3];
	rotate(rotmat, x, newtrans);
	rotate(rotmat, rotx, newrotx);
	rotate(rotmat, roty, newroty);
	rotate(rotmat, rotz, newrotz);
	for (int i = 0; i < 3; i++) {
		rotmat[i*4] = newrotx[i];
		rotmat[i*4+1] = newroty[i];
		rotmat[i*4+2] = newrotz[i];
		rotmat[i*4+3] += newtrans[i];
	}
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

template <typename T>
float bilin(T*img, float&x, float&y, int&w) {
	int ix = x, iy = y;
	float x1 = x-ix, x0 = 1-x1;
	float y1 = y-iy, y0 = 1-y1;
	return 
		img[ix  +iy*w  ]*x0*y0+
		img[ix+1+iy*w  ]*x1*y0+
		img[ix  +iy*w+w]*x0*y1+
		img[ix+1+iy*w+w]*x1*y1;		
}

static const int Iw_ = 1920, Ih_ = 1080;
float Ifx_ = 1081, Icx_ = Iw_*.5-.5, Icy_ = Ih_*.5-.5;

void findRotation(byte*I1_, float*Z1_, byte*I2_, float rotmat[12]) {
	for (int i = 0; i < 12; i++)
		rotmat[i] = i%5 == 0; //Set rotmat to idenitity matrix

	float steplen = 1;
	float isteplen = 1./steplen;

	int wh = Iw_*Ih_;
	float*dI2 = new float[wh*2];
	float*Z1 = new float[wh];
	byte*I1 = new byte[wh];
	byte*I2 = new byte[wh];

	float*J = new float[wh*6];
	float*J0 = new float[wh*6];
	float*R = new float[wh];

	float*plotimg = new float[wh];

	float lastivar = 1e-2;
	for (int scale = 6; scale >= 0; scale--) { //Iterate from coarse to fine
		int Iw = Iw_>>scale, Ih = Ih_>>scale;
		float s = 1./(1<<scale);
		float Ifx = Ifx_*s, iIfx = 1.f/Ifx;
		float Icx = (Icx_+.5)*s-.5;
		float Icy = (Icy_+.5)*s-.5;

		if (scale) { //Scale textures down
			for (int j = 0; j < Ih; j++) 
				for (int i = 0; i < Iw; i++) {
					int s1 = 0, s2 = 0;
					float s3 = 0, c = 0;
					for (int l = 0; l < (1<<scale); l++)
						for (int k = 0; k < (1<<scale); k++) {
							s1 += I1_[(i<<scale)+k+((j<<scale)+l)*Iw_];
							s2 += I2_[(i<<scale)+k+((j<<scale)+l)*Iw_];
							if (Z1_[(i<<scale)+k+((j<<scale)+l)*Iw_] < 1e8)
								s3 += Z1_[(i<<scale)+k+((j<<scale)+l)*Iw_], c++;
						}
					I1[j*Iw+i] = (s1>>scale*2-1)+1>>1;
					I2[j*Iw+i] = (s2>>scale*2-1)+1>>1;
					Z1[j*Iw+i] = c >= (1<<scale*2-1)?s3/c:1e9;
				}
		} else 
			for (int i = 0; i < Iw_*Ih_; i++)
				I1[i] = I1_[i], I2[i] = I2_[i], Z1[i] = Z1_[i];

		float f = isteplen*Ifx*.5;
		int wh = Iw*Ih;
		for (int j = 0; j < Ih; j++) { //Differentiate I2 in x and y
			for (int i = 0; i < Iw; i++) {
				dI2[i+j*Iw] = f*(I2[min(i+1, Iw)+j*Iw]-I2[max(i-1, 0)+j*Iw]);
				dI2[i+j*Iw+wh] = f*(I2[min(j+1, Ih)*Iw+i]-I2[max(j-1, 0)*Iw+i]);
			}
		}

		for (int i = 0; i < wh*6; i++) J0[i] = 0;

		for (int j = 0; j < Ih; j++) {
			for (int i = 0; i < Iw; i++) {

				//Deproject 3d point from I1 depth texture
				float z = Z1[j*Iw+i];

				float*Ji = J0+i+j*Iw;
				if (z > 1e7) continue;
				float x = (i-Icx)*z*iIfx;
				float y = (j-Icy)*z*iIfx;
					
				float iz = 1./z, izz = iz*iz;
				float dI2x = dI2[i+j*Iw];
				float dI2y = dI2[i+j*Iw+wh];

				Ji[wh*0] = dI2x*iz;
				Ji[wh*1] = dI2y*iz;
				Ji[wh*2] =-(dI2x*x+dI2y*y)*izz;
				Ji[wh*3] = -dI2x*x*y*izz-dI2y*(1+y*y*izz);
				Ji[wh*4] = dI2x*(1+x*x*izz)+dI2y*x*y*izz;
				Ji[wh*5] = (-dI2x*y+dI2y*x)*iz;
			}
		}

		float lasterror = 1e20;
		float lastx[6];
		for (int iterations = 0; iterations < 10; iterations++) {

			float A[36], b[6];
			for (int i = 0; i < 36; i++) A[i] = 0;
			for (int i = 0; i < 6; i++) b[i] = 0;

			float error = 0, errorc = 0, defined_count = 0;
			for (int j = 0; j < Ih; j++) {
				for (int i = 0; i < Iw; i++) {

					 //Deproject 3d point from I1 depth texture
					float pz1 = Z1[j*Iw+i];
					R[i+j*Iw] = 12345;
					if (pz1 > 1e7) continue;
					float px1 = (i-Icx)*pz1*iIfx;
					float py1 = (j-Icy)*pz1*iIfx;

					 //Transform to 3d point in I2 reference (with current estimate)
					float x = rotmat[0]*px1+rotmat[1]*py1+rotmat[2]*pz1+rotmat[3];
					float y = rotmat[4]*px1+rotmat[5]*py1+rotmat[6]*pz1+rotmat[7];
					float z = rotmat[8]*px1+rotmat[9]*py1+rotmat[10]*pz1+rotmat[11];

					//Project to I2 coordinate
					float iz = 1./z;
					float px2 = x*Ifx*iz+Icx;
					float py2 = y*Ifx*iz+Icy;
					int ix = px2, iy = py2;
					if (ix >= 0 and iy >= 0 and ix < Iw-1 and iy < Ih-1) {
						//Calculate jacobian of I2
						float*Ji = J+6*(i+j*Iw);

						float izz = iz*iz;
						float dI2x = bilin<float>(dI2, px2, py2, Iw);
						float dI2y = bilin<float>(dI2+Iw*Ih, px2, py2, Iw);

						Ji[0] = dI2x*iz;
						Ji[1] = dI2y*iz;
						Ji[2] =-(dI2x*x+dI2y*y)*izz;
						Ji[3] = -dI2x*x*y*izz-dI2y*(1+y*y*izz);
						Ji[4] = dI2x*(1+x*x*izz)+dI2y*x*y*izz;
						Ji[5] = (-dI2x*y+dI2y*x)*iz;

						//for (int k = 0; k < 6; k++)
						//	Ji[k] = bilin(J0+k*wh, px2, py2, Iw);

						defined_count++;
						R[i+j*Iw] = bilin(I2, px2, py2, Iw)-I1[i+j*Iw];
					}
				}
			}
			float ivar = lastivar, v = 5.f;
			for (int witer = 0; witer < 10; witer++) {
				float f = 0;//-defined_count/((v+1)*ivar);
				//float df = 0;
				for (int i = 0; i < wh; i++) {
					if (R[i] != 12345) {
						float r2 = R[i]*R[i];
						float isq = 1./(v+r2*ivar);
					  f += r2*isq;
						//df += r2*isq*isq;
					}
				}
				//cout << ivar << ' ' << f << ' ' << df << endl;
				//df *= v;
				//ivar -= f/df;
				ivar = defined_count/(v+1)/f;
				if (abs(ivar-lastivar) < ivar*1e-3) break;
				lastivar = ivar;
			}
			lastivar = ivar;

			//cout << 1./ivar << endl;

			for (int i = 0; i < Iw*Ih; i++) {
				if (R[i] == 12345) {
					plotimg[i] = -255;
					continue;
				}

				float W = (v+1)/(v+R[i]*R[i]*ivar);
				float RW = R[i]*W;
				float RRW = RW*R[i];
				float*Ji = J+i*6;
				plotimg[i] = abs(R[i])*2-255;//RRW*s*10-255;
				//Add jacobian to LHS matrix
				for (int l = 0; l < 6; l++) {
					float JiW = Ji[l]*W;
					for (int k = 0; k <= l; k++) 
						A[l*6+k] += JiW*Ji[k];
				}

				error += RRW;
				errorc++;
				for (int k = 0; k < 6; k++) 
					b[k] -= Ji[k]*RW;
			}

			for (int l = 0; l < 6; l++) 
				for (int k = 0; k < l; k++) 
					A[k*6+l] = A[l*6+k];

			for (int i = 0; i < wh; i++)
				plotimg[i] = J[i*6+5]*.2;
			//plot(plotimg, Iw, Ih);

			error /= errorc;
			cout << error << endl << endl;
			if (error >= lasterror*.99) {
				//cout << "Reset" << endl;
				//for (int i = 0; i < 6; i++) cout << lastx[i] << ' ';
				//cout << endl;

				updaterotmatAfter(rotmat, lastx);
				break;
			}
			lasterror = error;

			float x[6];
			solve6(A, x, b); //Solve for new transformation

			for (int i = 0; i < 6; i++) {
				if (x[i] != x[i]) goto next;
			}
			for (int i = 0; i < 6; i++) lastx[i] = -x[i];

			//for (int i = 0; i < 6; i++) cout << x[i] << ' ';
			//cout << endl;

			updaterotmatAfter(rotmat, x);
			/*for (int j = 0; j < 3; j++) {
				for (int i = 0; i < 4; i++) cout << rotmat[i+j*4] << ' ';
				cout << endl;
				}*/
		}
	next:;
	}
	delete[]I1;
	delete[]I2;
	delete[]Z1;
	delete[]dI2;
}

