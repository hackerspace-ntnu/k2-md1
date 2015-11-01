#include "../../deps/bat/bat.hpp"

void plot(char*, int, int);
void plot(float*, int, int);
#include "dvo.cpp"

int sw = 1920, sh = 1080;

void openimg(const char*name, char*img) {
	FILE*fp = fopen(name, "r");
	int t1, t2;
	fscanf(fp, "%s %d %d", img, &t1, &t2);
	getc(fp);
	for (int i = 0; i < sw*sh; i++) 
		img[i] = (fgetc(fp)+fgetc(fp)+fgetc(fp))/3;
	fclose(fp);
}

void openzbuf(const char*name, float*zbuf) {
	FILE*fp = fopen(name, "r");
	for (int i = 0; i < sw*sh; i++)
		fscanf(fp, "%f", zbuf+i);
	fclose(fp);
}

MyScreen screen;
Surface sf;
Clock myclock;
int Sw = 800, Sh = 600;

void loop() {
	while (1) {
		while (screen.gotEvent()) {
			Event e = screen.getEvent();
			if (e.type == KeyPress) {
				if (e.key == K_ESCAPE) exit(0);
				return;
			}
		}
		screen.putSurface(sf);
		myclock.tick(30);
	}
}

void plot(float*img, int w, int h) {
	for (int j = 0; j < Sh; j++) 
		for (int i = 0; i < Sw; i++) {
			float x = (i+.5)*1.f*w/Sw-.5, y = (j+.5)*1.f*h/Sh-.5;
			if (x > 0 and y > 0)
				sf.pixels[i+j*Sw] = int(bilin(img, x, y, w)*.5+127)*0x10101;
		}
	loop();	
}

void plot(char*img, int w, int h) {
	for (int j = 0; j < Sh; j++) 
		for (int i = 0; i < Sw; i++) {
			float x = (i+.5)*1.f*w/Sw-.5, y = (j+.5)*1.f*h/Sh-.5;
			sf.pixels[i+j*Sw] = int(bilin(img, x, y, w))*0x10101;
		}
	loop();
}

int main() {
	char*I1 = new char[sw*sh];
	char*I2 = new char[sw*sh];
	float*Z1 = new float[sw*sh];
	openimg("dump1.ppm", I1);
	openimg("dump2.ppm", I2);
	openzbuf("zbufdump1.txt", Z1);

	screen = MyScreen(Sw, Sh);
	sf = Surface(Sw, Sh);

	float rotmat[12];

	timeval ta, tb;
	gettimeofday(&ta, NULL);
	findRotation(I1, Z1, I2, rotmat);
	gettimeofday(&tb, NULL);
	double deltime = (tb.tv_sec-ta.tv_sec+(tb.tv_usec-ta.tv_usec)*.000001);

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 4; j++) 
			cout << rotmat[i*4+j] << ", ";
		cout << endl;
	}

	cout << "Time: " << deltime << endl;

}
