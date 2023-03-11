#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include "../include/struct.h"
#include"../include/helpers.h"
int main(int argc, char **argv)
{
    //Initialize Random generator based on pid so each process has a different rand
    srand(time(NULL) - getpid());
    clock_t start, end;
    //Temporarily create a string to open a log file
    char *proc_id = malloc(sizeof(char) * 100);
    if(proc_id==NULL){
        perror("malloc at proc_id");
        exit(20);
    }
    sprintf(proc_id, "./logs/%d_log.txt", getpid());
    
    //Delete previous log file in case proccess id is the same as a previous one
    if(remove(proc_id)==0){
        printf("Removed File succcessfully\n");
    }
    
    FILE *logs;
    //Get necessary integers from arguments
    int id = atoi(argv[0]);         //Id for memory block
    int max_sec = atoi(argv[1]);    //Number of sections
    int max_line = atoi(argv[2]);   //Number of chars in max line
    int loop = atoi(argv[3]);       //Total requests to be sent
    int total_lines = atoi(argv[4]);    //Total lines in each segment
    char *line;
    if (id == 0)
    {
        perror("Error at getting shm id from parent");
        exit(14);
    }
    //Attach memory
    shared_obj *memory = (shared_obj *)shmat(id, NULL, 0);
    if (memory == (void *)-1)
    {
        perror("Attachment.");
        exit(15);
    }
    // Attach shared string
    char *shared_string = (char *)shmat(memory->ID_str, NULL, 0);
    if(shared_string==(char *)-1){
        perror("Error at attaching child to shared_string");
        exit(16);
    }

    //Open created semaphores for usage
    sem_t *write_server = sem_open(WR_SERV, 0);
    sem_t *ready = sem_open(READY, 0);
    sem_t *critical = sem_open(CS, 0);
    sem_t *queue = sem_open(QUEUE, 0);

    //Variables needed
    int new = 1,old_req=-1,req_line,req_sec;
    
    while (loop > 0)
    {
        //Create a new request
        if (new == 1)
        {
            get_request(old_req, &req_line, &req_sec, total_lines, max_line, max_sec);
            start = clock();
            old_req = req_sec;
            printf("Client: %d made request <%d,%d>\n", getpid(), req_line, req_sec);
            fflush(stdout);
            //Make new 0 so that if the loop come backs we dont create a new request and discard current
            new = 0;
        }
        
        //In case requested segment is current segment go ahead and read
        if (req_sec == memory->curr_seg)
        {
            end = clock();      //Stop Clock
            line = get_line(shared_string, req_line);       //Read requested line
            logs = fopen(proc_id, "a");      //Open log file in log folder
            fprintf(logs, "Response Time:%fs,<%d,%d>,Line:%s\n", ((double)(end - start)) / CLOCKS_PER_SEC, req_line, req_sec, line); //Write in logs
            printf("Client: %d got request <%d,%d>\n", getpid(), req_line, req_sec);    //Print to let user know that request got completed
            loop--;             //One request done
            new = 1;            //Create new request
            fclose(logs);
            usleep(20000);      //Sleep for 20ms
            continue;
        }
        else
        {
            //In case current section available isn't requested section wait for critical section
            //to change the variable in memory
            sem_wait(critical);
            printf("Client: %d got queued\n", getpid());
            fflush(stdout);
            //If u are first to enter Queue make your requested segment the next to be put
            if (memory->in_q == 0)
            {
                memory->next_seg = req_sec;
                printf("Server: Next segment will be %d\n", memory->next_seg);
                fflush(stdout);
            }

            //Increase the number of processes waiting in queue
            memory->in_q++;
            if (memory->in_q == (N - memory->dead))
            {
                memory->in_q--;
                //In case the Queue is now full with all alive processes call server to change segment
                sem_post(write_server);
                sem_wait(ready);
                //Give critical to the next process
                sem_post(critical);
            }
            else
            {
                //If you Queue isn't Full give back critical and wait in Queue
                sem_post(critical);
                sem_wait(queue);
            }
        }
    }

    //process exits and waits for critical to update number of dead processes
    sem_wait(critical);
    memory->dead++;
    
    //In Case Queue is now full because all other processes are waiting in Queue or all processes are dead
    //change Segment and unblock
    
    if (memory->in_q == N - memory->dead)
    {
        //Post to server so that segment updates
        sem_post(write_server);
        sem_wait(ready);

    }
    //Give back critical
    sem_post(critical);
    
    //Close open File and exit
    free(proc_id);
    return 0;
}
