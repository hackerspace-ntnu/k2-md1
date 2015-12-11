#include <stdio.h>

void saveZB(float*z, int w, int h, int num) {
	char name[100];
	sprintf(name, "recoded/%03dzb%dx%d.ushort", num, w, h);
	FILE*fp = fopen(name, "w");
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {
			unsigned short s = z[i+j*w]*(1<<14);
			fwrite(&s, 2, 1, fp);
		}
	}
	fclose(fp);
}

void loadZB(float*z, int w, int h, int num) {
	char name[100];
	sprintf(name, "recoded/%03dzb%dx%d.ushort", num, w, h);
	FILE*fp = fopen(name, "r");
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {
			unsigned short s;
			fread(&s, 2, 1, fp);
			z[i+j*w] = s*(1.f/(1<<14));
		}
	}
	fclose(fp);
}

void saveIMG(unsigned char*I, int w, int h, int num) {
	char name[100];
	sprintf(name, "recoded/%03dcol%dx%d.ppm", num, w, h);
	FILE*fp = fopen(name, "w");
	fprintf(fp, "P5\n%d %d\n", w, h);
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {
			fputc(I[i+j*w], fp);
		}
	}
	fclose(fp);
}

void loadIMG(unsigned char*I, int w, int h, int num) {
	char name[100];
	sprintf(name, "recoded/%03dcol%dx%d.ppm", num, w, h);
	FILE*fp = fopen(name, "r");
	fprintf(fp, "P5\n%d %d\n", w, h);
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {
			I[i+j*w] = fgetc(fp);
		}
	}
	fclose(fp);
}
