#include <stdio.h>

void saveZB(float*z, int w, int h, int num) {
  char name[100];
  sprintf(name, "recorded/%03dzb%dx%d.ppm", num, w, h);
  FILE*fp = fopen(name, "w");
  fprintf(fp, "P5\n%d %d\n65535\n", w, h);
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      if (z[i+j*w] > 3990) z[i+j*w] = 0;
      unsigned short s = z[i+j*w]*((1<<14)/1000.f);
      fwrite(&s, 2, 1, fp);
    }
  }
  fclose(fp);
}

void loadZB(float*z, int w, int h, int num) {
  char name[100];
  sprintf(name, "recorded/%03dzb%dx%d.ppm", num, w, h);
  FILE*fp = fopen(name, "r");
  int tmp;
  fscanf(fp, "P5\n%d %d\n%d\n", &w, &h, &tmp);
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      unsigned short s;
      fread(&s, 2, 1, fp);
      z[i+j*w] = s*(1000.f/(1<<14));
    }
  }
  fclose(fp);
}

unsigned char writedata[960*540*3];
void saveIMG(Surface&I, int w, int h, int num) {
  char name[100];
  sprintf(name, "recorded/%03dcol%dx%d.ppm", num, w, h);
  FILE*fp = fopen(name, "w");
  fprintf(fp, "P6\n%d %d\n255\n", w, h);
  
  int c = 0;
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      writedata[c++] = I.pixels[i+j*w].r;
      writedata[c++] = I.pixels[i+j*w].g;
      writedata[c++] = I.pixels[i+j*w].b;
      //fputc(I.pixels[i+j*w].r, fp);
      //fputc(I.pixels[i+j*w].g, fp);
      //fputc(I.pixels[i+j*w].b, fp);
    }
  }
  fwrite(writedata, 1, c, fp);
  
  fclose(fp);
}

void loadIMG(unsigned char*I, int w, int h, int num) {
  char name[100];
  sprintf(name, "recorded/%03dcol%dx%d.ppm", num, w, h);
  FILE*fp = fopen(name, "r");
  int tmp;
  fscanf(fp, "P5\n%d %d\n%d\n", &w, &h, &tmp);
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      I[i+j*w] = fgetc(fp);
    }
  }
  fclose(fp);
}

void loadSurface(Surface&sf, int w, int h, int num) {
  char name[100];
  sprintf(name, "recorded/%03dcol%dx%d.ppm", num, sf.w, sf.h);
  FILE*fp = fopen(name, "r");
  if (!fp) {
    cout << "File not found :" << name << endl;
    return;
  }
  int tmp;
  fscanf(fp, "P6\n%d %d\n%d\n", &w, &h, &tmp);
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      sf.pixels[i+j*sf.w].r = fgetc(fp);
      sf.pixels[i+j*sf.w].g = fgetc(fp);
      sf.pixels[i+j*sf.w].b = fgetc(fp);
    }
  }
  fclose(fp);
}
