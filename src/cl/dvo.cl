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
	int px = (i?i-1:0), nx = (i<get_global_size(0)-1 ? i+1 : i);
	int py = (j?j-1:0), ny = (j<get_global_size(1)-1 ? j+1 : j);
	
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
																			__constant float rotmat[12], 
																			__global float*residual, 
																			__global float*jacobian, 
																			float Icx, 
																			float Icy,
																			float Ifx, 
																			float iIfx) {
	int2 ij = (int2)(get_global_id(0), get_global_id(1));
	int index = ij.x+ij.y*get_global_size(0);

	//Deproject 3d point from I1 depth texture
	float pz1 = read_imagef(Z1, int_sampler, ij).x;

	residual[index] = 12345.f;
	//for (int i = 0; i < 6; i++)
	//	jacobian[index*6+i] = 0;
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
	if (ix >= 0 && iy >= 0 && ix < get_global_size(0)-1 && iy < get_global_size(1)-1) {
		float2 p2 = (float2)(px2+.5f, py2+.5f);

		//Calculate jacobian of I2
		float izz = iz*iz;
		float4 dI2_ = read_imagef(dI2, bilinear, p2);
		float dI2x = dI2_.x*Ifx*.5f;
		float dI2y = dI2_.y*Ifx*.5f;

		vstore8((float8)(dI2x*iz, 
										dI2y*iz, 
										-(dI2x*x+dI2y*y)*izz, 
										-dI2x*x*y*izz-dI2y*(1+y*y*izz), 
										dI2x*(1+x*x*izz)+dI2y*x*y*izz, 
										(dI2y*x-dI2x*y)*iz, 0, 0), index, jacobian);
		/*int index6 = index*6;
		jacobian[index6  ] = dI2x*iz;
		jacobian[index6+1] = dI2y*iz;
		jacobian[index6+2] =-(dI2x*x+dI2y*y)*izz;
		jacobian[index6+3] =-dI2x*x*y*izz-dI2y*(1+y*y*izz);
		jacobian[index6+4] = dI2x*(1+x*x*izz)+dI2y*x*y*izz;
		jacobian[index6+5] = (dI2y*x-dI2x*y)*iz;*/

		//for (int k = 0; k < 6; k++)
		//Ji[k] = bilin(J0+k*wh, px2, py2, Iw);

		residual[index] = (read_imagef(I2, bilinear, p2).x-
			read_imagef(I1, int_sampler, ij).x);
	}
}

//Calculates ivar = 1/n sum(Ri^2(v+1)/(v+(ivar*Ri)^2)) iteratively for v = 5
__kernel void calcVariance1(__global float*R, int len, __global float*ivar, __local float*tmp, __local int*tmp2, __global float*partsum, __global int*partcount) {
  float sum = 0;
	int count = 0;
	for (int i = get_global_id(0); i < len; i += get_global_size(0)) {
		if (R[i] != 12345.f) {
			float rr = R[i]*R[i];
			sum += rr/(5.f+rr*ivar[0]);
			count++;
		}
	}

  int li = get_local_id(0);
  tmp[li] = sum;
	tmp2[li] = count;

  for (int offset = get_local_size(0)>>1; offset > 0; offset >>= 1) {

    barrier(CLK_LOCAL_MEM_FENCE);

    if (li < offset) {
			tmp[li] += tmp[li+offset];
			tmp2[li] += tmp2[li+offset];
		}
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

__kernel void calcLinEq1(__global float*R, __global float2*J, __global float*ivar, int len, __local float2*lds, __global float2*part) {
	float2 my[16];
	for (int i = 0; i < 16; i++) my[i] = (float2)(0,0);
	for (int i = get_global_id(0); i < len; i += get_global_size(0)) {
		if (R[i] != 12345.f) {
			float rr = R[i]*R[i];
			float W = 6./(5.f+rr*ivar[0]);

			float2 Ji0 = J[i*4];
			float2 JiW0 = Ji0*W;
			float2 Ji1 = J[i*4+1];
			float2 JiW1 = Ji1*W;
			float2 Ji2 = J[i*4+2];
			float2 JiW2 = Ji2*W;

			my[0] += JiW0*Ji0.x;
			my[1] += JiW1*Ji0.x;
			my[2] += JiW2*Ji0.x;
			my[3].y += JiW0.y*Ji0.y;
			my[4] += JiW1*Ji0.y;
			my[5] += JiW2*Ji0.y;

			my[6] += JiW1*Ji1.x;
			my[7] += JiW2*Ji1.x;
			my[8].y += JiW1.y*Ji1.y;
			my[9] += JiW2*Ji1.y;

			my[10] += JiW2*Ji2.x;
			my[11].y += JiW2.y*Ji2.y;

			my[12] -= JiW0*R[i];
			my[13] -= JiW1*R[i];
			my[14] -= JiW2*R[i];
		}
	}

	int li = get_local_id(0)*16;
	for (int i = 0; i < 16; i++) lds[li+i] = my[i];
	
  for (int offset = get_local_size(0)>>1<<4; offset > 15; offset >>= 1) {

    barrier(CLK_LOCAL_MEM_FENCE);

    if (li < offset) {
			for (int i = 0; i < 16; i++) 
				lds[li+i] += lds[li+offset+i];
		}
  }

  if (li == 0) {
		int globid = get_group_id(0)*16;
		for (int i = 0; i < 16; i++) 
			part[globid+i] = lds[i];
	}
}

__kernel void calcLinEq2(__global float2*part, int len, __global float2*A, __global float2*b) {
	for (int i = 0; i < 18; i++) A[i] = (float2)(0,0);
	for (int i = 0; i < 3; i++) b[i] = (float2)(0,0);

	for (int i = 0; i < len*16; i += 16) {
		A[0] += part[i];
		A[1] += part[i+1];
		A[2] += part[i+2];
		A[3].y += part[i+3].y;
		A[4] += part[i+4];
		A[5] += part[i+5];

		A[7] += part[i+6];
		A[8] += part[i+7];
		A[10].y += part[i+8].y;
		A[11] += part[i+9];

		A[14] += part[i+10];
		A[17].y += part[i+11].y;

		b[0] += part[i+12];
		b[1] += part[i+13];
		b[2] += part[i+14];
	}
	__global float*fA = A;
	for (int l = 0; l < 6; l++) 
		for (int k = 0; k < l; k++) 
		fA[l*6+k] = fA[k*6+l];
}

__kernel void useEstimate(__global float*A, __global float*b, __global float estimate[6], __constant float confidence[6]) {
	for (int i = 0; i < 6; i++) {
		A[i*7] += confidence[i];
		b[i] += estimate[i]*confidence[i];
	}
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
