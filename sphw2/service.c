#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "util.h"

#define ERR_EXIT(s) perror(s), exit(errno);
#define command_len 200

static unsigned long secret;
static char service_name[MAX_SERVICE_NAME_LEN];

static inline bool is_manager() {
    return strcmp(service_name, "Manager") == 0;
}

void print_not_exist(char *service_name) {
    printf("%s doesn't exist\n", service_name);
}

void print_receive_command(char *service_name, char *cmd) {
    printf("%s has received %s\n", service_name, cmd);
}

void print_spawn(char *parent_name, char *child_name) {
    printf("%s has spawned a new service %s\n", parent_name, child_name);
}

void print_kill(char *target_name, int decendents_num) {
    printf("%s and %d child services are killed\n", target_name, decendents_num);
}

void print_acquire_secret(char *service_a, char *service_b, unsigned long secret) {
    printf("%s has acquired a new secret from %s, value: %lu\n", service_a, service_b, secret);
}

void print_exchange(char *service_a, char *service_b) {
    printf("%s and %s have exchanged their secrets\n", service_a, service_b);
}
void simplify_receive(char *servicename,char *operand,char *print_a,char *print_b){
    scanf("%s",print_a); scanf("%s",print_b); 
    strcat(operand," ");
    strcat(operand,print_a);
    strcat(operand," ");
    strcat(operand,print_b);
    print_receive_command(service_name,operand);
}

service *newnode(pid_t newpid,int new_read_fd,int new_write_fd,char *new_name){
    service *new = (service *)malloc(sizeof(service));
    new->pid = newpid;
    new->read_fd = new_read_fd; 
    new->write_fd = new_write_fd;
    new->next=NULL;
    strcpy(new->name,new_name);
    return new;
}
void insert_node(service **head,pid_t newpid,int new_read_fd,int new_write_fd,char *new_name){
    service *now = *head;
    if (now==NULL) *head = newnode(newpid,new_read_fd,new_write_fd,new_name);
    else{
        while (now->next!=NULL){
            now=now->next;
        }
        now->next = newnode(newpid,new_read_fd,new_write_fd,new_name);
    }
}
int delete_node(service **head,char *target){    
    service *temp = *head;
    service *follow;

    if (*head==NULL) return -1; // don't have child 
    if (strcmp((*head)->name,target)==0){
        *head = (*head)->next;
        close(temp->read_fd); close(temp->write_fd);
        free(temp);
        return 1;
    }    
    while((temp != NULL) && (strcmp(temp->name,target))!=0){ //尋找要刪除的目標
        follow = temp;
        temp = temp->next;
    }
    if(temp == NULL) return -1; // not found
    else{
        follow->next = temp->next;
        close(temp->read_fd); close(temp->write_fd);
        free(temp);
    }
    return 1; // find 
}

int main(int argc, char *argv[]) {
    pid_t pid = getpid();        
    int childnum=0;
    service *head = NULL;
    int pfd[2]; int pfd1[2]; // pfd means parent write, pfd[1] means parent read

    if (argc != 2) {
        fprintf(stderr, "Usage: ./service [service_name]\n");
        return 0;
    }

    srand(pid);
    secret = rand();
    /* 
     * prevent buffered I/O
     * equivalent to fflush() after each stdout
     */
    setvbuf(stdout, NULL, _IONBF, 0);

    strncpy(service_name, argv[1], MAX_SERVICE_NAME_LEN);
    printf("%s has been spawned, pid: %d, secret: %lu\n", service_name, pid, secret);


    if (!is_manager()){
        messenge success_created;
        write(PARENT_WRITE_FD,&success_created,sizeof(messenge));
    }

    while(1){
        if (is_manager()){
            char operand[command_len];
            scanf("%s",operand);
            if (strcmp(operand,"spawn")==0){
                int find_flag=0;
                char parent_service[command_len];
                char new_child_service[command_len];
                scanf("%s",parent_service);
                scanf("%s",new_child_service);

                strcat(operand," ");
                strcat(operand,parent_service);
                strcat(operand," ");
                strcat(operand,new_child_service);

                print_receive_command(service_name,operand);


                if (strcmp(parent_service,"Manager")==0){
                    pid_t new_child_pid;
                    service newone;
                    pipe(pfd); pipe(pfd1);
                    newone.read_fd=pfd1[0];
                    newone.write_fd=pfd[1];
                    if ((newone.pid = fork()) == 0){
                        dup2(pfd1[1],PARENT_WRITE_FD);
                        dup2(pfd[0],PARENT_READ_FD);

                        if (pfd[0]!=3 && pfd[0]!=4) close(pfd[0]);
                        if (pfd[1]!=3 && pfd[1]!=4) close(pfd[1]);
                        if (pfd1[0]!=3 && pfd1[0]!=4) close(pfd1[0]);
                        if (pfd1[1]!=3 && pfd1[1]!=4) close(pfd1[1]);

                        service *tmp = head;
                        for (int i=0; i<childnum; i++){
                            if (tmp->read_fd!=3 && tmp->read_fd!=4) close(tmp->read_fd);
                            if (tmp->write_fd!=3 && tmp->write_fd!=4) close(tmp->write_fd);
                            tmp=tmp->next;
                        }

                        execlp("./service","service",new_child_service,NULL);
                    }
                    else{

                        messenge finish_spawn;
                        read(newone.read_fd,&finish_spawn,sizeof(messenge));
                        find_flag = 1;
                        insert_node(&head,newone.pid,newone.read_fd,newone.write_fd,new_child_service);
                        childnum++;
                        print_spawn(service_name,new_child_service);
                        close(pfd[0]); close(pfd1[1]);
                    }

                }
                else{
                    messenge send_spawn_mes; messenge receive_spawn_mes;
                    send_spawn_mes.type=1; send_spawn_mes.find_flag=0;
                    strcpy(send_spawn_mes.spawn_parent,parent_service);
                    strcpy(send_spawn_mes.spawn_child,new_child_service);
                    service *tmp = head;
                    for (int i=0; i<childnum; i++){
                        write(tmp->write_fd,&send_spawn_mes,sizeof(messenge));      
                        read(tmp->read_fd,&receive_spawn_mes,sizeof(messenge));
                        if (receive_spawn_mes.find_flag==1){
                            find_flag=1; 
                            break;
                        }
                        tmp = tmp->next;    
                    }
                }

                if (find_flag==0) print_not_exist(parent_service);
            }
            else if (strcmp(operand,"kill")==0){
                char killed_service[command_len];
                scanf("%s",killed_service);
                
                strcat(operand," ");
                strcat(operand,killed_service);  
                print_receive_command(service_name,operand);

                if (strcmp(killed_service,service_name)==0){
                    int total_child=0;
                    messenge kill_sendto_child;
                    messenge kill_readfrom_child;
                    kill_sendto_child.kill_flag=1;
                    kill_sendto_child.type=2;

                    for (int i=0; i<childnum; i++){
                        strcpy(kill_sendto_child.kill_target,head->name);
                        write(head->write_fd,&kill_sendto_child,sizeof(messenge));
                        read(head->read_fd,&kill_readfrom_child,sizeof(messenge));
                        wait(NULL);
                        total_child+=kill_readfrom_child.child_num;
                        delete_node(&head,head->name);
                    }
                    print_kill(service_name,total_child);
                    return 0;
                }
                else{
                    int find_flag=0; int total_child=0;
                    messenge kill_sendto_child;
                    messenge kill_readfrom_child;
                    kill_sendto_child.kill_flag=0;
                    kill_sendto_child.type=2;
                    strcpy(kill_sendto_child.kill_target,killed_service);

                    service *tmp=head;
                    for (int i=0; i<childnum; i++){
                        //printf("111\n");
                        write(tmp->write_fd,&kill_sendto_child,sizeof(messenge));
                        read(tmp->read_fd,&kill_readfrom_child,sizeof(messenge));
                        //printf("111\n");
                        if (kill_readfrom_child.find_flag==2){
                            //printf("111\n");
                            //wait(NULL);
                            total_child=kill_readfrom_child.child_num;
                            find_flag=1;
                            if (kill_readfrom_child.if_child==1){
                                wait(NULL);
                                delete_node(&head,killed_service);
                                childnum--;
                            } 
                            break;
                        }
                        tmp=tmp->next;
                    }
                    
                    if (find_flag==1) print_kill(killed_service,total_child);
                    else print_not_exist(killed_service);
                }

            }
            else if (strcmp(operand,"exchange")==0){
                umask(0000);
                char exchanged_service_a[command_len];
                char exchanged_service_b[command_len];
                scanf("%s",exchanged_service_a); 
                scanf("%s",exchanged_service_b); 
                strcat(operand," ");
                strcat(operand,exchanged_service_a);
                strcat(operand," ");
                strcat(operand,exchanged_service_b);
                print_receive_command(service_name,operand);

                char fifo_b_to_a[MAX_FIFO_NAME_LEN],fifo_a_to_b[MAX_FIFO_NAME_LEN];
                strcpy(fifo_a_to_b,exchanged_service_a);
                strcat(fifo_a_to_b,"_to_");
                strcat(fifo_a_to_b,exchanged_service_b);
                strcat(fifo_a_to_b,".fifo");

                strcpy(fifo_b_to_a,exchanged_service_b);
                strcat(fifo_b_to_a,"_to_");
                strcat(fifo_b_to_a,exchanged_service_a);
                strcat(fifo_b_to_a,".fifo");
                mkfifo(fifo_a_to_b,0777); mkfifo(fifo_b_to_a,0777);
                // printf("%s %s\n",fifo_a_to_b,fifo_b_to_a);


                int first_readfd=-1; int second_readfd=-1;
                messenge send_begin_one,receive_begin_one;
                strcpy(send_begin_one.exchange_target_first,exchanged_service_a);
                strcpy(send_begin_one.exchange_target_second,exchanged_service_b);
                strcpy(send_begin_one.fifo_btoa,fifo_b_to_a);
                strcpy(send_begin_one.fifo_atob,fifo_a_to_b);
                send_begin_one.type=3; send_begin_one.find_exchange_num=0;

                if (strcmp(service_name,exchanged_service_a)==0 || strcmp(service_name,exchanged_service_b)==0) send_begin_one.find_exchange_num=1;
                service *tmp=head; 
                for (int i=0; i<childnum; i++){
                    int ori_findnum=send_begin_one.find_exchange_num;
                    write(tmp->write_fd,&send_begin_one,sizeof(messenge));
                    read(tmp->read_fd,&receive_begin_one,sizeof(messenge));
                    // find all 
                    //printf("%d %d\n",ori_findnum,receive_begin_one.find_exchange_num);
                    if (receive_begin_one.find_exchange_num-ori_findnum==1){
                        if (first_readfd==-1) first_readfd = tmp->read_fd;
                        else second_readfd = tmp->read_fd;
                    }
                    else if (receive_begin_one.find_exchange_num-ori_findnum==2){
                        send_begin_one.find_exchange_num+=2;
                        first_readfd = tmp->read_fd;
                        break;
                    }

                    if (receive_begin_one.find_exchange_num==2) break;

                    send_begin_one.find_exchange_num = receive_begin_one.find_exchange_num;
                    tmp=tmp->next;
                }

                if (strcmp(service_name,exchanged_service_a)==0){
                    int fifo_read_fd=open(fifo_b_to_a,O_RDONLY);
                    int fifo_write_fd=open(fifo_a_to_b,O_WRONLY);
                    fifo_secret output,input;
                    output.secret_val = secret;
                    read(fifo_read_fd,&input,sizeof(fifo_secret));
                    secret = input.secret_val;
                    print_acquire_secret(service_name,exchanged_service_b,secret);
                    write(fifo_write_fd,&output,sizeof(fifo_secret));
                    close(fifo_read_fd); close(fifo_write_fd);
                }
                else if (strcmp(service_name,exchanged_service_b)==0){
                    int fifo_write_fd=open(fifo_b_to_a,O_WRONLY);
                    int fifo_read_fd=open(fifo_a_to_b,O_RDONLY);
                    fifo_secret output,input;
                    output.secret_val = secret;
                    write(fifo_write_fd,&output,sizeof(fifo_secret));
                    read(fifo_read_fd,&input,sizeof(fifo_secret));
                    secret = input.secret_val;
                    print_acquire_secret(service_name,exchanged_service_a,secret);
                    close(fifo_read_fd); close(fifo_write_fd);
                }
                //printf("%d %d\n",first_readfd,second_readfd);

                read(first_readfd,&receive_begin_one,sizeof(messenge));
                if (second_readfd!=-1) read(second_readfd,&receive_begin_one,sizeof(messenge));

                // delete fifo file
                unlink(fifo_a_to_b); unlink(fifo_b_to_a);
                print_exchange(exchanged_service_a,exchanged_service_b);
            }
        }
        else{
            messenge receive_from_parent;
            read(PARENT_READ_FD,&receive_from_parent,sizeof(messenge));
            if (receive_from_parent.type==1){ // spawn
                int find_flag=0;
                char operand[command_len];

                strcpy(operand,"spawn");
                strcat(operand," ");
                strcat(operand,receive_from_parent.spawn_parent);
                strcat(operand," ");
                strcat(operand,receive_from_parent.spawn_child);

                print_receive_command(service_name,operand);

            
                if (strcmp(receive_from_parent.spawn_parent,service_name)==0){
                    pid_t new_child_pid;
                    service newone;
                    pipe(pfd); pipe(pfd1);
                    newone.read_fd=pfd1[0];
                    newone.write_fd=pfd[1];
                    if ((newone.pid = fork()) == 0){
                        dup2(pfd1[1],PARENT_WRITE_FD);
                        dup2(pfd[0],PARENT_READ_FD);

                        if (pfd[0]!=3 && pfd[0]!=4) close(pfd[0]);
                        if (pfd[1]!=3 && pfd[1]!=4) close(pfd[1]);
                        if (pfd1[0]!=3 && pfd1[0]!=4) close(pfd1[0]);
                        if (pfd1[1]!=3 && pfd1[1]!=4) close(pfd1[1]);

                        service *tmp = head;
                        for (int i=0; i<childnum; i++){
                            if (tmp->read_fd!=3 && tmp->read_fd!=4) close(tmp->read_fd);
                            if (tmp->write_fd!=3 && tmp->write_fd!=4) close(tmp->write_fd);
                            tmp=tmp->next;
                        }
                        execlp("./service","service",receive_from_parent.spawn_child,NULL);
                    }
                    else{
                        messenge finish_spawn;
                        find_flag=1;
                        read(newone.read_fd,&finish_spawn,sizeof(messenge));
                        insert_node(&head,newone.pid,newone.read_fd,newone.write_fd,receive_from_parent.spawn_child);
                        childnum++;
                        print_spawn(service_name,receive_from_parent.spawn_child);
                        close(pfd[0]); close(pfd1[1]);
                    }  
                }
                else{
                    messenge send_spawn_mes; messenge receive_spawn_mes;
                    send_spawn_mes.type=1; send_spawn_mes.find_flag=0;
                    strcpy(send_spawn_mes.spawn_parent,receive_from_parent.spawn_parent);
                    strcpy(send_spawn_mes.spawn_child,receive_from_parent.spawn_child);
                    service *tmp = head;
                    for (int i=0; i<childnum; i++){
                        write(tmp->write_fd,&send_spawn_mes,sizeof(messenge));      
                        read(tmp->read_fd,&receive_spawn_mes,sizeof(messenge));
                        if (receive_spawn_mes.find_flag==1){
                            find_flag=1; 
                            break;
                        }
                        tmp = tmp->next;    
                    }   
                 
                }
                messenge send_to_parent;
                if (find_flag==1) send_to_parent.find_flag=1;
                else send_to_parent.find_flag=0;
                write(PARENT_WRITE_FD,&send_to_parent,sizeof(messenge));
            }   
            else if (receive_from_parent.type==2){
                
                if (receive_from_parent.kill_flag==1){
                    int total_child=0;    
                    messenge kill_sendto_child;
                    kill_sendto_child.type=2;
                    messenge kill_readfrom_child;
                    kill_sendto_child.kill_flag=1;
                    for (int i=0; i<childnum; i++){
                        strcpy(kill_sendto_child.kill_target,head->name);
                        write(head->write_fd,&kill_sendto_child,sizeof(messenge));
                        read(head->read_fd,&kill_readfrom_child,sizeof(messenge));
                        wait(NULL);
                        total_child+=kill_readfrom_child.child_num;
                        delete_node(&head,head->name);
                    }
                    messenge kill_sendto_parent;
                    kill_sendto_parent.if_child=1;
                    kill_sendto_parent.child_num=total_child+1;
                    kill_sendto_parent.kill_flag=2;
                    kill_sendto_parent.find_flag=2;
                    write(PARENT_WRITE_FD,&kill_sendto_parent,sizeof(messenge));
                    return 0;
                }
                else{
                    char operand[command_len];
                    strcpy(operand,"kill");
                    strcat(operand," ");
                    strcat(operand,receive_from_parent.kill_target);
                    print_receive_command(service_name,operand);

                    messenge kill_sendto_child;
                    messenge kill_readfrom_child;
                    int total_child=0;
                    kill_sendto_child.type=2;
                    if (strcmp(service_name,receive_from_parent.kill_target)==0){
                        kill_sendto_child.kill_flag=1;
                        for (int i=0; i<childnum; i++){
                            strcpy(kill_sendto_child.kill_target,head->name);
                            write(head->write_fd,&kill_sendto_child,sizeof(messenge));
                            read(head->read_fd,&kill_readfrom_child,sizeof(messenge));
                            wait(NULL);
                            total_child+=kill_readfrom_child.child_num;
                            delete_node(&head,head->name);
                        }
                        messenge kill_sendto_parent;
                        kill_sendto_parent.if_child=1;
                        kill_sendto_parent.child_num=total_child;
                        kill_sendto_parent.kill_flag=2;
                        kill_sendto_parent.find_flag=2;
                        write(PARENT_WRITE_FD,&kill_sendto_parent,sizeof(messenge));
                        return 0;
                    }
                    else{
                        int find_flag=0; int total_child=0;
                        kill_sendto_child.kill_flag=0;
                        kill_sendto_child.type=2;
                        strcpy(kill_sendto_child.kill_target,receive_from_parent.kill_target);

                        service *tmp=head;
                        for (int i=0; i<childnum; i++){
                            write(tmp->write_fd,&kill_sendto_child,sizeof(messenge));
                            read(tmp->read_fd,&kill_readfrom_child,sizeof(messenge));
                            if (kill_readfrom_child.find_flag==2){
                                total_child=kill_readfrom_child.child_num;
                                find_flag=1;
                                if (kill_readfrom_child.if_child==1){
                                    wait(NULL);
                                    delete_node(&head,receive_from_parent.kill_target);
                                    childnum--;
                                } 
                                break;
                            }
                            tmp=tmp->next;
                        }
                        messenge kill_sendto_parent;
                        if (find_flag==0){
                            kill_sendto_parent.if_child=0;
                            kill_sendto_parent.child_num=0;
                            kill_sendto_parent.kill_flag=0;
                            kill_sendto_parent.find_flag=0;                        
                        }
                        else{
                            kill_sendto_parent.if_child=0;
                            kill_sendto_parent.child_num=total_child;
                            kill_sendto_parent.kill_flag=2;
                            kill_sendto_parent.find_flag=2;
                        }
                        write(PARENT_WRITE_FD,&kill_sendto_parent,sizeof(messenge));
                    }

                }
            }
            else if (receive_from_parent.type==3){ // begin exchange
                char operand[MAX_CMD_LEN]; messenge exchange_send_parent;
                strcpy(operand,"exchange");
                strcat(operand," ");
                strcat(operand,receive_from_parent.exchange_target_first);
                strcat(operand," ");
                strcat(operand,receive_from_parent.exchange_target_second);
                print_receive_command(service_name,operand);

                int find_num=receive_from_parent.find_exchange_num;
                int first_readfd=-1; int second_readfd=-1;
                exchange_send_parent.find_exchange_num=find_num;
                //printf("%s %d %d\n",service_name,receive_from_parent.find_exchange_num,exchange_send_parent.find_exchange_num);
                if (strcmp(service_name,receive_from_parent.exchange_target_first)==0 || strcmp(service_name,receive_from_parent.exchange_target_second)==0) find_num++;
                if (find_num==2){
                    exchange_send_parent.find_exchange_num=2;
                    //printf("%s %d %d\n",service_name,receive_from_parent.find_exchange_num,exchange_send_parent.find_exchange_num);
                    write(PARENT_WRITE_FD,&exchange_send_parent,sizeof(messenge));
                } 
                else{
                    messenge exchange_send_child,exchange_receive_child; service *tmp=head;
                    exchange_send_child.type=3; exchange_send_child.find_exchange_num=find_num;
                    strcpy(exchange_send_child.exchange_target_first,receive_from_parent.exchange_target_first);
                    strcpy(exchange_send_child.exchange_target_second,receive_from_parent.exchange_target_second);
                    strcpy(exchange_send_child.fifo_atob,receive_from_parent.fifo_atob);
                    strcpy(exchange_send_child.fifo_btoa,receive_from_parent.fifo_btoa);

                    for (int i=0; i<childnum; i++){
                        //printf("%s %s %d %d\n",service_name,tmp->name,receive_from_parent.find_exchange_num,exchange_send_parent.find_exchange_num);
                        int ori_findnum=exchange_send_child.find_exchange_num;
                        write(tmp->write_fd,&exchange_send_child,sizeof(messenge));
                        read(tmp->read_fd,&exchange_receive_child,sizeof(messenge));

                        //printf("%s %s %d %d %d %d\n",service_name,tmp->name,receive_from_parent.find_exchange_num,exchange_send_child.find_exchange_num,exchange_send_parent.find_exchange_num,exchange_receive_child.find_exchange_num);
                        // find all 
                        if (exchange_receive_child.find_exchange_num-ori_findnum==1){
                            if (first_readfd==-1) first_readfd = tmp->read_fd;
                            else second_readfd = tmp->read_fd;
                        }
                        else if (exchange_receive_child.find_exchange_num==2){
                            exchange_send_child.find_exchange_num=2;
                            first_readfd = tmp->read_fd;
                            break;
                        }

                        //printf("%s %s %d %d\n",service_name,tmp->name,receive_from_parent.find_exchange_num,exchange_send_parent.find_exchange_num);
                        exchange_send_child.find_exchange_num = exchange_receive_child.find_exchange_num;
                        exchange_send_parent.find_exchange_num=exchange_send_child.find_exchange_num;
                        //printf("%s %s %d %d\n",service_name,tmp->name,exchange_send_parent.find_exchange_num,exchange_receive_child.find_exchange_num);
                        if (exchange_receive_child.find_exchange_num==2) break;
                        tmp=tmp->next;
                    }
                    //printf("%s %d %d\n",service_name,receive_from_parent.find_exchange_num,exchange_send_parent.find_exchange_num);
                    exchange_send_parent.find_exchange_num=exchange_send_child.find_exchange_num;
                    write(PARENT_WRITE_FD,&exchange_send_parent,sizeof(messenge));
                }
                //printf("%s %d %d %d %d\n",service_name,receive_from_parent.find_exchange_num,exchange_send_parent.find_exchange_num,first_readfd,second_readfd);
                if (strcmp(service_name,receive_from_parent.exchange_target_first)==0){
                    int fifo_write_fd=open(receive_from_parent.fifo_atob,O_RDWR);
                    int fifo_read_fd=open(receive_from_parent.fifo_btoa,O_RDWR);
                    fifo_secret output,input;
                    output.secret_val = secret;
                    read(fifo_read_fd,&input,sizeof(fifo_secret));
                    secret = input.secret_val;
                    print_acquire_secret(service_name,receive_from_parent.exchange_target_second,secret);
                    write(fifo_write_fd,&output,sizeof(fifo_secret));
                    close(fifo_read_fd); close(fifo_write_fd);
                }
                else if (strcmp(service_name,receive_from_parent.exchange_target_second)==0){
                    
                    int fifo_write_fd=open(receive_from_parent.fifo_btoa,O_RDWR);
                    int fifo_read_fd=open(receive_from_parent.fifo_atob,O_RDWR);
                    fifo_secret output,input;
                    output.secret_val = secret;
                    write(fifo_write_fd,&output,sizeof(fifo_secret));
                    read(fifo_read_fd,&input,sizeof(fifo_secret));
                    secret = input.secret_val;
                    print_acquire_secret(service_name,receive_from_parent.exchange_target_first,secret);
                    close(fifo_read_fd); close(fifo_write_fd);
                }
                messenge buf_receive;
                if (first_readfd!=-1){
                    //printf("%s %d %d %d %d\n",service_name,receive_from_parent.find_exchange_num,exchange_send_parent.find_exchange_num,first_readfd,second_readfd);
                    read(first_readfd,&buf_receive,sizeof(messenge));
                    //printf("%s %d %d %d %d\n",service_name,receive_from_parent.find_exchange_num,exchange_send_parent.find_exchange_num,first_readfd,second_readfd);
                } 
                if (second_readfd!=-1){
                    read(second_readfd,&buf_receive,sizeof(messenge));
                } 
                if (receive_from_parent.find_exchange_num<exchange_send_parent.find_exchange_num ){
                    write(PARENT_WRITE_FD,&buf_receive,sizeof(messenge));
                    //printf("%s %d %d %d %d\n",service_name,receive_from_parent.find_exchange_num,exchange_send_parent.find_exchange_num,first_readfd,second_readfd);
                }
                
            }
        }
    }

    return 0;
}