__constant sampler_t int_sampler = CLK_NORMALIZED_COORDS_FALSE |
	CLK_ADDRESS_CLAMP |
	CLK_FILTER_NEAREST;

__constant sampler_t bilinear = CLK_NORMALIZED_COORDS_FALSE |
	CLK_ADDRESS_CLAMP |
	CLK_FILTER_LINEAR;

__kernel void downSample(__read_only image2d_t in, __write_only image2d_t out, int log_scale) {
	int scale = 1<<log_scale;
	/*int w = get_image_width(in)>>log_scale;
	int h = get_image_height(in)>>log_scale;

	for (int j = get_global_id(1); j < h; j += get_global_size(1)) {
		for (int i = get_global_id(0); i < w; i += get_global_size(0)) {*/
	int i = get_global_id(0), j = get_global_id(1);
			float sum = 0;
			for (int l = 0; l < scale; l++)
				for (int k = 0; k < scale; k++)
					sum += read_imagef(in, int_sampler, (int2)(i*scale+k, j*scale+l)).x;
			
			write_imagef(out, (int2)(i, j), sum/(scale*scale));
			//}
		//}
}

__kernel void downSampleSkipUndefined(__read_only image2d_t in, __write_only image2d_t out, int log_scale) {
	int scale = 1<<log_scale;
	/*int w = get_image_width(in)>>log_scale;
		int h = get_image_height(in)>>log_scale;

		for (int j = get_global_id(1); j < h; j += get_global_size(1)) {
		for (int i = get_global_id(0); i < w; i += get_global_size(0)) {*/
	int i = get_global_id(0), j = get_global_id(1);

	float sum = 0;
	int c = 0;
	for (int l = 0; l < scale; l++) {
		for (int k = 0; k < scale; k++) {
			float z = read_imagef(in, int_sampler, (int2)(i*scale+k, j*scale+l)).x;
			if (z < 1e8) {
				c++;
				sum += z;
			}
		}
	}
	write_imagef(out, (int2)(i, j), c >= scale*scale*.5 ? sum/c : 1e9);
	//}
	//}
}

__kernel void calcGradient(__read_only image2d_t in, __write_only image2d_t out) {
	int i = get_global_id(0), j = get_global_id(1);
	int px = i-1, nx = i+1;
	int py = j-1, ny = j+1;
	if (px < 0) px = 0;
	if (py < 0) py = 0;
	if (nx >= get_global_size(0)) nx = get_global_size(0)-1;
	if (ny >= get_global_size(0)) ny = get_global_size(1)-1;
	
	float dx = read_imagef(in, int_sampler, (int2)(nx, j)).x-
		read_imagef(in, int_sampler, (int2)(px, j)).x;
	float dy = read_imagef(in, int_sampler, (int2)(i, ny)).x-
		read_imagef(in, int_sampler, (int2)(i, py)).x;
	write_imagef(out, (int2)(i, j), (float4)(dx, dy, 0.f, 1.f));
}

__kernel void calcResidualAndJacobian(__read_only image2d_t I1, 
																			__read_only image2d_t Z1, 
																			__read_only image2d_t I2, 
																			__read_only image2d_t dI2, 
																			__global __read_only float rotmat[12], 
																			__global __write_only float*residual, 
																			__global __write_only float*jacobian, 
																			float Icx, 
																			float Icy,
																			float Ifx, 
																			float iIfx) {
	int2 ij = (int2)(get_global_id(0), get_global_id(1));
	int index = ij.x+ij.y*get_global_size(0);

	//Deproject 3d point from I1 depth texture
	float pz1 = read_imagef(Z1, int_sampler, ij).x;

	residual[index] = 12345;
	for (int i = 0; i < 6; i++)
		jacobian[index*6+i] = 0;
	if (pz1 > 1e7) return;

	float px1 = (ij.x-Icx)*pz1*iIfx;
	float py1 = (ij.y-Icy)*pz1*iIfx;

	//Transform to 3d point in I2 reference (with current estimate)
	float x = rotmat[0]*px1+rotmat[1]*py1+rotmat[2]*pz1+rotmat[3];
	float y = rotmat[4]*px1+rotmat[5]*py1+rotmat[6]*pz1+rotmat[7];
	float z = rotmat[8]*px1+rotmat[9]*py1+rotmat[10]*pz1+rotmat[11];

	//Project to I2 coordinate
	float iz = 1.f/z;
	float px2 = x*Ifx*iz+Icx;
	float py2 = y*Ifx*iz+Icy;
	int ix = floor(px2), iy = floor(py2);
	if (ix >= 0 and iy >= 0 and ix < get_global_size(0)-1 and iy < get_global_size(1)-1) {
		float2 p2 = (float2)(px2+.5f, py2+.5f);

		//Calculate jacobian of I2
		float izz = iz*iz;
		float4 dI2_ = read_imagef(dI2, bilinear, p2);
		float dI2x = dI2_.x*Ifx*.5f;
		float dI2y = dI2_.y*Ifx*.5f;

		int index6 = index*6;
		jacobian[index6  ] = dI2x*iz;
		jacobian[index6+1] = dI2y*iz;
		jacobian[index6+2] =-(dI2x*x+dI2y*y)*izz;
		jacobian[index6+3] =-dI2x*x*y*izz-dI2y*(1+y*y*izz);
		jacobian[index6+4] = dI2x*(1+x*x*izz)+dI2y*x*y*izz;
		jacobian[index6+5] = (dI2y*x-dI2x*y)*iz;

		//for (int k = 0; k < 6; k++)
		//Ji[k] = bilin(J0+k*wh, px2, py2, Iw);

		residual[index] = (read_imagef(I2, bilinear, p2).x-
			read_imagef(I1, int_sampler, ij).x);
	}
}

//Calculates ivar = 1/n sum(Ri^2(v+1)/(v+Ri^2)) iteratively for v = 5
__kernel void calcVariance1(__global float*R, int len, __global float*ivar, __local float*tmp, __local int*tmp2, __global float*partsum, __global int*partcount) {
  float sum = 0;
	int count = 0;
	for (int i = get_global_id(0); i < len; i += get_global_size(0)) {
		if (R[i] == 12345.f) 
			count++;
		else {
			float rr = R[i]*R[i];
			sum += rr/(5.f+rr*ivar[0]);
		}
	}

  int li = get_local_id(0);
  tmp[li] = sum;
	tmp2[li] = count;
  barrier(CLK_LOCAL_MEM_FENCE);

  for (int offset = get_local_size(0)>>1; offset > 0; offset >>= 1) {
    if (li < offset) {
			tmp[li] += tmp[li+offset];
			tmp2[li] += tmp2[li+offset];
		}
    barrier(CLK_LOCAL_MEM_FENCE);
  }
  if (li == 0) {
    partsum[get_group_id(0)] = tmp[0];
		partcount[get_group_id(0)] = tmp2[0];
	}
}

__kernel void calcVariance2(__global float*ivar, __global float*partsum, __global int*partcount, int len) {
	float sum = 0;
	int count = 0;
	for (int i = 0; i < len; i++)
		sum += partsum[i], count += partcount[i];
	ivar[0] = count/(6.*sum);
}

__kernel void calcLinEq1(__global float*R, __global float*J, __global float*ivar, int len, __local float*localA, __local float*localb, __global float*partA, __global float*partb) {
	int li = get_local_id(0);
	__local float*myA = localA+li*21;
	__local float*myb = localb+li*6;
	for (int i = 0; i < 21; i++) myA[i] = 0;
	for (int i = 0; i < 6; i++) myb[i] = 0;
	for (int i = get_global_id(0); i < len; i += get_global_size(0)) {
		if (R[i] != 12345.f) {
			float rr = R[i]*R[i];
			float W = 6./(5.f+rr*ivar[0]);
			int c = 0;
			for (int l = 0; l < 6; l++) {
				float JiW = J[i*6+l]*W;
				for (int k = 0; k <= l; k++)
					myA[c++] += JiW*J[i*6+k];
				myb[l] -= JiW*R[i];
			}
		}
	}

	barrier(CLK_LOCAL_MEM_FENCE);
  for (int offset = get_local_size(0)>>1; offset > 0; offset >>= 1) {
    if (li < offset) {
			for (int i = 0; i < 21; i++) 
				myA[i] += myA[i+offset*21];
			for (int i = 0; i < 6; i++)
				myb[i] += myb[i+offset*6];
		}
    barrier(CLK_LOCAL_MEM_FENCE);
  }
  if (li == 0) {
		int globid = get_group_id(0);
		for (int i = 0; i < 21; i++) 
			partA[globid*21+i] = myA[i];
		for (int i = 0; i < 6; i++)
			partb[globid*6+i] = myb[i];
	}
}

__kernel void calcLinEq2(__global float*partA, __global float*partb, int len, __global float*A, __global float*b) {
	for (int i = 0; i < 36; i++) A[i] = 0;
	for (int i = 0; i < 6; i++) b[i] = 0;
	int c = 0;
	for (int i = 0; i < len; i++) {
		for (int l = 0; l < 6; l++) {
			for (int k = 0; k <= l; k++)
				A[l*6+k] += partA[c++];
			b[l] += partb[i*6+l];
		}
	}
	for (int l = 0; l < 6; l++) 
		for (int k = 0; k < l; k++) 
			A[k*6+l] = A[l*6+k];
}

__kernel void solveLinEq(__global float*A, __global float*x, __global float*r) {
	const int n = 6;
	float p[6];
	float Ap[6];
	
	float rr = 0, eps = 1e-6;
	for (int i = 0; i < n; i++) rr += r[i]*r[i], x[i] = 0, p[i] = r[i];

	if (rr < eps) return;
	int c = 0;
	while (c++ < 6) {
		float pAp = 0;
		for (int i = 0; i < n; i++) {
			Ap[i] = 0;
			for (int j = 0; j < n; j++) 
				Ap[i] += p[j]*A[i*n+j];
			pAp += p[i]*Ap[i];
		}

		float alpha = rr/pAp, rr2 = 0;
		for (int i = 0; i < n; i++) {
			x[i] += alpha*p[i];			
			r[i] -= alpha*Ap[i];
			rr2 += r[i]*r[i];
		}
	
		if (rr2 < eps) return;

		float beta = rr2/rr;
		rr = rr2;

		for (int i = 0; i < n; i++) p[i] = r[i]+beta*p[i];
	}
}
