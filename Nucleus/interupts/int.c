// THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING
// A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Kaixin Ma

#include "../../h/const.h"
#include "../../h/types.h"
#include "../../h/vpop.h"
#include "../../h/util.h"
#include "../../h/procq.e"
#include "../../h/asl.e"
#include "../../h/main.e"
#include "../../h/int.e"
#include "../../h/trap.e"

struct intsave
{
	int stat;
	int amount;
}intsavearr[15];

state_t *oldterminal;
state_t *oldprinter;
state_t *olddisk;
state_t *oldfloppy;
state_t *oldclock;
long timeslice = 500;
devreg_t *dr[15];
int devsem[15];
int count;

int pclock;
#define	PRINT0ADDR		(devreg_t *)0x1450
char normal[] = "nucleus: normal termination";
char deadlock[] = "nucleus: deadlock termination";
devreg_t *p_dr = PRINT0ADDR;

void static intsemop();
void static inthandler();
void static intterminalhandler();
void static intprinterhandler();
void static intdiskhandler();
void static intfloppyhandler();
void static intclockhandler();
void static sleep();

void intinit(){
	oldterminal = (state_t *)0x9c8;
	state_t *newareaterminal;
	newareaterminal = (state_t *)0x9c8+1;
	STST(newareaterminal);
	newareaterminal->s_pc = (int)intterminalhandler;
	newareaterminal->s_sr.ps_int = 7;
	oldprinter = (state_t *)0xa60;
	state_t *newareaprinter;
	newareaprinter = (state_t *)0xa60+1;
	STST(newareaprinter);
	newareaprinter->s_pc = (int)intprinterhandler;
	newareaprinter->s_sr.ps_int = 7;
	olddisk = (state_t *)0xaf8;
	state_t *newareadisk;
	newareadisk = (state_t *)0xaf8+1;
	STST(newareadisk);
	newareadisk->s_pc = (int)intdiskhandler;
	newareadisk->s_sr.ps_int = 7;
	oldfloppy = (state_t *)0xb90;
	state_t *newareafloppy;
	newareafloppy = (state_t *)0xb90+1;
	STST(newareafloppy);
	newareafloppy->s_pc = (int)intfloppyhandler;
	newareafloppy->s_sr.ps_int = 7;
	oldclock = (state_t *)0xc28;
	state_t *newareaclock;
	newareaclock = (state_t *)0xc28+1;
	STST(newareaclock);
	newareaclock->s_pc = (int)intclockhandler;
	newareaclock->s_sr.ps_int = 7;
	int i;
	for (i=0; i<15; i++){		
		devsem[i] = 0;
		dr[i] = (devreg_t *)(0x1400+0x10*i);
	}
	pclock = 0;
	count = 100000/timeslice;
}

void waitforpclock(state_t *oldsys){
	proc_t *head;
	head = headQueue(rq);
	head->p_s = *oldsys;
	intsemop(&pclock,LOCK);
}

void waitforio(state_t *oldsys){
	proc_t *head;
	//head = headQueue(rq);
	int devnum;
	devnum = oldsys->s_r[4];
	if (devsem[devnum] == 1){
		devsem[devnum] -= 1;
		oldsys->s_r[3] = intsavearr[devnum].stat;
		oldsys->s_r[2] = intsavearr[devnum].amount;
	}else{
		head = headQueue(rq);
		head->p_s = *oldsys;
		intsemop(&(devsem[devnum]), LOCK);
		
	}
}

void kprint(char *msg, int len){
	/* set up printer's device registers */
	p_dr->d_amnt = len;
	p_dr->d_badd = msg;
	p_dr->d_stat = -1;
	p_dr->d_op = IOWRITE;		/* start i/o    */
	/* wait for the i/o to complete and check result */
	while (dr[5]->d_stat != 0);
}

void intdeadlock(){
	int i;
	int notempty = 0;
	for (i=0; i<15; i++){
		if (headBlocked(&devsem[i]) != (proc_t *)ENULL){
			notempty = 1;
		}
	}
	if (headBlocked(&pclock) != (proc_t *)ENULL){
		intschedule();
		sleep();
	}else if(notempty == 1){
		sleep();
	}else if (headASL() == FALSE){
		kprint(normal, 27);
		HALT();
	}else{
		kprint(deadlock,29);
		HALT();
	}
}

void intschedule(){
	LDIT(&timeslice);
}

void static inthandler(int devt, int devnum){
	proc_t *tail;
	if (devt == 0){
		if(devsem[devnum] == -1){
			tail = headBlocked(&devsem[devnum]);
			//intsemop(&devsem[devnum], UNLOCK);			
			//tail = rq.next;
			tail->p_s.s_r[3] = dr[devnum]->d_stat;
			tail->p_s.s_r[2] = dr[devnum]->d_amnt;
			intsemop(&devsem[devnum], UNLOCK);
		}else{
			devsem[devnum] += 1;
			intsavearr[devnum].stat = dr[devnum]->d_stat;
			intsavearr[devnum].amount = dr[devnum]->d_amnt;
		}
	}
	if (devt == 1){
		if(devsem[devnum+5] == -1){
			tail = headBlocked(&devsem[devnum+5]);
			//intsemop(&devsem[devnum+5], UNLOCK);
			//tail = rq.next;
			tail->p_s.s_r[3] = dr[devnum+5]->d_stat;
			tail->p_s.s_r[2] = dr[devnum+5]->d_amnt;
			intsemop(&devsem[devnum+5], UNLOCK);
			

		}else{
			devsem[devnum+5] += 1;
			intsavearr[devnum+5].stat = dr[devnum+5]->d_stat;
			intsavearr[devnum+5].amount = dr[devnum+5]->d_amnt;
		}
	}
	if (devt == 2){
		if(devsem[devnum+7] == -1){
			tail = headBlocked(&devsem[devnum+7]);
			//intsemop(&devsem[devnum+7], UNLOCK);
			//tail = rq.next;
			tail->p_s.s_r[3] = dr[devnum+7]->d_stat;
			tail->p_s.s_r[2] = dr[devnum+7]->d_amnt;
			intsemop(&devsem[devnum+7], UNLOCK);
		}else{
			devsem[devnum+7] += 1;
			intsavearr[devnum+7].stat = dr[devnum+7]->d_stat;
			intsavearr[devnum+7].amount = dr[devnum+7]->d_amnt;
		}
	}
	if (devt == 3){
		if(devsem[devnum+11] == -1){
			tail = headBlocked(&devsem[devnum+11]);
			//intsemop(&devsem[devnum+11], UNLOCK);
			//tail = rq.next;
			tail->p_s.s_r[3] = dr[devnum+11]->d_stat;
			tail->p_s.s_r[2] = dr[devnum+11]->d_amnt;
			intsemop(&devsem[devnum+11], UNLOCK);
		}else{
			devsem[devnum+11] += 1;
			intsavearr[devnum+11].stat = dr[devnum+11]->d_stat;
			intsavearr[devnum+11].amount = dr[devnum+11]->d_amnt;
		}
	}
}

void static intterminalhandler(){
	proc_t *head;
	head = headQueue(rq);
	inthandler(0,oldterminal->s_tmp.tmp_int.in_dno);
	if (head == (proc_t *)ENULL){
		schedule();
	}else{
		LDST(oldterminal);
	}
}

void static intprinterhandler(){
	proc_t *head;
	head = headQueue(rq);
	inthandler(1,oldprinter->s_tmp.tmp_int.in_dno);
	if (head == (proc_t *)ENULL){
		schedule();
	}else{
		LDST(oldprinter);
	}
}

void static intdiskhandler(){
	proc_t *head;
	head = headQueue(rq);
	inthandler(2,olddisk->s_tmp.tmp_int.in_dno);
	if (head == (proc_t *)ENULL){
		schedule();
	}else{
		LDST(olddisk);
	}
}

void static intfloppyhandler(){
	proc_t *head;
	head = headQueue(rq);
	inthandler(3,oldfloppy->s_tmp.tmp_int.in_dno);
	if (head == (proc_t *)ENULL){
		schedule();
	}else{
		LDST(oldfloppy);
	}
}

void static intclockhandler(){
	proc_t *head;
	head = headQueue(rq);
	count--;
	if (head != (proc_t *)ENULL){
		head = removeProc(&rq);
		caltime(head);
		head->p_s = *oldclock;
		insertProc(&rq, head);
	}
	if (count == 0){
		count = 100000/timeslice;
		if(headBlocked(&pclock) != (proc_t *)ENULL){
			intsemop(&pclock,UNLOCK);
		}
	}
	schedule();
}

void static intsemop(int *sem, int op){
	proc_t *head;
	proc_t *removed;
	//head = headQueue(rq);
	int before;	
	before = *(sem);
	*(sem) += op;
	if (op == LOCK){
		if (before <= 0){
		    head = removeProc(&rq);
		    if (head != (proc_t *)ENULL){						
				insertBlocked(sem,head);
			}
			schedule();
		}
	}else{
		if(before < 0){
			//proc_t *removed;
			removed = removeBlocked(sem);
			if (removed != (proc_t *)ENULL && removed->qcount == 0){
				insertProc(&rq, removed);
			}
		}
	}
}

void static sleep(){
	asm("	stop	#0x2000");
}
