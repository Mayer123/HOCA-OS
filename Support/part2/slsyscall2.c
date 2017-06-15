// THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING
// A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Kaixin Ma

#include "../../h/const.h"
#include "../../h/vpop.h"
#include "../../h/util.h"
#include "../../h/support.h"
#include "../../h/procq.e"
#include "../../h/asl.e"
#include "../../h/main.e"
#include "../../h/int.e"
#include "../../h/trap.e"
#include "../../h/page.e"
#include "../../h/support.e"

register int r2 asm("%d2");
register int r3 asm("%d3");
register int r4 asm("%d4");

void V_Virtual_Semaphore(state_t *told_sys, int index){
	int *vaddr;
	int i;
	vaddr = (int *)told_sys->s_r[4];
	// check if vaddr is in segment 2
	if (vaddr >= (int *)(2048*512)){
		if (*vaddr >= 0){
			*vaddr += 1;
		}else{
			for(i = 0; i<MAXTPROC; i++){
				if (vasem[i].vsem == vaddr){
					*vaddr += 1;
					vasem[i].vsem = 0;
		    		vpop semoperation;
		    		semoperation.op = UNLOCK;
		    		semoperation.sem = &vasem[i].physem;
		    		r3 = 1;
		    		r4 = (int)&semoperation;	
		    		SYS3();
		    		break;								
				}
			}
		}
	}else{
		Terminate(told_sys, index);
	}
}

void P_Virtual_Semaphore(state_t *told_sys, int index){
	int *vaddr;
	int i;
	vaddr = (int *)told_sys->s_r[4];
	if (vaddr >= (int *)(2048*512)){
		if (*vaddr >= 1){
			*vaddr -= 1;
		}else{
			for(i = 0; i<MAXTPROC; i++){
				if (vasem[i].vsem == 0){
					*vaddr -= 1;
					vasem[i].vsem = vaddr;
		    		vpop semoperation;
		    		semoperation.op = LOCK;
		    		semoperation.sem = &vasem[i].physem;
		    		r3 = 1;
		    		r4 = (int)&semoperation;	
		    		SYS3();		
		    		break;		
				}
			}
		}
	}else{
		Terminate(told_sys, index);		
	}
}

void Disk_Put(state_t *told_sys, int index){
	char *vaddr; 
	int track, sector;
	vaddr = (char *)told_sys->s_r[3];
	int i;
	for (i = 0; i < 512; i++){
		tp[index].buffer[i] = *(vaddr+i);
	}	
	track = told_sys->s_r[4];
	sector = told_sys->s_r[2];
	diskwait[index].sector = sector;
	diskwait[index].track = track;
	diskwait[index].op = IOWRITE;
	vpop semoperation[2];
	semoperation[0].op = LOCK;
	semoperation[0].sem = &diskwait[index].sem;
	semoperation[1].op = UNLOCK;
	semoperation[1].sem = &sem_disk;
	r3 = 2;
	r4 = (int)&semoperation;	
	SYS3();
	if (dr[7]->d_stat == 0){				
		told_sys->s_r[2] = 512;
	}else{
		told_sys->s_r[2] = -dr[7]->d_stat;
	}
}

void Disk_Get(state_t *told_sys, int index){
	char *vaddr;
	int track, sector; 
	vaddr = (char *)told_sys->s_r[3];
	if ((int *)vaddr < (int *)(1024*512)){
		Terminate(told_sys, index);
	}
	track = told_sys->s_r[4];
	sector = told_sys->s_r[2];
	diskwait[index].sector = sector;
	diskwait[index].track = track;
	diskwait[index].op = IOREAD;
	vpop semoperation[2];
	semoperation[0].op = LOCK;
	semoperation[0].sem = &diskwait[index].sem;
	semoperation[1].op = UNLOCK;
	semoperation[1].sem = &sem_disk;
	r3 = 2;
	r4 = (int)&semoperation;	
	SYS3();
	int i;
	if (dr[7]->d_stat == 0){		
		for (i = 0; i < 512; i++){
			*(vaddr+i) = tp[index].buffer[i];
		}
		told_sys->s_r[2] = 512;
	}else{
		told_sys->s_r[2] = -dr[7]->d_stat;
	}
}

int findnext(int track, int sector, int direction){
	int i;
	int next = -1;
	int distance = 999;
	if(direction == 0){
		for(i = 0; i < MAXTPROC; i++){
			if(diskwait[i].sem == -1){				
				if(diskwait[i].track >= track && diskwait[i].sector >= sector){
					if(((diskwait[i].track - track)*8 + diskwait[i].sector - sector) < distance){
						distance = (diskwait[i].track - track)*8 + diskwait[i].sector - sector;
						next = i;
					}
				}
						
			}
		}
	}
	if(direction == 1){
		for(i = 0; i < MAXTPROC; i++){
			if(diskwait[i].sem == -1){				
				if(diskwait[i].track <= track && diskwait[i].sector <= sector){
					if(((track - diskwait[i].track)*8 + sector - diskwait[i].sector) < distance){
						distance = (track - diskwait[i].track)*8 + sector - diskwait[i].sector;
						next = i;
					}
				}
						
			}
		}
	}
	return next;
}

void diskdaemon(){
	int i;
	int currentsector = 0;
	int currenttrack = 0;
	int direction = 0;
	int index;
	int distance;
	int value;
	while (p_alive > 0){
		vpop semoperation;
		semoperation.op = LOCK;
		semoperation.sem = &sem_disk;
		r3 = 1;
		r4 = (int)&semoperation;
		SYS3();
		//index = findnext(currenttrack, currentsector, direction);
		index = -1;
		distance = 999;
		if(direction == 0){
			for(i = 0; i < MAXTPROC; i++){
				if(diskwait[i].sem == -1){		
					value = (diskwait[i].track - currenttrack)*8 + diskwait[i].sector - currentsector;		
						if(value < distance && value >= 0){
							distance = value;
							index = i;
						}	
				}
			}
		}
		if(direction == 1){
			for(i = 0; i < MAXTPROC; i++){
				if(diskwait[i].sem == -1){	
					value = (currenttrack - diskwait[i].track)*8 + currentsector - diskwait[i].sector;
						if(value < distance && value >= 0){
							distance = value;
							index = i;
						}
							
				}
			}
		}
		if (index == -1){
			direction ^= 1;
			//index = findnext(currenttrack, currentsector, direction);
			distance = 999;
			if(direction == 0){
				for(i = 0; i < MAXTPROC; i++){
					if(diskwait[i].sem == -1){	
						value = (diskwait[i].track - currenttrack)*8 + diskwait[i].sector - currentsector;				
							if(value < distance && value >= 0){
								distance = value;
								index = i;
							}
					}
				}
			}
			if(direction == 1){
				for(i = 0; i < MAXTPROC; i++){
					if(diskwait[i].sem == -1){	
						value = (currenttrack - diskwait[i].track)*8 + currentsector - diskwait[i].sector;			
							if(value < distance){
								distance = value;
								index = i;
							}
					}
				}
			}
		}
		currentsector = diskwait[index].sector;
		currenttrack = diskwait[index].track;
		dr[7]->d_track = diskwait[index].track;
		dr[7]->d_op = IOSEEK;
		r4 = 7;
		SYS8();
		dr[7]->d_badd = tp[index].buffer;
		dr[7]->d_sect = diskwait[index].sector;
		dr[7]->d_op = diskwait[index].op;
		r4 = 7;
		SYS8();
	    semoperation.op = UNLOCK;
	    semoperation.sem = &diskwait[index].sem;
	    r3 = 1;
	    r4 = (int)&semoperation;	
	    SYS3();								
	}
	SYS2();	
}
