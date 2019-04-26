#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <chrono>
#include <thread>
#ifdef DOUBLE_P
#define float double
#endif
typedef struct part {
	int n;
	float xp,yp,xv,yv;
} part;
float randf() {
	return (float)rand()/(float)RAND_MAX;
}
void part_calcForce_safe(part *p1, part *p2,float dt) {//calculates force between particles, ignores collisions
	float dx,dy,f;
	dx=p2->xp-p1->xp;
	dy=p2->yp-p1->yp;
	//printf("%d and %d\n",p1->n,p2->n);
	if(dx==0&dy==0) {
		printf("collision between %d and %d\n",p1->n,p2->n);
		return ;
	}
	f=(dx*dx+dy*dy)*dt;
	p1->xv+=dx/f;
	p1->yv+=dy/f;
	p2->xv-=dx/f;
	p2->yv-=dy/f;
}
void part_calcForce(part *p1, part *p2,float dt) {//calculates force between particles
	float dx,dy,f;
	dx=p2->xp-p1->xp;
	dy=p2->yp-p1->yp;
	f=(dx*dx+dy*dy)*dt;
	p1->xv+=dx/f;
	p1->yv+=dy/f;
	p2->xv-=dx/f;
	p2->yv-=dy/f;
}
void part_calcForce_mt(part *parts, int nparts, int pstart, int pend, float dt) {
	int p1,p2;
	for(p1=pstart;p1<pend;++p1) {
		for(p2=p1+1;p2<nparts;++p2) {
			//if SAFE_INTER is defined, use the collision safe force fucntion
			#ifdef SAFE_INTER
			part_calcForce_safe(&parts[p1],&parts[p2],dt);
			#else
			part_calcForce(&parts[p1],&parts[p2],dt);
			#endif
		}
	}
}
int main(int argc, char *argv[]) {
	/*if(argc!=7) {
		printf("arguments: nframes xres yres nparts dt nthreads\n");
		exit(0);
	}*/
	#ifdef LINUX
	system("clear");
	#endif
	printf("Initializing...\n");
	FILE *fp=fopen("./out.gsim","wb");
	int nframes=atoi(argv[1]);
	int xr=atoi(argv[2]);;
	int yr=atoi(argv[3]);;
	int nparts=atoi(argv[4]);;
	float dt=atof(argv[5]);
	int nthreads=atoi(argv[6]);
	dt*=60.0;
	srand(0);
	//write simulation info to file
	fwrite(&nframes,sizeof(int),1,fp);
	fwrite(&xr,sizeof(int),1,fp);
	fwrite(&yr,sizeof(int),1,fp);
	fwrite(&nparts,sizeof(int),1,fp);
	//allocate memory for paticles and initialize them
	part *parts=(part*)calloc(nparts,sizeof(part));
	{//initialize parts
		int p;
		for(p=0;p<nparts;++p) {
			parts[p].n=p;
			parts[p].xp=randf()*(xr-1);
			parts[p].yp=randf()*(yr-1);
			parts[p].xv=0;
			parts[p].yv=0;
		}
	}
	{//integrate
		int f,p,p1,p2;
		float nx,ny;
		
		int t;
		std::thread *threads=(std::thread*)calloc(nthreads,sizeof(std::thread));
		int *tRanges=(int*)calloc(sizeof(int),nthreads+1);
		if(nthreads>1) {
			printf("Scheduling threads...\n");
			unsigned long long nInters=0.5*nparts*nparts-0.5*nparts;
			unsigned long long nIntersPerThread=nInters/nthreads;
			unsigned long long count=0;
			int ntd=1;
			for(p1=0;p1<nparts;++p1) {
				for(p2=p1+1;p2<nparts;++p2) {
					count++;
					if(count>ntd*nIntersPerThread) {
						tRanges[ntd]=p1;
						ntd++;
					}
				}
			}
			tRanges[ntd]=nparts;
		}
		auto start=std::chrono::steady_clock::now();
		auto end=std::chrono::steady_clock::now();
		printf("Simulating...\n");
		for(f=0;f<nframes;++f) {//for every frame do
			start=std::chrono::steady_clock::now();
			if(nthreads==1) {
				for(p1=0;p1<nparts;++p1) {//compute the force and apply it to the velocities
					for(p2=p1+1;p2<nparts;++p2) {
						part_calcForce(&parts[p1],&parts[p2],dt);
					}
				}
			}
			else {
				for(t=0;t<nthreads-1;++t) {
					threads[t]=std::thread(part_calcForce_mt,parts,nparts,tRanges[t],tRanges[t+1],dt);
				}
				threads[t]=std::thread(part_calcForce_mt,parts,nparts,tRanges[t],tRanges[t+1],dt);
				for(t=0;t<nthreads;++t) {
					threads[t].join();
				}
			}
			for(p=0;p<nparts;++p) {
				//check if the particles are going to be out of bounds
				nx=parts[p].xp+parts[p].xv;
				ny=parts[p].yp+parts[p].yv;
				//reflect them off the bounds
				if(nx>=xr|nx<=0) {
					parts[p].xv*=-.5;
				}
				if(ny>=yr|ny<=0) {
					parts[p].yv*=-.5;
				}
				//write the positions to the output files
				#ifdef DOUBLE_P
				#undef float
				float xpf=parts[p].xp;
				float ypf=parts[p].yp;
				fwrite(&xpf,sizeof(float),1,fp);
				fwrite(&ypf,sizeof(float),1,fp);
				#define float double
				#else
				fwrite(&parts[p].xp,sizeof(float),1,fp);
				fwrite(&parts[p].yp,sizeof(float),1,fp);
				#endif
				//step them forwards
				parts[p].xp+=parts[p].xv;
				parts[p].yp+=parts[p].yv;
			}
			end = std::chrono::steady_clock::now();
			std::chrono::milliseconds elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			#ifdef WINDOWS
			//printf("frame: %d, frametime: %lu, framerate: %f\r",f,(clock()-start)*1000/CLOCKS_PER_SEC,1000.0/(clock()-start)*1000/CLOCKS_PER_SEC);
			printf("frame:%d, frametime:%llums, framerate: %f\r",f,elapsed.count(),1000.0/elapsed.count());
			#endif
			#ifdef LINUX 
			//printf("\033[2;0Hframe: %d, frametime: %lu, framerate: %f\n",f,(clock()-start)*1000/CLOCKS_PER_SEC,1000.0/(clock()-start)*1000/CLOCKS_PER_SEC);
			printf("\033[3;0Hframe:%d, frametime:%llums\n",f,elapsed.count());
			#endif
		}
	}
	fclose(fp);
	return 0;
}
