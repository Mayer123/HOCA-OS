// THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING
// A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Kaixin Ma

#include "../../h/const.h"
#include "../../h/types.h"
#include "../../h/procq.e"
#include "../../h/vpop.h"
#include "../../h/asl.e"
#include "../../h/util.h"
#include "../../h/main.e"

state_t * help;
void Create_Process(state_t *oldsys){
	proc_t *newproc;
	newproc = allocProc();
	proc_t *head;
	head = headQueue(rq);
	if (newproc == (proc_t *)ENULL){
		oldsys->s_r[2] = -1;
	}else{
		newproc->p_s = *(state_t *)oldsys->s_r[4];
		help = &newproc->p_s;
		if (head->child == (proc_t *)ENULL){
			head->child = newproc;
			newproc->parent = head;
		}else{
			proc_t *bro;
			bro = head->child;
			while (bro->parent == (proc_t *)ENULL){
				bro = bro->sibling;
			}
			bro->sibling = newproc;
			bro->parent = (proc_t *)ENULL;
			newproc->parent = head;
		}	
		insertProc(&rq, newproc);
		oldsys->s_r[2] = 0;
	}
}

void cleansem(proc_t *leader){
	int i;
	for(i=0;i<SEMMAX;i++){
		if (leader->semvec[i] != (int *)ENULL){
			*(leader->semvec[i])+=1;
		}
	}
}

void killgchildren(proc_t *head){
		proc_t helper;
		proc_t *leader;
		leader = head->child;
		proc_t *sub;
		while (leader->sibling != (proc_t *)ENULL){
			helper.sibling = leader->sibling;
			leader->sibling = (proc_t *)ENULL;
			cleansem(leader);
			//leader = outBlocked(leader);
			sub = outBlocked(leader);
			if (sub == (proc_t *)ENULL){
				leader = outProc(&rq, leader);
			}else{
				leader = sub;
			}
			if (leader->child != (proc_t *)ENULL){
				killgchildren(leader);
			}
			freeProc(leader);
			leader = helper.sibling;
		}
		leader->parent = (proc_t *)ENULL;
		cleansem(leader);
		sub = outBlocked(leader);
		if (sub == (proc_t *)ENULL){
			leader = outProc(&rq, leader);
		}else{
			leader = sub;
		}
		if (leader->child != (proc_t *)ENULL){
			killgchildren(leader);
		}
		freeProc(leader);
		head->child = (proc_t *)ENULL;	
}

void killchildren(proc_t *head){
	proc_t *cleaner;
	if (head->child == (proc_t *)ENULL){
		head = outProc(&rq, head);
		freeProc(head);
	}else{
		proc_t helper;
		proc_t *leader;
		proc_t *sub;
		leader = head->child;
		while (leader->sibling != (proc_t *)ENULL){
			helper.sibling = leader->sibling;
			leader->sibling = (proc_t *)ENULL;
			cleansem(leader);
			//leader = outBlocked(leader);
			sub = outBlocked(leader);
			if (sub == (proc_t *)ENULL){
				leader = outProc(&rq, leader);
			}else{
				leader = sub;
			}
			if (leader->child != (proc_t *)ENULL){
				killgchildren(leader);
			}
			freeProc(leader);
			leader = helper.sibling;
		}
		leader->parent = (proc_t *)ENULL;
		cleansem(leader);
		//leader = outBlocked(leader);
		sub = outBlocked(leader);
		if (sub == (proc_t *)ENULL){
			leader = outProc(&rq, leader);
		}else{
			leader = sub;
		}
		if (leader->child != (proc_t *)ENULL){
			killgchildren(leader);
		}
		freeProc(leader);
		head->child = (proc_t *)ENULL;
		head = outProc(&rq, head);
		freeProc(head);		
	}
	schedule();
}

void Terminate_Process(state_t *oldsys){
	proc_t *head;
	head = headQueue(rq);
	if (head->parent == (proc_t *)ENULL && head->sibling == (proc_t *)ENULL){
		killchildren(head);
	}else if (head->sibling == (proc_t *)ENULL){
		// last one in the sibling
		proc_t *leader;
		leader = head->parent->child;
		if (leader == head){
			// only one in the sibling
			head->parent->child = (proc_t *)ENULL;
			head->parent = (proc_t *)ENULL;
			killchildren(head);
		}else{
			while(leader->sibling != head){
				leader = leader->sibling;
			}
			leader->sibling = (proc_t *)ENULL;
			leader->parent = head->parent;
			head->parent = (proc_t *)ENULL;
			killchildren(head);
		}
	}else{
		proc_t *neighbor;
		neighbor = head->sibling;
		while(neighbor->parent == (proc_t *)ENULL){
			neighbor = neighbor->sibling;
		}
		if (neighbor->parent->child == head){
			// first one in the sibling
			neighbor->parent->child = head->sibling;
			head->sibling = (proc_t *)ENULL;
			killchildren(head);
		}else{
			// middle one in the sibling
			neighbor = neighbor->parent->child;
			while(neighbor->sibling != head){
				neighbor = neighbor->sibling;
			}
			neighbor->sibling = head->sibling;
			head->sibling = (proc_t *)ENULL;
			killchildren(head);
		}

	}

}

void Sem_OP(state_t *oldsys){
	proc_t *head;
	head = headQueue(rq);
	int num;
	vpop *addr;
	num = oldsys->s_r[3];
	addr = (vpop *)oldsys->s_r[4];
	int i;
	int before;
	int shouldschedule = 0;
	for (i=0;i<num;i++){
		before = *(addr->sem);
		*(addr->sem) += addr->op;
		if (addr->op == LOCK){
			if (before <= 0){						
				
				//head = removeProc(&rq);
				head->p_s = *oldsys;
				insertBlocked(addr->sem,head);
				shouldschedule = 1;
				//schedule();
			}
		}else{
			if(before < 0){
					proc_t *removed;
					removed = removeBlocked(addr->sem);
					if (removed != (proc_t *)ENULL && removed->qcount == 0){
						insertProc(&rq, removed);
					}
				}
		}
		addr++;
	}
	if (shouldschedule == 1){
		head = removeProc(&rq);
		schedule();
	}
}

void notused(state_t *oldsys){
	HALT();
}

void Specify_Trap_State_Vector(state_t *oldsys){
	proc_t *head;
	head = headQueue(rq);
	int type;
	type = oldsys->s_r[2];
	if (type == 0){
		if (head->progold != (state_t *)ENULL){
			Terminate_Process(oldsys);
		}else{
			head->progold = (state_t *)oldsys->s_r[3];
			head->prognew = (state_t *)oldsys->s_r[4];
		}
	}
	if (type == 1){
		if (head->mmold != (state_t *)ENULL){
			Terminate_Process(oldsys);
		}else{
			head->mmold = (state_t *)oldsys->s_r[3];
			head->mmnew = (state_t *)oldsys->s_r[4];
		}
	}
	if (type == 2){
		if (head->sysold != (state_t *)ENULL){
			Terminate_Process(oldsys);
		}else{
			head->sysold = (state_t *)oldsys->s_r[3];
			head->sysnew = (state_t *)oldsys->s_r[4];
		}
	}
}

void Get_CPU_Time(state_t *oldsys){
	proc_t *head;
	head = headQueue(rq);
	oldsys->s_r[2] = head->CPUtime;	
}