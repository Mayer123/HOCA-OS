// THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING
// A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Kaixin Ma

#include "../h/const.h"
#include "../h/types.h"
#include "../h/procq.e"
#include "../h/asl.h"
#include "../h/asl.e"

semd_t semdTable[MAXPROC];
semd_t *semd_h;
semd_t *semdFree_h;

int insertBlocked(int *semAdd, proc_t *p){
	// if asl is empty
	semd_t *temp;
	if (semd_h == (semd_t *)ENULL){
		temp = semdFree_h;
		semdFree_h = semdFree_h->s_next;
		temp->s_next = (semd_t *)ENULL;
		semdFree_h->s_prev = (semd_t *)ENULL;
		temp->s_semAdd = semAdd;
		insertProc(& temp->s_link, p);
		int i;
		for (i=0;i<SEMMAX;i++){
			if (p->semvec[i] == (int *)ENULL){
				p->semvec[i] = semAdd;
				break;
			}
		}
		semd_h = temp;
		return FALSE;
	}else{
		semd_t *tempprev;
		temp = semd_h;
		while (temp != (semd_t *)ENULL && temp->s_semAdd != semAdd){
			tempprev = temp;
			temp = temp->s_next;
		}
		// not find in asl
		if (temp == (semd_t *)ENULL){
			if (semdFree_h == (semd_t *)ENULL){
				return TRUE;
			}
			temp = semdFree_h;
			semdFree_h = semdFree_h->s_next;
			temp->s_next = (semd_t *)ENULL;
			if (semdFree_h != (semd_t *)ENULL){
				semdFree_h->s_prev = (semd_t *)ENULL;
			}
			temp->s_semAdd = semAdd;
			insertProc(& temp->s_link, p);
			int i;
			for (i=0;i<SEMMAX;i++){
				if (p->semvec[i] == (int *)ENULL){
					p->semvec[i] = semAdd;
					break;
				}
			}
			tempprev->s_next = temp;
			temp->s_prev = tempprev;
			return FALSE;		
		}else{
			insertProc(& temp->s_link, p);
			int i;
			for (i=0;i<SEMMAX;i++){
				if (p->semvec[i] == (int *)ENULL){
					p->semvec[i] = semAdd;
					break;
				}
			}
			return FALSE;
		}

	}
	return TRUE;
}

proc_t* removeBlocked(int *semAdd){
	proc_t *temp;
	temp = headBlocked(semAdd);
	if (temp == (proc_t *)ENULL){
		return (proc_t *)ENULL;
	}else{
		semd_t *help;
		help = semd_h;
		while (help->s_semAdd != semAdd){
			help = help->s_next;
		}
		proc_t *remove;
		//remove = outProc(& help->s_link, temp);
		remove = removeProc(& help->s_link);
		int i;
		for (i=0;i<SEMMAX;i++){
			if (remove->semvec[i] == semAdd){
				remove->semvec[i] = (int *)ENULL;
				break;
			}
		}
		// decide whether to free this semAdd		
		if (headBlocked(semAdd) == (proc_t *)ENULL){
			//add help to freelist
			if (help->s_prev != (semd_t *)ENULL && help->s_next != (semd_t *)ENULL){
				help->s_prev->s_next = help->s_next;
				help->s_next->s_prev = help->s_prev;
			}else if (help->s_next == (semd_t *)ENULL && help->s_prev != (semd_t *)ENULL){
				help->s_prev->s_next = help->s_next;
			}else if (help->s_next != (semd_t *)ENULL && help->s_prev == (semd_t *)ENULL){
				help->s_next->s_prev = help->s_prev;
				semd_h = help->s_next;
			}else{
				semd_h = help->s_next;
			}
			if (semdFree_h == (semd_t *)ENULL){
				help->s_next = (semd_t *)ENULL;
				help->s_prev = (semd_t *)ENULL;
				help->s_semAdd = (int *)ENULL;
				semdFree_h = help;
			}else{
				help->s_next = semdFree_h;
				semdFree_h->s_prev = help;
				help->s_prev = (semd_t *)ENULL;
				help->s_semAdd = (int *)ENULL;
				semdFree_h = help;
			}	
		}
		return remove;
	}
}


proc_t* outBlocked(proc_t *p){
	int i;
	semd_t *temp;
	proc_t *out;
	out = (proc_t *)ENULL;
	// if no asl
	if (semd_h == (semd_t *)ENULL){
		return (proc_t *)ENULL;
	}
	for (i=0;i<SEMMAX;i++){
		// if is on the queue
		if (p->semvec[i] != (int *)ENULL){
			temp = semd_h;
			while (temp != (semd_t *)ENULL && temp->s_semAdd != p->semvec[i]){
				temp = temp->s_next;
			}
			if (temp == (semd_t *)ENULL){
				return (proc_t *)ENULL;
			}else{
				// remove the p
				out = outProc(& temp->s_link, p);				
				if (out == (proc_t *)ENULL){
					return out;
				}
				if (headBlocked(temp->s_semAdd) == (proc_t *)ENULL){
					//add help to freelist
					if (temp->s_prev != (semd_t *)ENULL && temp->s_next != (semd_t *)ENULL){
						temp->s_prev->s_next = temp->s_next;
						temp->s_next->s_prev = temp->s_prev;
					}else if (temp->s_next == (semd_t *)ENULL && temp->s_prev != (semd_t *)ENULL){
						temp->s_prev->s_next = temp->s_next;
					}else if (temp->s_next != (semd_t *)ENULL && temp->s_prev == (semd_t *)ENULL){
						temp->s_next->s_prev = temp->s_prev;
						semd_h = temp->s_next;
					}else{
						semd_h = temp->s_next;
					}
					if (semdFree_h == (semd_t *)ENULL){
						temp->s_next = (semd_t *)ENULL;
						temp->s_prev = (semd_t *)ENULL;
						temp->s_semAdd = (int *)ENULL;
						semdFree_h = temp;
					}else{
						temp->s_next = semdFree_h;
						semdFree_h->s_prev = temp;
						temp->s_prev = (semd_t *)ENULL;
						temp->s_semAdd = (int *)ENULL;
						semdFree_h = temp;
					}	
				}
				p->semvec[i] = (int *)ENULL;
			}
		}
	}
	return out;
}

proc_t* headBlocked(int *semAdd){
	semd_t *temp;
	temp = semd_h;
	if (temp == (semd_t *)ENULL){
		return (proc_t *)ENULL;
	}
	while (temp != (semd_t *)ENULL && temp->s_semAdd != semAdd){
		temp = temp->s_next;
	}
	if (temp == (semd_t *)ENULL){
		return (proc_t *)ENULL;
	}else{
		return headQueue(temp->s_link);
	}

}

int initSemd(){
	int i;
	for (i=0;i<MAXPROC -1;i++){
		semdTable[i].s_next = &semdTable[i+1];
		semdTable[i+1].s_prev = &semdTable[i];
		semdTable[i].s_semAdd = (int *)ENULL;
		semdTable[i].s_link.index = -1;
		semdTable[i].s_link.next = (proc_t *)ENULL;
	}
	semdTable[0].s_prev = (semd_t *)ENULL;
	semdTable[MAXPROC-1].s_next = (semd_t *)ENULL;
	semdTable[MAXPROC-1].s_semAdd = (int *)ENULL;
	semdTable[MAXPROC-1].s_link.next = (proc_t *)ENULL;
	semdTable[MAXPROC-1].s_link.index = -1;
	semdFree_h = &semdTable[0];
	semd_h = (semd_t *)ENULL;
	return 0;
}

int headASL(){
	if (semd_h == (semd_t *)ENULL){
		return FALSE;
	}else{
		return TRUE;
	}

}
