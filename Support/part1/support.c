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
#include "../../h/slsyscall1.e"
#include "../../h/slsyscall2.e"

register int r2 asm("%d2");
register int r3 asm("%d3");
register int r4 asm("%d4");

extern int end();
extern int start();
extern int endt0();
extern int startt1();
extern int etext();
extern int startd0();
extern int endd0();
extern int startd1();
extern int edata();
extern int startb0();
extern int endb0();
extern int startb1();

int bootcode[] = {	
0x41f90008,
0x00002608,
0x4e454a82,
0x6c000008,
0x4ef90008,
0x0000d1c2,
0x787fb882,
0x6d000008,
0x10bc000a,
0x52486000,
0xffde4e71
};

Tprocmmtable tp[MAXTPROC];
sd_t P1aSegtable[32];
pd_t P1aSegtablept[256];
pd_t sharetable[32];
delaysem ds[MAXTPROC];
statearea sa[MAXTPROC];
int cronsem;

static void p1a();
static void tprocess();
static void cron();
static void slproghandler();
static void slsyshandler();
static void slmmhandler();

p1(){
	
	pageinit();
	// system process have access to all seg 0
	P1aSegtable[0].sd_p = 1;
	P1aSegtable[0].sd_len = 255;  // total size?
	P1aSegtable[0].sd_prot = 7;
	P1aSegtable[0].sd_pta = &P1aSegtablept[0];
	int i;
	int j;
	for (i = 1; i < 32; i++){
		P1aSegtable[i].sd_p = 0;
	}
	for (i = 0; i < 256; i++){
		if (i == 2){
			P1aSegtablept[i].pd_p = 1;
			P1aSegtablept[i].pd_r = 1;
			P1aSegtablept[i].pd_frame = i;
			// what about pageframes?
		}else if (i >= (int)startt1/512 && i <= (int)etext/512){
			P1aSegtablept[i].pd_p = 1;
			P1aSegtablept[i].pd_r = 1;
			P1aSegtablept[i].pd_frame = i;
		}else if (i >= (int)startd1/512 && i <= (int)edata/512){			
			P1aSegtablept[i].pd_p = 1;
			P1aSegtablept[i].pd_r = 1;
			P1aSegtablept[i].pd_frame = i;
		}else if (i >= (int)startb1/512 && i <= (int)end/512){			
			P1aSegtablept[i].pd_p = 1;
			P1aSegtablept[i].pd_r = 1;
			P1aSegtablept[i].pd_frame = i;
		}else if (i == Scronframe){
			P1aSegtablept[i].pd_p = 1;
			P1aSegtablept[i].pd_r = 1;
			P1aSegtablept[i].pd_frame = i;
		}else{
			P1aSegtablept[i].pd_p = 0;
			P1aSegtablept[i].pd_r = 1;
			P1aSegtablept[i].pd_frame = i;
		}
	}
	for (i = 0; i < MAXTPROC; i++){
		ds[i].sem = 0;
		ds[i].delaytime = 0;
		ds[i].begintime = 0;

		tp[i].Seg[0].sd_p = 0;   // can not get seg 0
		tp[i].Seg[1].sd_p = 1;   // its own seg
		tp[i].Seg[2].sd_p = 1;   // shared seg
		tp[i].Seg[1].sd_len = 31;
		tp[i].Seg[2].sd_len = 31;
		tp[i].Seg[1].sd_prot = 7;
		tp[i].Seg[2].sd_prot = 7;
		tp[i].Seg[1].sd_pta = &(tp[i].Pagetable[0]);
		tp[i].Seg[2].sd_pta = &(sharetable[0]);
		tp[i].Segpriv[0].sd_p = 1;
		tp[i].Segpriv[1].sd_p = 1;
		tp[i].Segpriv[2].sd_p = 1;
		tp[i].Segpriv[0].sd_len = 255;
		tp[i].Segpriv[1].sd_len = 31;
		tp[i].Segpriv[2].sd_len = 31;
		tp[i].Segpriv[0].sd_prot = 7;
		tp[i].Segpriv[1].sd_prot = 7;
		tp[i].Segpriv[2].sd_prot = 7;		
		tp[i].Segpriv[0].sd_pta = &(tp[i].Pagepriv[0]);
		tp[i].Segpriv[1].sd_pta = &(tp[i].Pagetable[0]);
		tp[i].Segpriv[2].sd_pta = &(sharetable[0]);
		
		for (j = 3; j < 32; j++){			
			tp[i].Seg[j].sd_p = 0;
			tp[i].Segpriv[j].sd_p = 0;
			
		}
		for (j = 0; j < 32; j++){
			tp[i].Pagetable[j].pd_p = 0;
			sharetable[j].pd_p = 0;
			tp[i].Pagetable[j].pd_r = 1;
			sharetable[j].pd_r = 1;			
		}
		// mark certain areas as not present
		for (j = 0; j < 256; j++){
			if (j == 2){
				tp[i].Pagepriv[j].pd_p = 1;
				tp[i].Pagepriv[j].pd_r = 1;
				tp[i].Pagepriv[j].pd_frame = j;
			}else if (j >= 0x1400/512 && j <= 0x1600/512){
				tp[i].Pagepriv[j].pd_p = 1;
				tp[i].Pagepriv[j].pd_r = 1;
				tp[i].Pagepriv[j].pd_frame = j;
			}else if (j >= (int)startt1/512 && j <= (int)etext/512){
				tp[i].Pagepriv[j].pd_p = 1;
				tp[i].Pagepriv[j].pd_r = 1;
				tp[i].Pagepriv[j].pd_frame = j;
			}else if (j >= (int)startd1/512 && j <= (int)edata/512){			
				tp[i].Pagepriv[j].pd_p = 1;
				tp[i].Pagepriv[j].pd_r = 1;
				tp[i].Pagepriv[j].pd_frame = j;
			}else if (j >= (int)startb1/512 && j <= (int)end/512){			
				tp[i].Pagepriv[j].pd_p = 1;
				tp[i].Pagepriv[j].pd_r = 1;
				tp[i].Pagepriv[j].pd_frame = j;
			}else if (j == Tsysframe[i]){
				tp[i].Pagepriv[j].pd_p = 1;
				tp[i].Pagepriv[j].pd_r = 1;	
				tp[i].Pagepriv[j].pd_frame = j;			
			}else if (j == Tmmframe[i]){
				tp[i].Pagepriv[j].pd_p = 1;
				tp[i].Pagepriv[j].pd_r = 1;
				tp[i].Pagepriv[j].pd_frame = j;
			}else{
				tp[i].Pagepriv[j].pd_p = 0;
				tp[i].Pagepriv[j].pd_r = 1;
				tp[i].Pagepriv[j].pd_frame = j;
			}
		}		
	}
	sem_mm = 1;
	cronsem = 0;
	// load the bootcode
	int bootpage;
	
	for (i = 0; i < MAXTPROC; i++){
		bootpage = getfreeframe(i, 31, 1);
		for (j = 0; j < 11; j++){
			*(((int *)(bootpage*512))+j) = bootcode[j];
		}
		tp[i].Pagetable[31].pd_frame = bootpage;
		tp[i].Pagetable[31].pd_p = 1;
	}
	// pass to p1a
	
	p_alive = MAXTPROC;
	state_t p1_a;
	STST(&p1_a);
	p1_a.s_sr.ps_m = 1;
	p1_a.s_sr.ps_int = 0;
	p1_a.s_pc = (int)p1a;
	p1_a.s_sp = Scronstack;
	p1_a.s_crp = &P1aSegtable[0];
	LDST(&p1_a);
}

void static p1a(){
	//state_t tpro[MAXTPROC];
	state_t tpro;
	STST(&tpro);
	int i;
	for (i = 0; i < MAXTPROC; i++){
		tpro.s_pc = (int)tprocess;
		tpro.s_sp = Tsysstack[i];
		tpro.s_crp = &tp[i].Segpriv[0];
		tpro.s_r[12] = i;
		//STST(&tpro[i]);
		/*tpro[i].s_pc = (int)tprocess;
		tpro[i].s_sp = Tsysstack[i];
		tpro[i].s_crp = &tp[i].Seg[0];
		tpro[i].s_r[12] = i;
		r4 = (int)&(tpro[i]);*/
		r4 = (int)&tpro;
		SYS1();
	}
	cron();
}

void static tprocess(){

	state_t boot;
	STST(&boot);
	int index;
	index = boot.s_r[12];
	STST(&sa[index].tsys_n);
	sa[index].tsys_n.s_pc = (int)slsyshandler;
	sa[index].tsys_n.s_sp = Tsysstack[index];
	sa[index].tsys_n.s_crp = &tp[index].Segpriv[0];
	sa[index].tsys_n.s_r[12] = index;
	STST(&sa[index].tmm_n);
	sa[index].tmm_n.s_pc = (int)slmmhandler;
	sa[index].tmm_n.s_sp = Tmmstack[index];
	sa[index].tmm_n.s_crp = &tp[index].Segpriv[0];
	sa[index].tmm_n.s_r[12] = index;;
	STST(&sa[index].tprog_n);
	sa[index].tprog_n.s_pc = (int)slproghandler;
	sa[index].tprog_n.s_sp = Tsysstack[index];
	sa[index].tprog_n.s_crp = &tp[index].Segpriv[0];
	sa[index].tprog_n.s_r[12] = index;

	r2 = PROGTRAP;
	r3 = (int)&sa[index].tprog_o;
	r4 = (int)&sa[index].tprog_n;
	SYS5();

	r2 = MMTRAP;
	r3 = (int)&sa[index].tmm_o;
	r4 = (int)&sa[index].tmm_n;
	SYS5();

	r2 = SYSTRAP;
	r3 = (int)&sa[index].tsys_o;
	r4 = (int)&sa[index].tsys_n;
	SYS5();

	boot.s_sr.ps_s = 0;
	boot.s_pc = 0x80000+31*PAGESIZE;
	boot.s_crp = &tp[index].Seg[0];
	boot.s_sp = 0x80000+32*PAGESIZE-4;
	LDST(&boot);

}

void static slmmhandler(){

	int index;
	int newpage;
	state_t temp;
	STST(&temp);
	int pagenum, segnum;
	index = temp.s_r[12];
	pagenum = sa[index].tmm_o.s_tmp.tmp_mm.mm_pg;
	segnum = sa[index].tmm_o.s_tmp.tmp_mm.mm_seg;
	
	if (tp[index].Segpriv[segnum].sd_p == 0){
		Terminate(&sa[index].tmm_o, index);
	}else if (segnum == 0){
		Terminate(&sa[index].tmm_o, index);
	}else if (segnum == 1){
		newpage = getfreeframe(index, pagenum, 1);
		if (tp[index].Pagetable[pagenum].pd_r == 0){
			pagein(index, pagenum, 1, newpage);
		}
		vpop semoperation;
    	semoperation.op = LOCK;
    	semoperation.sem = &sem_mm;
    	r3 = 1;
    	r4 = (int)&semoperation;
    	SYS3();
		tp[index].Pagetable[pagenum].pd_frame = newpage;
		tp[index].Pagetable[pagenum].pd_p = 1;
		semoperation.op = UNLOCK;
    	semoperation.sem = &sem_mm;
    	r3 = 1;
    	r4 = (int)&semoperation;
    	SYS3();	

	}else if (segnum == 2){
		newpage = getfreeframe(index, pagenum, 2);
		if (sharetable[pagenum].pd_r == 0){
			pagein(index, pagenum, 2, newpage);
		}	
		vpop semoperation;
    	semoperation.op = LOCK;
    	semoperation.sem = &sem_mm;
    	r3 = 1;
    	r4 = (int)&semoperation;
    	SYS3();
		sharetable[pagenum].pd_frame = newpage;
		sharetable[pagenum].pd_p = 1;
    	semoperation.op = UNLOCK;
    	semoperation.sem = &sem_mm;
    	r3 = 1;
    	r4 = (int)&semoperation;
    	SYS3();		


	}
	LDST(&sa[index].tmm_o);
}

void static slsyshandler(){
	state_t temp;
	int index;
	STST(&temp);
	index = temp.s_r[12];
	switch (sa[index].tsys_o.s_tmp.tmp_sys.sys_no) {
		case 9:
			Read_from_Terminal(&sa[index].tsys_o, index);
			break;
		case 10:
			Write_to_Terminal(&sa[index].tsys_o, index);
			break;
		case 11:
			V_Virtual_Semaphore(&sa[index].tsys_o, index);
			break;
		case 12:
			P_Virtual_Semaphore(&sa[index].tsys_o, index);
			break;
		case 13:
			Delay(&sa[index].tsys_o, index);
			break;
		case 14:
			Disk_Put(&sa[index].tsys_o, index);
			break;
		case 15:
			Disk_Get(&sa[index].tsys_o, index);
			break;
		case 16:
			Get_Time_of_Day(&sa[index].tsys_o, index);
			break;
		case 17:
			Terminate(&sa[index].tsys_o, index);
			break;
	}
	LDST(&sa[index].tsys_o);
}

void static slproghandler(){
	state_t temp;
	int index;
	STST(&temp);
	index = temp.s_r[12];
	Terminate(&sa[index].tprog_o, index);
	LDST(&sa[index].tprog_o);
}

void static cron(){
	int i;
	long ending;
	int work = 0;
	while (p_alive > 0){
		for (i = 0; i < MAXTPROC; i++){
			if (ds[i].sem < 0){
				STCK(&ending);
				if (ending - ds[i].begintime > ds[i].delaytime){
					vpop semoperation;
    				semoperation.op = UNLOCK;
    				semoperation.sem = &ds[i].sem;
    				r3 = 1;
    				r4 = (int)&semoperation;
    				SYS3();
				}
			}
		}
		for (i = 0; i < MAXTPROC; i++){
			if (ds[i].sem < 0){
				work = 1;
			}
		}
		if (work == 1){
			SYS7();
		}else{
			vpop semoperation;
			semoperation.op = LOCK;
			semoperation.sem = &cronsem;
			r3 = 1;
			r4 = (int)&semoperation;
			SYS3();
		}
	}
	SYS2();
}
