 // THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING
// A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Kaixin Ma

#include "../h/const.h"
#include "../h/types.h"
#include "../h/procq.h"

proc_t procTable[MAXPROC];
proc_t *procFree_h; 
char panicbuf[512];

void panic(sp)
register char *sp;
{
	register char *ep = panicbuf;

	while ((*ep++ = *sp++) != '\0')
		;
/*
	HALT();
*/
        asm("	trap	#0");

}

int insertProc(proc_link *tp, proc_t *p){
	int i, oldtailindex;
	oldtailindex = tp->index;
	proc_t *oldtail;
	oldtail = tp->next; 
	for(i=0;i<SEMMAX;i++){
		if(p->p_link[i].index == -1){
			if (oldtail == (proc_t *)ENULL){
				// only one in process queue
				p->p_link[i].index = i;
				p->p_link[i].next = p;
				tp->index = i;
				tp->next = p;
				p->qcount++;
				break;	
			}
			// available link position
			p->p_link[i].index = oldtail->p_link[oldtailindex].index;
			p->p_link[i].next = oldtail->p_link[oldtailindex].next;
			oldtail->p_link[oldtailindex].next = p;
			oldtail->p_link[oldtailindex].index = i;
			p->qcount++;
			tp->index = i;
			tp->next = p;
			break;
		}
		if (i == SEMMAX-1){
			panic("process is already in SEMMAX queues");
		}
	}
	return 0;
}

proc_t* removeProc(proc_link *tp){
	// if this queue is empty
	if (tp->next == (proc_t *)ENULL){
		return (proc_t *)ENULL;
	}
	// if there is only one in the queue
	proc_t *first;
	if (tp->next->p_link[tp->index].next == tp->next){
		first = tp->next;
		first->p_link[tp->index].index = -1;
		first->p_link[tp->index].next = (proc_t *)ENULL;
		tp->index = -1;
		tp->next = (proc_t *)ENULL;
		first->qcount--;
		return first;
	}
	// many in the queue
	first = tp->next->p_link[tp->index].next;
	int firstindex;
	firstindex = tp->next->p_link[tp->index].index;
	tp->next->p_link[tp->index].next = first->p_link[firstindex].next;
	tp->next->p_link[tp->index].index = first->p_link[firstindex].index;
	first->p_link[firstindex].next = (proc_t *)ENULL;
	first->p_link[firstindex].index = -1;
	first->qcount--;
	return first;
}

proc_t* outProc(proc_link *tp, proc_t *p){
	if (tp->next == (proc_t *)ENULL){
		return (proc_t *)ENULL;
	}
	// queue is not null;
	// queue only has one element
	proc_t *out;
	if (tp->next->p_link[tp->index].next == tp->next){
		if (tp->next == p){
			out = tp->next;
			out->p_link[tp->index].index = -1;
			out->p_link[tp->index].next = (proc_t *)ENULL;
			tp->index = -1;
			tp->next = (proc_t *)ENULL;
			out->qcount--;
			return out;
		}else{
			return (proc_t *)ENULL;
		}
	}
	// if there are many in the queue
	int outindex, help, previndex, helper;
	proc_t *outprev;
	outprev = tp->next;
	previndex = tp->index;
	out = tp->next->p_link[tp->index].next;
	outindex = tp->next->p_link[tp->index].index;
	while (out != p && out !=tp->next){
		helper = previndex;
		previndex = outprev->p_link[previndex].index;
		outprev = outprev->p_link[helper].next;
		help = outindex;
		outindex = out->p_link[outindex].index;
		out = out->p_link[help].next;	
	}
	// break when reach tail
	if (out == tp->next){
		// update tail
		if (out == p){
			outprev->p_link[previndex].next = out->p_link[outindex].next;
			outprev->p_link[previndex].index = out->p_link[outindex].index;
			out->p_link[outindex].next = (proc_t *)ENULL;
			out->p_link[outindex].index = -1;
			tp->next = outprev;
			tp->index = previndex;
			out->qcount--;
			return out;
		}else{
			return (proc_t *)ENULL;
		}
	}else{
		// break when find desired process
		outprev->p_link[previndex].next = out->p_link[outindex].next;
		outprev->p_link[previndex].index = out->p_link[outindex].index;
		out->p_link[outindex].next = (proc_t *)ENULL;
		out->p_link[outindex].index = -1;
		out->qcount--;
		return out;
	}

}

proc_t* allocProc(){
	if (procFree_h == (proc_t *)ENULL) {
		return (proc_t *)ENULL;
	}
	proc_t *freeproc;
	freeproc = procFree_h; 
	procFree_h = procFree_h->p_link[0].next;
	freeproc->p_link[0].next = (proc_t *)ENULL;
	freeproc->p_link[0].index = -1;
	return freeproc;
}

int freeProc(proc_t *p){
	proc_t *temp;
	if (procFree_h == (proc_t *)ENULL){
		procFree_h = p;
		
	}else{
		temp = procFree_h;
		while(temp->p_link[0].next != (proc_t *)ENULL){
			temp = temp->p_link[0].next;
		}
		temp->p_link[0].next = p;
	}
	int i;
	for (i=0;i<SEMMAX;i++){
		p->p_link[i].next = (proc_t *)ENULL;
		p->p_link[i].index = -1;
		p->semvec[i] = (int *)ENULL;	
	}
	p->qcount = 0;
	p->CPUtime = 0;
	p->starttime = 0;
	return 0;
}

proc_t* headQueue(proc_link tp){
	if (tp.next == (proc_t *)ENULL){
		return (proc_t *)ENULL;
	}
	return tp.next->p_link[tp.index].next;
}

int initProc(){
	int i,j;
	for (i=0;i<MAXPROC;i++){
		for (j=0;j<SEMMAX;j++){
			procTable[i].p_link[j].next = (proc_t *)ENULL;			
			procTable[i].p_link[j].index = -1;
			procTable[i].semvec[j] = (int *)ENULL;	
		}
		procTable[i].qcount = 0;
		procTable[i].parent = (proc_t *)ENULL;	
		procTable[i].child = (proc_t *)ENULL;	
		procTable[i].sibling = (proc_t *)ENULL;	
		procTable[i].CPUtime = 0;
		procTable[i].starttime = 0;
		procTable[i].sysold = (state_t *)ENULL;	
		procTable[i].sysnew = (state_t *)ENULL;		
		procTable[i].mmold = (state_t *)ENULL;		
		procTable[i].mmnew = (state_t *)ENULL;		
		procTable[i].progold = (state_t *)ENULL;		
		procTable[i].prognew = (state_t *)ENULL;			
	}
	for (i=0;i<MAXPROC-1;i++){
		procTable[i].p_link[0].next = &procTable[i+1];	
		procTable[i].p_link[0].index = 0;
	}
	procTable[MAXPROC-1].p_link[0].next = (proc_t *)ENULL;
	procFree_h = &procTable[0];
	return 0;
}
