#include "hw1.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <sys/time.h>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)
#define BUFFER_SIZE RECORD_NUM*(FROM_LEN+CONTENT_LEN)

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[BUFFER_SIZE];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    int id;
} request;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list
int where_write[2000]={0};
int client_socket[MAX_CLIENTS+1]={0};
fd_set readfd,tmpsd;

// initailize a server, exit for error
static void init_server(unsigned short port);

// initailize a request instance
static void init_request(request* reqP);

// free resources used by a request instance
static void free_request(request* reqP);

int main(int argc, char** argv) {

    // Parse args.
    if (argc != 2) {
        ERR_EXIT("usage: [port]");
        exit(1);
    }

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;
    int conn_fd;  // fd for a new connection with client
    int file_fd;  // fd for file that we open for reading
    char bufone[BUFFER_SIZE];
    int buf_len;
    int last=RECORD_NUM-1; int now_last;
    // Initialize server
    init_server((unsigned short) atoi(argv[1]));
    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    file_fd = open(RECORD_PATH,O_RDWR | O_CREAT );
    ftruncate(file_fd,sizeof(record)*RECORD_NUM);

    int ifLock[RECORD_NUM+1]; // write lock
    memset(ifLock,-1,sizeof(int)*(RECORD_NUM+1));

    int fd_A[MAX_CLIENTS+1]; int sd;
    memset(fd_A,-1,sizeof(int)*(MAX_CLIENTS+1));

    client_socket[0] = svr.listen_fd;
    maxfd = svr.listen_fd;
    // printf("%d\n",svr.listen_fd);

    while (1) {
        FD_ZERO(&readfd);
        FD_SET(svr.listen_fd,&readfd);
        maxfd = svr.listen_fd;

        for (int i=1; i<=MAX_CLIENTS; i++){
            sd=client_socket[i];

            if (sd>0) FD_SET(sd,&readfd);
            if (sd>maxfd) maxfd = sd;
        }

        //TODO: Add IO multiplexing
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;


        int ret = select(maxfd+1,&readfd,NULL,NULL,&timeout);
        if (ret < 0){
            perror("select");
            continue;
        }
            
        if (FD_ISSET(svr.listen_fd,&readfd)){
            clilen = sizeof(cliaddr);
            conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
            if (conn_fd < 0) {
                if (errno == EINTR || errno == EAGAIN) continue;  // try again
                if (errno == ENFILE) {
                    (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                    continue;
                }
                ERR_EXIT("accept");
            }
            requestP[conn_fd].conn_fd = conn_fd;
            strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
            fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);

            for (int i=1; i<=MAX_CLIENTS; i++){
                if (client_socket[i]==0){
                    client_socket[i]=conn_fd;
                    break;
                }
            }       
        }

        for (int i=1; i<=MAX_CLIENTS; i++){
            sd=client_socket[i];

            if (FD_ISSET(sd,&readfd)){

                // TODO: handle requests from clients

                record read_bullet;

                memset(read_bullet.Content,'\0',sizeof(CONTENT_LEN));
                memset(read_bullet.From,'\0',sizeof(FROM_LEN));

                int len=0;
                while(1){
                    len += read(sd,&read_bullet,sizeof(record));;
                    if (len==sizeof(record)){
                        break;
                    }
                }

                struct flock lock;
                lock.l_type = F_UNLCK;   
                lock.l_whence = SEEK_SET;
                lock.l_start = 0;      
                lock.l_len = sizeof(record);

                if (strcmp(read_bullet.Content,"!exit")==0){
                    FD_CLR(sd,&readfd);
                    client_socket[i]=0; 
                    close(sd);
                    free_request(&requestP[sd]);                 
                }
                else if (strcmp(read_bullet.Content,"!pull")==0){
                    lock.l_start = 0; lock.l_type = F_RDLCK;
                    int cnt_lock=0; 
                    for (int j=0; j<RECORD_NUM; j++){

                        lock.l_type = F_RDLCK; lock.l_start=sizeof(record)*j;
                        if (ifLock[j]!=1 && fcntl(file_fd, F_SETLK, &lock) !=-1){ 
                            memset(read_bullet.Content,'\0',sizeof(CONTENT_LEN));
                            memset(read_bullet.From,'\0',sizeof(FROM_LEN));
                            pread(file_fd,&read_bullet,sizeof(record),j*sizeof(record));

                            if (strlen(read_bullet.From)==0 && strlen(read_bullet.From)==0){
                                read_bullet.From[0]='!'; read_bullet.From[1]='\0';
                                read_bullet.Content[0]='!'; read_bullet.Content[1]='\0';
                                
                                len=0;
                                while(1){
                                    len += write(sd,&read_bullet,sizeof(record));
                                    if (len==sizeof(record)){
                                        break;
                                    }
                                }                               
                            }
                            else{
                                len=0;
                                while(1){
                                    len += write(sd,&read_bullet,sizeof(record));
                                    if (len==sizeof(record)){
                                        break;
                                    }
                                } 
                            }
                            lock.l_type=F_UNLCK;
                            fcntl(file_fd, F_SETLK, &lock);
                        }
                        else{

                            read_bullet.From[0]='!'; read_bullet.From[1]='\0';
                            read_bullet.Content[0]='!'; read_bullet.Content[1]='\0';
                            write(sd,&read_bullet,sizeof(record)); 
                            cnt_lock++;

                        }
                    }
                    if (cnt_lock>0){
                        printf("[Warning] Try to access locked post - %d\n",cnt_lock);
                        fflush(stdout);
                    }
                    lock.l_start = 0;
                }
                else if (strcmp(read_bullet.Content,"!post")==0){
                    record judge={"",""};
                    int flag=0; lock.l_type = F_WRLCK;
                    for (int j=(last+1)%RECORD_NUM; ;j=(j+1)%RECORD_NUM){
                        lock.l_start = sizeof(record)*((j)%RECORD_NUM);
                        lock.l_type = F_WRLCK;
                        if (ifLock[j]!=1 && fcntl(file_fd, F_SETLK, &lock)!=-1){
                            ifLock[j]=1;   
                            flag=1; 
                            strcpy(judge.Content,"yes");

                            len=0;
                            while(1){
                                len += write(sd,&judge,sizeof(record));
                                if (len==sizeof(record)){
                                    break;
                                }
                            } 
                            
                            where_write[sd] = j;
                            last=j;       
                            break;
                        }
                        if (j==last){
                            lock.l_start = 0;
                            break;
                        } 
                        lock.l_start = 0;
                    }
                    if (flag==0){
                        strcpy(judge.Content,"no");
                        len=0;
                        while(1){
                            len += write(sd,&judge,sizeof(record));
                            if (len==sizeof(record)){
                                break;
                            }
                        }
                        
                    }
                } 
                else{
                    pwrite(file_fd,&read_bullet,sizeof(record),((where_write[sd])%RECORD_NUM)*sizeof(record));
 
                    lock.l_start = where_write[sd]*sizeof(record);
                    lock.l_type = F_UNLCK;
                    fcntl(file_fd, F_SETLK, &lock);
                    printf("[Log] Receive post from %s\n",read_bullet.From);
                    fflush(stdout);
                    ifLock[where_write[sd]]=-1;                
                }
            }
        }

    }
    free_request(&requestP[conn_fd]); 
    free(requestP);
    return 0;
}

// ======================================================================================================
// You don't need to know how the following codes are working

static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->id = 0;
}

static void free_request(request* reqP) {
    init_request(reqP);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initialize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);
    
    return;
}
