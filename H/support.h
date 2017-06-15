#include "types.h"

#define	MAXTPROC 2

typedef struct Tprocmmtable
{
	char buffer[512];
	sd_t Seg[32];
	sd_t Segpriv[32];
	pd_t Pagetable[32];
	pd_t Pagepriv[256];
}Tprocmmtable;

typedef struct delaysem
{
	int sem;
	int delaytime;
	long begintime;
}delaysem;

typedef struct statearea
{
	state_t tsys_o;
	state_t tmm_o;
	state_t tprog_o;
	state_t tsys_n;
	state_t tmm_n;
	state_t tprog_n;
}statearea;

typedef struct framestat
{
	int processnum;
	int page;
	int segment;
	int track;
	int sector;
}framestat;

typedef struct virtualsem
{
	int physem;
	int *vsem;
}virtualsem;

typedef struct diskreq
{
	int sem;
	int op;
	int track;
	int sector;
}diskreq;