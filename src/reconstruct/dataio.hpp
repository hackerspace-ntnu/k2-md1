#include <stdio.h>

void saveZB(float*z, int w, int h, int num) {
  char name[100];
  sprintf(name, "recorded/%03dzb%dx%d.ppm", num, w, h);
  FILE*fp = fopen(name, "w");
  fprintf(fp, "P5\n%d %d\n65535\n", w, h);
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
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

void saveIMG(unsigned char*I, int w, int h, int num) {
  char name[100];
  sprintf(name, "recorded/%03dcol%dx%d.ppm", num, w, h);
  FILE*fp = fopen(name, "w");
  fprintf(fp, "P5\n%d %d\n255\n", w, h);
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      fputc(I[i+j*w], fp);
    }
  }
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
