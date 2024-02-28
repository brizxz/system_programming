#include "hw1.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)
#define BUFFER_SIZE 4096

typedef struct {
    char* ip; // server's ip
    unsigned short port; // server's port
    int conn_fd; // fd to talk with server
    char buf[BUFFER_SIZE]; // data sent by/to server
    size_t buf_len; // bytes used by buf
} client;

client cli;
static void init_client(char** argv);

int main(int argc, char** argv){
    
    record record_buf; 
    char std_buffer[BUFFER_SIZE];
    char oper_buffer[16]="!pull";
    // Parse args.
    if(argc!=3){
        ERR_EXIT("usage: [ip] [port]");
    }

    // Handling connection
    init_client(argv);
    fprintf(stderr, "connect to %s %d\n", cli.ip, cli.port);
    printf("==============================\n"
           "Welcome to CSIE Bulletin board\n"
           "==============================\n");

    fflush(stdout);

    memset(record_buf.Content,'\0',sizeof(CONTENT_LEN));
    memset(record_buf.From,'\0',sizeof(FROM_LEN));
    strcpy(record_buf.Content,"!pull");

    int len = 0;
    while(1){
        len += write(cli.conn_fd,&record_buf,sizeof(record));
        if (len==sizeof(record)){
            break;
        }
    }   

    for (int i=0; i<RECORD_NUM; i++){
        len=0;
        while(1){
            len += read(cli.conn_fd,&record_buf,sizeof(record));
            if (len==sizeof(record)){
                break;
            }
        }

        if ((record_buf.Content[0])=='!' && (record_buf.From[0])=='!') continue;
        printf("FROM: ");
        fflush(stdout);
        for (int i=0; i<strlen(record_buf.From); i++) printf("%c",record_buf.From[i]);
        fflush(stdout);       

        printf("\nCONTENT:\n");
        fflush(stdout);
        
        for (int i=0; i<strlen(record_buf.Content); i++) printf("%c",record_buf.Content[i]);
        printf("\n");
        fflush(stdout); 

    }
    
    printf("==============================\n");
    fflush(stdout);

    while(1){   
        printf("Please enter your command (post/pull/exit): "); 
        fflush(stdout);
        scanf("%s",oper_buffer+1);  

        memset(record_buf.Content,'\0',sizeof(CONTENT_LEN));
        memset(record_buf.From,'\0',sizeof(FROM_LEN));
        strcpy(record_buf.Content,oper_buffer);

        len=0;
        while(1){
            len += write(cli.conn_fd,&record_buf,sizeof(record));
            if (len==sizeof(record)){
                break;
            }
        }

        if (strcmp(record_buf.Content,"!post")==0){

            record judge={"",""};
            len=0;
            while(1){
                len += read(cli.conn_fd,&judge,sizeof(record));
                if (len==sizeof(record)){
                    break;
                }
            }


            if (strcmp(judge.Content,"yes")==0){
                memset(record_buf.Content,'\0',sizeof(CONTENT_LEN));
                memset(record_buf.From,'\0',sizeof(FROM_LEN));

                printf("FROM: ");
                fflush(stdout);
                scanf("%s",record_buf.From);
               
                printf("CONTENT:\n");
                fflush(stdout);
                scanf("%s",record_buf.Content);
                
                write(cli.conn_fd,&record_buf,sizeof(record));
            }
            else{
                printf("[Error] Maximum posting limit exceeded\n");
                fflush(stdout);
            }

        }
        else if (strcmp(record_buf.Content,"!pull")==0){

            printf("==============================\n");
            fflush(stdout);

            for (int i=0; i<RECORD_NUM; i++){


                len=0;
                while(1){
                    len += read(cli.conn_fd,&record_buf,sizeof(record));
                    if (len==sizeof(record)){
                        break;
                    }
                }
                if ((record_buf.Content[0])=='!' && (record_buf.From[0])=='!') continue;
                printf("FROM: ");
                fflush(stdout);
                for (int i=0; i<strlen(record_buf.From); i++){
                    printf("%c",record_buf.From[i]);
                    fflush(stdout);
                } 
                printf("\n");
                fflush(stdout);       
                
                printf("CONTENT:\n");
                fflush(stdout);
                
                for (int i=0; i<strlen(record_buf.Content); i++){
                    printf("%c",record_buf.Content[i]);
                    fflush(stdout);
                }
                printf("\n");
                fflush(stdout);
            }
            printf("==============================\n");
            fflush(stdout);
        }
        else if (strcmp(record_buf.Content,"!exit")==0) {
            break;
        } 

    }
 
}

static void init_client(char** argv){
    
    cli.ip = argv[1];

    if(atoi(argv[2])==0 || atoi(argv[2])>65536){
        ERR_EXIT("Invalid port");
    }
    cli.port=(unsigned short)atoi(argv[2]);

    struct sockaddr_in servaddr;
    cli.conn_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(cli.conn_fd<0){
        ERR_EXIT("socket");
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(cli.port);

    if(inet_pton(AF_INET, cli.ip, &servaddr.sin_addr)<=0){
        ERR_EXIT("Invalid IP");
    }

    if(connect(cli.conn_fd, (struct sockaddr*)&servaddr, sizeof(servaddr))<0){
        ERR_EXIT("connect");
    }

    return;
}
