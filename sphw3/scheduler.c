#include "threadtools.h"
#include <sys/signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * Print out the signal you received.
 * If SIGALRM is received, reset the alarm here.
 * This function should not return. Instead, call siglongjmp(sched_buf, 1).
 */
void sighandler(int signo) {
    // TODO
    
    if (signo == SIGTSTP){
        signal(SIGTSTP,sighandler);
        printf("caught SIGTSTP\n");
    }
    else if (signo == SIGALRM){
        printf("caught SIGALRM\n");
        alarm(timeslice);
    }
    signal(signo,sighandler);
    sigprocmask(SIG_BLOCK, &base_mask , NULL);
    longjmp(sched_buf, 1);
}



/*
 * Prior to calling this function, both SIGTSTP and SIGALRM should be blocked.
 */
void scheduler() {
    // TODO
    int type = setjmp(sched_buf);
    //fprintf(stderr,"wrong %d %d %d %d %d %d\n",bank.lock_owner,rq_current,rq_size,wq_size,ready_queue[rq_current]->id,type);
    // fprintf(stderr,"wrong %d %d %d %d\n",rq_current,rq_size,wq_size,type);
    if (type == 0){
        rq_current = 0;
        sigprocmask(SIG_BLOCK, &base_mask , NULL);
        longjmp(RUNNING->environment,1);
    }
    else {
        if (bank.lock_owner == -1 && wq_size>0){
            ready_queue[rq_size] = waiting_queue[0];
            bank.lock_owner = waiting_queue[0]->id;
            rq_size++;
            for (int i=0; i<wq_size-1; i++) waiting_queue[i]=waiting_queue[i+1];
            waiting_queue[wq_size-1] = NULL;
            wq_size--;
            //longjmp(ready_queue[rq_current]->environment,1);
        }

        if (type == 1){
            rq_current = (rq_current+1)%rq_size;
        }
        if (type == 2){
            waiting_queue[wq_size] = ready_queue[rq_current];
            wq_size++;
            //fprintf(stderr,"wrong %d %d %d %d %d\n",rq_current,rq_size,wq_size,type,ready_queue[rq_current]->id);
            rq_size--;
            if (rq_current!=rq_size){
                ready_queue[rq_current] = ready_queue[rq_size];
                ready_queue[rq_size] = NULL;
            }
            else rq_current=0;
            //fprintf(stderr,"wrong %d %d %d %d %d\n",rq_current,rq_size,wq_size,type,ready_queue[rq_current]->id);

        }
        if (type == 3){
            // printf("%d\n",rq_size);
            free(ready_queue[rq_current]);
            ready_queue[rq_current] = ready_queue[rq_size-1];
            ready_queue[rq_size-1] = NULL;
            rq_size--;
            //fprintf(stderr,"wrong %d %d %d %d\n",rq_current,rq_size,wq_size,type);
            if (rq_current==rq_size) rq_current=0;
            //fprintf(stderr,"wrong %d %d %d %d\n",rq_current,rq_size,wq_size,type);
        }

        if (rq_size==0 && wq_size==0) return;
        //fprintf(stderr,"wrong %d %d %d %d\n",rq_current,rq_size,wq_size,ready_queue[rq_current]->id);
        longjmp(ready_queue[rq_current]->environment,1);
    }
}

    