// THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING
// A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Kaixin Ma

#include "../../h/const.h"
#include "../../h/types.h"
#include "../../h/procq.e"
#include "../../h/vpop.h"
#include "../../h/int.e"
#include "../../h/util.h"
#include "../../h/trap.e"
#include "../../h/asl.e"

extern int p1();
state_t temp;
proc_link rq;

void static init(){
	STST(&temp);
	initProc();
	initSemd();
	trapinit();
	intinit();
}

void schedule(){
	proc_t *head;
	head = headQueue(rq);
	if (head == (proc_t *)ENULL){
		intdeadlock();
}else{

	intschedule();
	head = headQueue(rq);
	updatetime(head);
	LDST(&(head->p_s));
}
}

void main()
{
	init();
	proc_t *p = allocProc();
	STST(&p->p_s);
	int stack;
	stack = temp.s_sp;
	if (stack%2 == 1) {
		stack -= 1;
	}
	stack -= 512;
	p->p_s.s_sp = stack;
	p->p_s.s_pc = (int)p1;
	rq.index = -1;
	rq.next = (proc_t *)ENULL;
	// set up processor state 
	insertProc(&rq, p);
	schedule();
}
