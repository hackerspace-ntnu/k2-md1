#include "cldvo.hpp"

typedef unsigned char byte;

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

int main() {
	int w = 1920, h = 1080;
	byte*I1 = new byte[w*h];
	byte*I2 = new byte[w*h];
	float*Z1 = new float[w*h];
	openimg("../tracker/dump1.ppm", I1, w*h);
	openimg("../tracker/dump2.ppm", I2, w*h);
	openzbuf("../tracker/zbufdump1.txt", Z1, w*h);

	Tracker tracker(w, h);
	tracker.update(I1, Z1);
	tracker.update(I2, Z1);
	tracker.update(I1, Z1);
	

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 4; j++) 
			cout << tracker.result[i*4+j] << ", ";
		cout << endl;
	}
}
