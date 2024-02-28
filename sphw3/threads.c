#include "threadtools.h"
#include <sys/signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


void fibonacci(int id, int arg) {
    thread_setup(id, arg);

    for (RUNNING->i = 1; ; RUNNING->i++) {
        if (RUNNING->i <= 2) RUNNING->x = RUNNING->y = 1;
        else {
            /* We don't need to save tmp, so it's safe to declare it here. */
            int tmp = RUNNING->y;
            RUNNING->y = RUNNING->x + RUNNING->y;
            RUNNING->x = tmp;
        }
        printf("%d %d\n", RUNNING->id, RUNNING->y);
        sleep(1);

        if (RUNNING->i == RUNNING->arg) {
            thread_exit();
        }
        else {
            thread_yield();
        }
    }
}

void factorial(int id, int arg){
    // TODO
    thread_setup(id, arg);
    for (RUNNING->j = 1; ; RUNNING->j++) {
        if (RUNNING->j == 1) RUNNING->fac_val = 1;
        else RUNNING->fac_val = RUNNING->fac_val*RUNNING->j;
        printf("%d %d\n", RUNNING->id, RUNNING->fac_val);
        sleep(1);

        if (RUNNING->j == RUNNING->arg) {
            thread_exit();
        }
        else {
            thread_yield();
        }
    }
}

void bank_operation(int id, int arg) {
    // TODO
    thread_setup(id, arg);
    for (RUNNING->k = 1; ; RUNNING->k++) {
        if (RUNNING->k == 1){
            lock();
            printf("%d acquired the lock\n",RUNNING->id);
        }
        else if (RUNNING->k == 2){
            int ori_balance = bank.balance;
            if (RUNNING->arg>0) bank.balance+=RUNNING->arg;
            else if (bank.balance >= abs(RUNNING->arg)) bank.balance -= abs(RUNNING->arg);
            printf("%d %d %d\n",RUNNING->id,ori_balance,bank.balance);
        }
        else if (RUNNING->k == 3){
            unlock();
            printf("%d released the lock\n",RUNNING->id);
        }

        sleep(1);
        if (RUNNING->k == 3) {
            thread_exit();
        }
        else {
            thread_yield();
        }
    }
}