#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
typedef unsigned char rgb[3];
typedef struct ppm {
	int w,h,d;
	unsigned char *pix;
} ppm;
void ppm_init(ppm *img,int w, int h) {
	img->w=w;
	img->h=h;
	img->d=255;
	img->pix=(unsigned char *)calloc(w*h*3,sizeof(char));
}
void ppm_getPix(ppm img, int x, int y, rgb dest) {
	dest[0]=img.pix[3*(y*img.w+x)];
	dest[1]=img.pix[3*(y*img.w+x)+1];
	dest[2]=img.pix[3*(y*img.w+x)+2];
}
void ppm_setPix(ppm img, int x, int y, rgb src) {
	img.pix[3*(y*img.w+x)]=src[0];
	img.pix[3*(y*img.w+x)+1]=src[1];
	img.pix[3*(y*img.w+x)+2]=src[2];
}
int ppm_write_p(ppm *img,char *path) {
	char header[1024]="";
	printf("[INFO] Writing ppm image to %s\n",path);
	int t;
	int w,h,d;
	w=img->w;
	h=img->h;
	FILE* fp=fopen(path,"wb");
	sprintf(header,"P6\n%d %d\n255\n",w,h);
	fwrite(&header[0],sizeof(char),strlen(header),fp);
	fwrite(img->pix,sizeof(char),w*h*3,fp);
	fclose(fp);
	return 0;
}
int ppm_write(ppm img,char *path) {
	printf("[INFO] Writing ppm image to %s\n",path);
	char header[1024]="";
	int t;
	int w,h,d;
	w=img.w;
	h=img.h;
	FILE* fp=fopen(path,"wb");
	sprintf(header,"P6\n%d %d\n255\n",w,h);
	fwrite(&header[0],sizeof(char),strlen(header),fp);
	fwrite(img.pix,sizeof(char),w*h*3,fp);
	fclose(fp);
	return 0;
}
typedef struct part {
	float xp,yp;
} part;
int main() {
	FILE *fp=fopen("./out.gsim","rb");
	int nframes;
	int xr,yr;
	int nparts;
	fread(&nframes,sizeof(int),1,fp);
	fread(&xr,sizeof(int),1,fp);
	fread(&yr,sizeof(int),1,fp);
	fread(&nparts,sizeof(int),1,fp);
	rgb white={255,255,255};
	printf("nframes:%d, xres:%d, yres:%d, nparts:%d\n",nframes,xr,yr,nparts);
	{
		int f,p;
		float xp,yp;
		char path[255];
		ppm img;
		ppm_init(&img,xr,yr);
		printf("%d\n",white[0]);
		for(f=0;f<nframes;++f) {
			memset(img.pix,0,sizeof(char)*img.w*img.h*3);
			for(p=0;p<nparts;++p) {
				fread(&xp,sizeof(float),1,fp);
				fread(&yp,sizeof(float),1,fp);
				ppm_setPix(img,xp,yp,white);
			}
			sprintf(path,"./images/%06d.ppm",f);
			ppm_write(img,path);
		}
	}
	return 0;
}
