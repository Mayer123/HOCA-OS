// THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING
// A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Kaixin Ma

#include "../../h/const.h"
#include "../../h/types.h"
#include "../../h/procq.e"
#include "../../h/vpop.h"
#include "../../h/asl.e"
#include "../../h/util.h"
#include "../../h/syscall.e"
#include "../../h/main.e"
#include "../../h/int.e"

state_t *oldsys;
state_t *oldmm;
state_t *oldprog;

void updatetime(proc_t *head){
	long time;
	STCK(&time);
	head->starttime = time;
}

void caltime(proc_t *head){
	long now;
	STCK(&now);
	int inc = now - head->starttime;
	head->CPUtime += inc;
}

void passup(state_t *old, int type){
	proc_t *head;
	head = headQueue(rq);
	if (head == (proc_t *)ENULL){
		// call panic
	}
	if (type == 2){
		if (head->sysnew != (state_t *)ENULL){
			*head->sysold = *old;
			updatetime(head);
			LDST(head->sysnew);
		}else{
			Terminate_Process(old);
		}
	}
	if (type == 1){
		if(head->mmnew != (state_t *)ENULL){
			*head->mmold = *old;
			updatetime(head);
			LDST (head->mmnew);
		}else{
			Terminate_Process(old);
		}
	}
	if (type == 0){
		if (head->prognew != (state_t *)ENULL){
			*head->progold = *old;
			updatetime(head);
			LDST(head->prognew);
		}else{
			Terminate_Process(old);
		}
	}
}

void static trapsyshandler(){
	proc_t *head;
	head = headQueue(rq);
	caltime(head);
	if (oldsys->s_sr.ps_s != 1 && oldsys->s_tmp.tmp_sys.sys_no < 9){
		oldsys->s_tmp.tmp_pr.pr_typ = PRIVILEGE;
		passup(oldsys,0);
	}
	switch (oldsys->s_tmp.tmp_sys.sys_no) {
		case 1:
			Create_Process(oldsys);
			break;
		case 2:
			Terminate_Process(oldsys);
			break;
		case 3:
			Sem_OP(oldsys);
			break;
		case 4:
			notused(oldsys);
			break;
		case 5:
			Specify_Trap_State_Vector(oldsys);
			break;
		case 6:
			Get_CPU_Time(oldsys);
			break;
		case 7:
			waitforpclock(oldsys);
			break;
		case 8:
			waitforio(oldsys);
			break;
		default:
			passup(oldsys,2);
			break;

	}
	updatetime(head);
	LDST(oldsys);

}

void static trapmmhandler(){
	proc_t *head;
	head = headQueue(rq);
	caltime(head);	
	passup(oldmm, 1);

}

void static trapproghandler(){
	proc_t *head;
	head = headQueue(rq);
	caltime(head);
	passup(oldprog, 0);
}

void trapinit(){
	*(int *)0x008 = (int)STLDMM;
	*(int *)0x00c = (int)STLDADDRESS;
	*(int *)0x010 = (int)STLDILLEGAL;
	*(int *)0x014 = (int)STLDZERO;
	*(int *)0x020 = (int)STLDPRIVILEGE;
	*(int *)0x08c = (int)STLDSYS;
	*(int *)0x94 = (int)STLDSYS9;
	*(int *)0x98 = (int)STLDSYS10;
	*(int *)0x9c = (int)STLDSYS11;
	*(int *)0xa0 = (int)STLDSYS12;
	*(int *)0xa4 = (int)STLDSYS13;
	*(int *)0xa8 = (int)STLDSYS14;
	*(int *)0xac = (int)STLDSYS15;
	*(int *)0xb0 = (int)STLDSYS16;
	*(int *)0xb4 = (int)STLDSYS17;
	*(int *)0x100 = (int)STLDTERM0;
	*(int *)0x104 = (int)STLDTERM1;
	*(int *)0x108 = (int)STLDTERM2;
	*(int *)0x10c = (int)STLDTERM3;
	*(int *)0x110 = (int)STLDTERM4;
	*(int *)0x114 = (int)STLDPRINT0;
	*(int *)0x11c = (int)STLDDISK0;
	*(int *)0x12c = (int)STLDFLOPPY0;
	*(int *)0x140 = (int)STLDCLOCK;
	oldsys = (state_t *)0x930;
	state_t *newareasys;
	newareasys = (state_t *)0x930+1;
	STST(newareasys);
	newareasys->s_pc = (int)trapsyshandler;
	oldmm = (state_t *)0x898;
	state_t *newareamm;
	newareamm = (state_t *)0x898+1;
	STST(newareamm);
	newareamm->s_pc = (int)trapmmhandler;
	oldprog = (state_t *)0x800;
	state_t *newareaprog;
	newareaprog = (state_t *)0x800+1;
	STST(newareaprog);
	newareaprog->s_pc = (int)trapproghandler;

}
