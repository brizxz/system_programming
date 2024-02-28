#define PARENT_READ_FD 3
#define PARENT_WRITE_FD 4
#define MAX_CHILDREN 8
#define MAX_FIFO_NAME_LEN 64
#define MAX_SERVICE_NAME_LEN 16
#define MAX_CMD_LEN 128
#include <sys/types.h>
typedef struct service{
    int pid;
    int read_fd;
    int write_fd;
    char name[MAX_SERVICE_NAME_LEN];
    struct service *next;
} service;

typedef struct msg{
    int type; // 1:spawn 2:kill 3:exchange begin 4:exchange finish
    int find_flag;
    char spawn_parent[MAX_SERVICE_NAME_LEN];
    char spawn_child[MAX_SERVICE_NAME_LEN];

    int kill_flag; // 0 means unfound, 1 means found in the up layer, 2 means find and write to parent
    int if_child; // set one to its parent
    char kill_target[MAX_SERVICE_NAME_LEN];
    int child_num;

    int find_exchange_num;
    char exchange_target_first[MAX_SERVICE_NAME_LEN]; // 
    char exchange_target_second[MAX_SERVICE_NAME_LEN];
    char fifo_btoa[MAX_FIFO_NAME_LEN];
    char fifo_atob[MAX_FIFO_NAME_LEN];
} messenge;

typedef struct {
    /* data */
    unsigned long secret_val;
} fifo_secret;
