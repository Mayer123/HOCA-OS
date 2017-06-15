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
#include "../../h/support.e"

#define MAXFRAMES 10

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

int sem_mm, p_alive;
int sem_pf;
int sem_floppy;
int sem_disk;
int sem_page;
int pf_ctr, pf_start;
int Tsysframe[5];
int Tmmframe[5];
int Scronframe, Spagedframe, Sdiskframe;
int Tsysstack[5];
int Tmmstack[5];
int Scronstack, Spagedstack, Sdiskstack;
int mydebug = 0;

framestat frameinfo[MAXFRAMES];
virtualsem vasem[MAXTPROC];
diskreq diskwait[MAXTPROC];

sd_t PDSegtable[32];
pd_t PDpagetable[256];
sd_t DDSegtable[32];
pd_t DDpagetable[256];
char daemonbuffer[512];
int freesector[100][8]; 

static void pagedaemon();

void pageinit(){
    int endframe;
    /* check if you have space for 35 page frames, the system
     has 128K */
    endframe=(int)end / PAGESIZE;
    if (endframe > 220 ) { /* 110 K */
        HALT();
    }
    int k = 2;
    int Spagedframeend;
    int size;
    Tsysframe[0] = endframe + k;k++;
    Tsysstack[0] = (endframe + k)*512 - 2;
    Tsysframe[1] = endframe + k;k++;
    Tsysstack[1] = (endframe + k)*512 - 2;
    Tsysframe[2] = endframe + k;k++;
    Tsysstack[2] = (endframe + k)*512 - 2;
    Tsysframe[3] = endframe + k;k++;
    Tsysstack[3] = (endframe + k)*512 - 2;
    Tsysframe[4] = endframe + k;k++;
    Tsysstack[4] = (endframe + k)*512 - 2;
    Tmmframe[0]  = endframe + k;k++;
    Tmmstack[0]  = (endframe + k)*512 - 2;
    Tmmframe[1]  = endframe + k;k++;
    Tmmstack[1]  = (endframe + k)*512 - 2;
    Tmmframe[2]  = endframe + k;k++;
    Tmmstack[2]  = (endframe + k)*512 - 2;
    Tmmframe[3]  = endframe + k;k++;
    Tmmstack[3]  = (endframe + k)*512 - 2;
    Tmmframe[4]  = endframe + k;k++;
    Tmmstack[4]  = (endframe + k)*512 - 2;
    Scronframe  = endframe + k;k++;
    Scronstack   = (endframe + k)*512 - 2;
    size = 1;
    Spagedframe = endframe + k;k+=size;
    Spagedframeend = Spagedframe - 1 + size;
    Spagedstack  = (endframe + k)*512 - 2;
    Sdiskframe  = endframe + k;k++;
    Sdiskstack   = (endframe + k)*512 - 2;
    pf_start    = (endframe + k+1);
 
  /*
    pf_start = (int)end / PAGESIZE + 1; 
  */
    if (pf_start+MAXFRAMES >= 256){
        HALT();
    }
    sem_pf = MAXFRAMES;
    pf_ctr = 0;

    sem_page = 0;
    sem_disk = 0;
    sem_floppy = 1;
    int i;
    int j;
    for (i = 0; i < 100; i++){
        for (j = 0; j < 8; j++){
            freesector[i][j] = 0;
        }
    }
    for (i = 0; i < MAXFRAMES; i++){
        frameinfo[i].processnum = -1;
        frameinfo[i].page = -1;
        frameinfo[i].segment = -1;
        frameinfo[i].track = -1;
        frameinfo[i].sector = -1;
    }
    for (i = 0; i < MAXTPROC; i++){
        vasem[i].physem = 0;
        vasem[i].vsem = 0;
        diskwait[i].sem = 0;
        diskwait[i].op = -1;
        diskwait[i].track = -1;
        diskwait[i].sector = -1;
    }
    PDSegtable[0].sd_p = 1;
    PDSegtable[0].sd_len = 255;  
    PDSegtable[0].sd_prot = 7;
    PDSegtable[0].sd_pta = &PDpagetable[0];
    DDSegtable[0].sd_p = 1;
    DDSegtable[0].sd_len = 255;  
    DDSegtable[0].sd_prot = 7;
    DDSegtable[0].sd_pta = &DDpagetable[0];
    for (i = 1; i < 32; i++){
        PDSegtable[i].sd_p = 0;
        DDSegtable[i].sd_p = 0;
    }
    for (i = 0; i < 256; i++){
        if (i == 2){
            PDpagetable[i].pd_p = 1;
            PDpagetable[i].pd_r = 1;
            PDpagetable[i].pd_frame = i;
            DDpagetable[i].pd_p = 1;
            DDpagetable[i].pd_r = 1;
            DDpagetable[i].pd_frame = i;
            // what about pageframes?
        }else if (i >= 0x1400/512 && i < 0x1600/512){
            PDpagetable[i].pd_p = 1;
            PDpagetable[i].pd_r = 1;
            PDpagetable[i].pd_frame = i;
            DDpagetable[i].pd_p = 1;
            DDpagetable[i].pd_r = 1;
            DDpagetable[i].pd_frame = i;
        }else if (i >= (int)startt1/512 && i <= (int)etext/512){
            PDpagetable[i].pd_p = 1;
            PDpagetable[i].pd_r = 1;
            PDpagetable[i].pd_frame = i;
            DDpagetable[i].pd_p = 1;
            DDpagetable[i].pd_r = 1;
            DDpagetable[i].pd_frame = i;
        }else if (i >= (int)startd1/512 && i <= (int)edata/512){            
            PDpagetable[i].pd_p = 1;
            PDpagetable[i].pd_r = 1;
            PDpagetable[i].pd_frame = i;
            DDpagetable[i].pd_p = 1;
            DDpagetable[i].pd_r = 1;
            DDpagetable[i].pd_frame = i;
        }else if (i >= (int)startb1/512 && i <= (int)end/512){          
            PDpagetable[i].pd_p = 1;
            PDpagetable[i].pd_r = 1;
            PDpagetable[i].pd_frame = i;
            DDpagetable[i].pd_p = 1;
            DDpagetable[i].pd_r = 1;
            DDpagetable[i].pd_frame = i;
        }else if (i >= Spagedframe && i <= Spagedframeend){
            PDpagetable[i].pd_p = 1;
            PDpagetable[i].pd_r = 1;
            PDpagetable[i].pd_frame = i;
        }else if (i == Sdiskframe){
            DDpagetable[i].pd_p = 1;
            DDpagetable[i].pd_r = 1;
            DDpagetable[i].pd_frame = i;
        }else{
            PDpagetable[i].pd_p = 0;
            PDpagetable[i].pd_r = 1;
            PDpagetable[i].pd_frame = i;
            DDpagetable[i].pd_p = 0;
            DDpagetable[i].pd_r = 1;
            DDpagetable[i].pd_frame = i;
        }
    }

    state_t pagedmon;
    STST(&pagedmon);
    pagedmon.s_sr.ps_m = 1;
    pagedmon.s_sr.ps_int = 0;
    pagedmon.s_pc = (int)pagedaemon;
    pagedmon.s_sp = Spagedstack;
    pagedmon.s_crp = &PDSegtable[0];
    r4 = (int)&pagedmon;
    SYS1();   
    state_t diskdmon;
    STST(&diskdmon);
    diskdmon.s_sr.ps_m = 1;
    diskdmon.s_sr.ps_int = 0;
    diskdmon.s_pc = (int)diskdaemon;
    diskdmon.s_sp = Sdiskstack;
    diskdmon.s_crp = &DDSegtable[0];
    r4 = (int)&diskdmon;
    SYS1();   
}

void dosemop(int op, int *sem){
    vpop semoperation;
    semoperation.op = op;
    semoperation.sem = sem;
    r3 = 1;
    r4 = (int)&semoperation;
    SYS3();    

}

void dosemoptwice(int op1, int *sem1, int op2, int *sem2){
    vpop semoperation[2];
    semoperation[0].op = op1;
    semoperation[0].sem = sem1;
    semoperation[1].op = op2;
    semoperation[1].sem = sem2;
    r3 = 2;
    r4 = (int)&semoperation;
    SYS3();    
}

int getfreeframe(int term, int page, int seg){
    int i;
    int fnum;

    dosemop(LOCK, &sem_pf);
    dosemop(LOCK, &sem_mm);
    int found = 0;
    if (seg == 2){
        for(i = 0; i < MAXFRAMES; i++){
            if(frameinfo[i].processnum != -1 && frameinfo[i].page == page && frameinfo[i].segment == seg){
                fnum = i+pf_start;
                found = 1;
                break;
            }
        }        
    }
    if (found == 0){
        for(i = 0; i < MAXFRAMES; i++){
            if(frameinfo[i].processnum == -1){
                frameinfo[i].processnum = term;
                frameinfo[i].page = page;
                frameinfo[i].segment = seg;
                frameinfo[i].track = -1;
                frameinfo[i].sector = -1;
                fnum = i+pf_start;
                break;
            }
        }
    }
    if (sem_pf <= 2){
        dosemop(UNLOCK, &sem_page);
    }  
    dosemop(UNLOCK, &sem_mm);   
    return (fnum);
} 

void pagein(int term, int page, int seg, int pf){
    int sec, tra, pos;
    dosemop(LOCK, &sem_mm);
    if (seg == 1){
        pos = tp[term].Pagetable[page].pd_frame;
    }
    if (seg == 2){
        pos = sharetable[page].pd_frame;
    }    
    sec = pos & 7;
    tra = ((pos - sec)>>3) & 127;
                           
    //dosemoptwice(UNLOCK, &sem_mm, LOCK, &sem_floppy);
                                vpop semoperation[2];
                                semoperation[0].op = UNLOCK;
                                semoperation[0].sem = &sem_mm;
                                semoperation[1].op = LOCK;
                                semoperation[1].sem = &sem_floppy;
                                r3 = 2;
                                r4 = (int)&semoperation;
                                SYS3();
    dr[11]->d_track = tra;
    dr[11]->d_op = IOSEEK;
    r4 = 11;
    SYS8(); 
    dr[11]->d_badd = (char *)(pf*512);
    dr[11]->d_sect = sec;
    dr[11]->d_op = IOREAD;  
    r4 = 11;
    SYS8();

    //dosemoptwice(UNLOCK, &sem_floppy, LOCK, &sem_mm);
                                //vpop semoperation[2];
                                semoperation[0].op = LOCK;
                                semoperation[0].sem = &sem_mm;
                                semoperation[1].op = UNLOCK;
                                semoperation[1].sem = &sem_floppy;
                                r3 = 2;
                                r4 = (int)&semoperation;
                                SYS3();
    frameinfo[pf-pf_start].sector = sec;
    frameinfo[pf-pf_start].track = tra;

    if (seg == 1){
        tp[term].Pagetable[page].pd_r = 1;
    }
    if (seg == 2){
        sharetable[page].pd_r = 1;
    }
    dosemop(UNLOCK, &sem_mm);   
}

void putframe(int term){
    int i;
    // what about the sharetable
    int sec, tra;
    dosemop(LOCK, &sem_mm);
    for (i = 0; i < MAXFRAMES; i++){
        if (frameinfo[i].processnum == term){
            frameinfo[i].processnum = -1;
            dosemop(UNLOCK, &sem_pf);
        }       
    }
    dosemop(UNLOCK, &sem_mm);
    for (i = 0; i < 32; i++){        
        if (tp[term].Pagetable[i].pd_p == 0 && tp[term].Pagetable[i].pd_r == 0){        
                sec = tp[term].Pagetable[i].pd_frame & 7;
                tra = (tp[term].Pagetable[i].pd_frame>>3) & 127;
                freesector[tra][sec] = 0;  
        }
    }
}

int getfreesector(){
    int i,j;
    int pos;
    for (i = 0; i < 100; i++){
        for (j = 0; j < 8; j++){
            if (freesector[i][j] == 0){
                pos = (i<<3)+j;
                freesector[i][j] = 1;
                return pos;    
                //break; 
            }
        }
    }
    return -1;    
}

void pagedaemon(){
    int i;
    int mark = 4;
    int pos, sec, tra;
    vpop semoperation[2];
    while (p_alive > 0){
        dosemop(LOCK, &sem_page);
        while (sem_pf < mark){
            for (i = 0; i < MAXFRAMES; i++){
                
                if (frameinfo[i].processnum != -1){
                    
                    if (frameinfo[i].segment == 1 && tp[frameinfo[i].processnum].Pagetable[frameinfo[i].page].pd_p == 1){
                        dosemop(LOCK, &sem_mm);
                        if (tp[frameinfo[i].processnum].Pagetable[frameinfo[i].page].pd_r == 1){
                            tp[frameinfo[i].processnum].Pagetable[frameinfo[i].page].pd_r = 0;
                        }else{
                            tp[frameinfo[i].processnum].Pagetable[frameinfo[i].page].pd_p = 0;
                            /*if (tp[frameinfo[i].processnum].Pagetable[frameinfo[i].page].pd_m == 0){
                                pos = (frameinfo[i].track<<3)+frameinfo[i].sector;
                                tp[frameinfo[i].processnum].Pagetable[frameinfo[i].page].pd_frame = pos;
                                frameinfo[i].processnum = -1;  
                                dosemop(UNLOCK, &sem_pf);   
                            }
                            else if (frameinfo[i].track != -1){*/
                            if (frameinfo[i].track != -1){
                                pos = (frameinfo[i].track<<3)+frameinfo[i].sector;
                                tp[frameinfo[i].processnum].Pagetable[frameinfo[i].page].pd_frame = pos;
                                //dosemoptwice(UNLOCK, &sem_mm, LOCK, &sem_floppy);
                                //vpop semoperation[2];
                                semoperation[0].op = UNLOCK;
                                semoperation[0].sem = &sem_mm;
                                semoperation[1].op = LOCK;
                                semoperation[1].sem = &sem_floppy;
                                r3 = 2;
                                r4 = (int)&semoperation;
                                SYS3();    
                                dr[11]->d_track = frameinfo[i].track;
                                dr[11]->d_op = IOSEEK;
                                r4 = 11;
                                mydebug = 1;
                                SYS8();
                                dr[11]->d_badd = (char *)((i+pf_start)*512);
                                dr[11]->d_sect = frameinfo[i].sector;
                                dr[11]->d_op = IOWRITE;
                                r4 = 11;
                                SYS8(); 
                                //dosemoptwice(UNLOCK, &sem_floppy, LOCK, &sem_mm);
                                //vpop semoperation[2];
                                semoperation[0].op = LOCK;
                                semoperation[0].sem = &sem_mm;
                                semoperation[1].op = UNLOCK;
                                semoperation[1].sem = &sem_floppy;
                                r3 = 2;
                                r4 = (int)&semoperation;
                                SYS3();
                                if (frameinfo[i].processnum != -1){
                                    frameinfo[i].processnum = -1;  
                                    dosemop(UNLOCK, &sem_pf);      
                                }                                                         
                            }
                            else{
                                pos = getfreesector();
                                sec = pos & 7;
                                tra = ((pos - sec)>>3) & 127;
                                tp[frameinfo[i].processnum].Pagetable[frameinfo[i].page].pd_frame = pos;
                                //dosemoptwice(UNLOCK, &sem_mm, LOCK, &sem_floppy);
                                //vpop semoperation[2];
                                semoperation[0].op = UNLOCK;
                                semoperation[0].sem = &sem_mm;
                                semoperation[1].op = LOCK;
                                semoperation[1].sem = &sem_floppy;
                                r3 = 2;
                                r4 = (int)&semoperation;
                                SYS3();
                                dr[11]->d_track = tra;
                                dr[11]->d_op = IOSEEK;
                                r4 = 11;
                                //mydebug = 1;
                                SYS8();
                                dr[11]->d_badd = (char *)((i+pf_start)*512);
                                dr[11]->d_sect = sec;
                                dr[11]->d_op = IOWRITE;
                                r4 = 11;
                                SYS8(); 
                                //dosemoptwice(UNLOCK, &sem_floppy, LOCK, &sem_mm);
                                //vpop semoperation[2];
                                semoperation[0].op = LOCK;
                                semoperation[0].sem = &sem_mm;
                                semoperation[1].op = UNLOCK;
                                semoperation[1].sem = &sem_floppy;
                                r3 = 2;
                                r4 = (int)&semoperation;
                                SYS3();
                                if (frameinfo[i].processnum != -1){
                                    frameinfo[i].processnum = -1;  
                                    dosemop(UNLOCK, &sem_pf);      
                                }     
                            }                    
                        }
                        dosemop(UNLOCK, &sem_mm);
                    }
                    if (frameinfo[i].segment == 2 && sharetable[frameinfo[i].page].pd_p == 1){
                        dosemop(LOCK, &sem_mm);
                        if (sharetable[frameinfo[i].page].pd_r == 1){
                            sharetable[frameinfo[i].page].pd_r = 0;
                        }else{
                            sharetable[frameinfo[i].page].pd_p = 0;
                            /*if(sharetable[frameinfo[i].page].pd_m == 0){
                                pos = (frameinfo[i].track<<3)+frameinfo[i].sector;
                                sharetable[frameinfo[i].page].pd_frame = pos;
                                frameinfo[i].processnum = -1;
                                dosemop(UNLOCK, &sem_pf);    
                            }
                            else if (frameinfo[i].track != -1){*/
                            if (frameinfo[i].track != -1){
                                pos = (frameinfo[i].track<<3)+frameinfo[i].sector;
                                sharetable[frameinfo[i].page].pd_frame = pos;
                                //dosemoptwice(UNLOCK, &sem_mm, LOCK, &sem_floppy);
                                //vpop semoperation[2];
                                semoperation[0].op = UNLOCK;
                                semoperation[0].sem = &sem_mm;
                                semoperation[1].op = LOCK;
                                semoperation[1].sem = &sem_floppy;
                                r3 = 2;
                                r4 = (int)&semoperation;
                                SYS3();
                                dr[11]->d_track = frameinfo[i].track;
                                dr[11]->d_op = IOSEEK;
                                r4 = 11;
                                SYS8();
                                dr[11]->d_badd = (char *)((i+pf_start)*512);
                                dr[11]->d_sect = frameinfo[i].sector;
                                dr[11]->d_op = IOWRITE;
                                r4 = 11;
                                SYS8(); 
                                //dosemoptwice(UNLOCK, &sem_floppy, LOCK, &sem_mm);
                                //vpop semoperation[2];
                                semoperation[0].op = LOCK;
                                semoperation[0].sem = &sem_mm;
                                semoperation[1].op = UNLOCK;
                                semoperation[1].sem = &sem_floppy;
                                r3 = 2;
                                r4 = (int)&semoperation;
                                SYS3();
                                if (frameinfo[i].processnum != -1){
                                    frameinfo[i].processnum = -1;  
                                    dosemop(UNLOCK, &sem_pf);      
                                }                                                                
                            }
                            else{
                                pos = getfreesector();
                                sec = pos & 7;
                                tra = ((pos - sec)>>3) & 127;
                                sharetable[frameinfo[i].page].pd_frame = pos;
                                //dosemoptwice(UNLOCK, &sem_mm, LOCK, &sem_floppy);
                                //vpop semoperation[2];
                                semoperation[0].op = UNLOCK;
                                semoperation[0].sem = &sem_mm;
                                semoperation[1].op = LOCK;
                                semoperation[1].sem = &sem_floppy;
                                r3 = 2;
                                r4 = (int)&semoperation;
                                SYS3();
                                dr[11]->d_track = tra;
                                dr[11]->d_op = IOSEEK;
                                r4 = 11;
                                SYS8();
                                dr[11]->d_badd = (char *)((i+pf_start)*512);
                                dr[11]->d_sect = sec;
                                dr[11]->d_op = IOWRITE;
                                r4 = 11;
                                SYS8(); 
                                //dosemoptwice(UNLOCK, &sem_floppy, LOCK, &sem_mm);
                                //vpop semoperation[2];
                                semoperation[0].op = LOCK;
                                semoperation[0].sem = &sem_mm;
                                semoperation[1].op = UNLOCK;
                                semoperation[1].sem = &sem_floppy;
                                r3 = 2;
                                r4 = (int)&semoperation;
                                SYS3();
                                if (frameinfo[i].processnum != -1){
                                    frameinfo[i].processnum = -1;  
                                    dosemop(UNLOCK, &sem_pf);      
                                }   
                            }            
                        }
                        dosemop(UNLOCK, &sem_mm);
                    }
                    
                }
                
            }
        }           
    }
    SYS2(); 
}
