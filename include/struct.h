#define WR_SERV "/server123"
#define READY "/ready123"
#define CS "/critical123"
#define QUEUE "/fqueue123"
#define N 10
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP )
typedef struct shared_obj{
    int ID_str;
    int in_q;
    int curr_seg;
    int next_seg;
    int dead;
}shared_obj;
