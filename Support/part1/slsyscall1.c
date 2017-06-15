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
#include "../../h/slsyscall2.e"

register int r2 asm("%d2");
register int r3 asm("%d3");
register int r4 asm("%d4");

void Read_from_Terminal(state_t *told_sys, int index){
	char *addr;
	char *vaddr; 
	vaddr = (char *)told_sys->s_r[3];
	addr = tp[index].buffer;
	dr[index]->d_badd = addr;
	dr[index]->d_op = IOREAD;
	r4 = index;
	SYS8();
	int i;
	if (dr[index]->d_stat == 0 || dr[index]->d_stat == 7){
		if (dr[index]->d_amnt != 0){
			for (i = 0; i < dr[index]->d_amnt; i++){
				*(vaddr+i) = tp[index].buffer[i];
			}
			told_sys->s_r[2] = dr[index]->d_amnt;
		}else{
			// no data available?
			told_sys->s_r[2] = -7;
		}
	}else{
		told_sys->s_r[2] = -dr[index]->d_stat;
	}
}

void Write_to_Terminal(state_t *told_sys, int index){
	int length;
	char *addr;
	char *vaddr; 
	vaddr = (char *)told_sys->s_r[3];
	length = told_sys->s_r[4];
	addr = tp[index].buffer;
	int i;
	for (i = 0; i < length; i++){
		tp[index].buffer[i] = *(vaddr+i);
	}	
	dr[index]->d_badd = addr;
	dr[index]->d_amnt = length;
	dr[index]->d_op = IOWRITE;
	r4 = index;
	SYS8();
	if (dr[index]->d_stat == 0){		
		if (dr[index]->d_amnt != 0){
			told_sys->s_r[2] = dr[index]->d_amnt;
		}else{
			// no data available?
			told_sys->s_r[2] = -7;			
		}
	}else{
		told_sys->s_r[2] = -dr[index]->d_stat;
	}
}

void Delay(state_t *told_sys, int index){
	int duration;
	duration = told_sys->s_r[4];
	ds[index].delaytime = duration;
	STCK(&(ds[index].begintime));
    vpop semoperation[2];
    semoperation[0].op = LOCK;
    semoperation[0].sem = &(ds[index].sem);
    semoperation[1].op = UNLOCK;
    semoperation[1].sem = &cronsem;
    r3 = 2;
    r4 = (int)&semoperation;
    SYS3();
}

void Get_Time_of_Day(state_t *told_sys, int index){
	long time;
	STCK(&time);
	told_sys->s_r[2] = time;
}

void Terminate(state_t *told_sys, int index){
	p_alive--;
	putframe(index);
	if (p_alive == 0){
	    vpop semoperation;
	    semoperation.op = UNLOCK;
	    semoperation.sem = &cronsem;
	    r3 = 1;
	    r4 = (int)&semoperation;	
	    SYS3();
	}
	SYS2();
}