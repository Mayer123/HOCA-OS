/*link descriptor type */
typedef struct proc_link{
	int index;
	struct proc_t *next;
}proc_link;

typedef struct proc_t proc_t;
/* process table entry type */
struct proc_t{
	proc_link p_link[SEMMAX];  /* pointer to the next entries */
	state_t  p_s;  /* processor state of the process */
	int qcount;  /* number of queues containing this entry */
	int *semvec[SEMMAX]; /* vector of active semaphores for this entry */
	proc_t *parent;
	proc_t *child;
	proc_t *sibling;
	int CPUtime;
	long starttime;
	state_t *sysold;
	state_t *sysnew;
	state_t *mmold;
	state_t *mmnew;
	state_t *progold;
	state_t *prognew;
};
