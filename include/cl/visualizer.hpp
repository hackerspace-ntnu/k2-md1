#include "../../deps/bat/bat.hpp"

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

void plot(float*img, int w, int h, float f = 1) {
	for (int j = 0; j < Sh; j++) 
		for (int i = 0; i < Sw; i++) {
			float x = (i+.5)*1.f*w/Sw-.5, y = (j+.5)*1.f*h/Sh-.5;
			if (x > 0 and y > 0)
				sf.pixels[i+j*Sw] = int(bilin(img, x, y, w)*f*.5+127)*0x10101;
		}
	loop();	
}

void plot(byte*img, int w, int h) {
	for (int j = 0; j < Sh; j++) 
		for (int i = 0; i < Sw; i++) {
			float x = (i+.5)*1.f*w/Sw-.5, y = (j+.5)*1.f*h/Sh-.5;
			sf.pixels[i+j*Sw] = int(bilin(img, x, y, w))*0x10101;
		}
	loop();
}

void initVisual(int w, int h) {
	Sw = w, Sh = h;
	screen = MyScreen(w, h);
	sf = Surface(w, h);
}
