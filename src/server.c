#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include "../include/struct.h"
#include <errno.h>
#include "../include/helpers.h"
int main(int argc, char **argv)
{
    srand(time(NULL));
    if (argc != 4)
    {
        perror("Wrong arguments,format: ./server filename X Y\nX:Lines in each section  Y:Requests per Child\n");
        return 1;
    }
    // Get information needed from file provided from user
    int total_lines = get_lines(argv[1]);
    int max_line = get_max_line(argv[1]); // Find longest line to have an upper limit to block size
    int lines_per_section = atoi(argv[2]);
    int req_per_child = atoi(argv[3]);
    int sections = (total_lines / lines_per_section) + (total_lines % lines_per_section == 0 ? 0 : 1);
    int block_size = lines_per_section * max_line + 1; // Calculate block size to allocate

    printf("Total Lines: %d\nMax Line: %d\nRequests per child: %d\nLines per section: %d\n", total_lines, max_line, req_per_child, lines_per_section);

    // Open files to read and logs file to write
    FILE *source = fopen(argv[1], "r+");
    if(!source){
        perror("File to read not found");
        exit(44);
    }
    FILE *log = fopen("./logs/server_log.txt", "w+");

    char *segment;
    if (lines_per_section == 0 || lines_per_section > total_lines)
    {
        perror("Error at number of lines per section");
        exit(40);
    }
    if (!source)
    {
        perror("Error at reading file");
        exit(30);
    }
    if (!log)
    {
        perror("Error at writing logs");
        exit(31);
    }

    // Create shared memory object that stores usefull information(But not the string)
    int key = ftok("./server.c", 'R');
    if (key == 1)
    {
        perror("ftok");
        exit(1);
    }

    int id = shmget(key, sizeof(shared_obj), 0664 | IPC_CREAT);
    if (id == -1)
    {
        perror("shmget");
        exit(2);
    }
    shared_obj *memory = (shared_obj *)shmat(id, NULL, 0);
    if (memory == (shared_obj *)-1)
    {
        perror("shmat");
        exit(3);
    }

    // Create another shared block that is specifically for the string and store its ID in shared obj

    memory->ID_str = shmget(IPC_PRIVATE, block_size * sizeof(char), 0664 | IPC_CREAT);
    if (memory->ID_str == -1)
    {
        perror("string shmget");
        exit(4);
    }

    char *shared_string = (char *)shmat(memory->ID_str, NULL, 0);

    if (shared_string == (char *)-1)
    {
        perror("string shmat");
        exit(5);
    }
    // Create an array for string to pass as arguments
    char **args = malloc(sizeof(char *) * 6);
    if (args == NULL)
    {
        perror("malloc args");
        exit(9);
    }
    for (int i = 0; i < 6; i++)
    {
        args[i] = malloc(sizeof(char) * 50);
        if (args[i] == NULL)
        {
            perror("malloc at *args");
            exit(10);
        }
    }
    // Write arguments that need to be passed
    sprintf(args[0], "%d", id);
    sprintf(args[1], "%d", sections);
    sprintf(args[2], "%d", lines_per_section);
    sprintf(args[3], "%d", req_per_child);
    sprintf(args[4], "%d", total_lines);
    args[5] = NULL;

    // Initialization of shared object

    memory->curr_seg = -1;
    memory->next_seg = -1;
    memory->in_q = 0;
    memory->dead = 0;

    // Semaphores

    // First unlink all semaphores in Case previous run of program was stopped without sem_close
    sem_unlink(WR_SERV);
    sem_unlink(READY);
    sem_unlink(CS);
    sem_unlink(QUEUE);

    // Initialize them
    sem_t *write_server = sem_open(WR_SERV, O_CREAT, SEM_PERMS, 0);
    sem_t *ready = sem_open(READY, O_CREAT, SEM_PERMS, 0);
    sem_t *critical = sem_open(CS, O_CREAT, SEM_PERMS, 0);
    sem_t *queue = sem_open(QUEUE, O_CREAT, SEM_PERMS, 0);

    // Post critical so only one process can acces it
    sem_post(critical);

    // Start forking and exec to create child processes

    pid_t id_p;
    for (int p_number = 0; p_number < N; p_number++)
    {
        if ((id_p = fork()) == 0)
        {
            int err = execv("client", args);
            if (err == -1)
            {
                printf("failed exec\n");
                exit(6);
            }
        }
        else if (id_p < 0)
        {
            perror("Error at forking\n");
            exit(7);
        }
    }
    int temp;
    // Start loop and wait for client to post in order to change segment
    while (1)
    {
        if (sem_wait(write_server) < 0)
        {
            perror("Didn't wait in write_server\n");
            exit(9);
        }
        // In case the write_server was posted from the last process to exit
        if (memory->dead == N)
        {
            fprintf(log, "Segment %d Time of Exit: %f\n", memory->curr_seg, (double)clock() / CLOCKS_PER_SEC);
            sem_post(ready);
            break;
        }
        // In case a new segment shouldn't be put in memory
        if (memory->curr_seg == memory->next_seg)
        {
            sem_post(ready);
            continue;
        }

        // Change segment and update time for segment

        fflush(stdout);
        segment = get_sector(argv[1], memory->next_seg, lines_per_section, max_line);
        strncpy(shared_string, segment,block_size);
        printf("Server: Changing from %d to %d\n", memory->curr_seg, memory->next_seg);
        // Write in log file
        if (memory->curr_seg != -1)
        {
            fprintf(log, "Segment %d Time of Exit: %f\n", memory->curr_seg, (double)clock() / CLOCKS_PER_SEC);
        }
        fprintf(log, "Segment %d Time of Entry: %f\n", memory->next_seg, (double)clock() / CLOCKS_PER_SEC);

        if (segment != NULL)
        {
            free(segment);
        }
        else
        {
            perror("Couldn't get segment\n");
            exit(8);
        }
        memory->curr_seg = memory->next_seg;
        // Post that new segment is ready for clients
        temp = memory->in_q;
        memory->in_q = 0;
        for (int i = 0; i < temp; i++)
        {
            sem_post(queue);
        }
        if (sem_post(ready) < 0)
        {
            perror("Didn't post ready semaphore\n");
            exit(10);
        }
    }

    // Wait for child to exit
    int status;
    while ((id_p = wait(&status)) > 0)
    {
        printf("Process %d exited with status %d\n", id_p, status);
    }

    // Dettach from shared memory blocks

    if (shmdt(shared_string) == -1)
    {
        perror("Error at shared string detachment");
        exit(21);
    }

    if (shmdt(memory) == -1)
    {
        perror("Error at memory detachment");
        exit(22);
    }

    // Close Semaphores
    sem_close(write_server);
    sem_close(ready);
    sem_close(critical);
    sem_close(queue);
    return 0;
}