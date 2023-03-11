# Inter Process Communication

This repository contains an OS assigment concerning IPC and its solution. To better understand what the program does I suggest you read the assignment first, however it is written in Greek, so for those who don't know Greek there is a summary in English below.

# Assignment

Create a program that has one parent process and N child processes that read/write from a buffer. The parent process has access to a .txt file from which it writes a specific segment to the buffer and only it can write in the buffer, also the child processes may only read from the buffer. Multiple children can read at a time but the parent can't write when there is at least one child still reading. The way children read is by sending requests about a specific segment from  the  .txt file and a specific line from the segment,if the segment they are requesting is the one currently at the buffer they may read the line, if not they wait in Queue for the segment to change. The strategy used to change segments is a simple FIFO strategy. The server(parent process) will change the segment only when there is no other request for the current segment available and there are child processes waiting for some other segment.The .txt file to read from as well as the number of lines each segment of the .txt file will contain and the number of requests each child will make are passed as arguments when running the executable(e.g ./server test.txt 2 2 will read from test.txt with each segment containing 2 lines and each child sending 2 requests). The way child processes create requests is as follows with 30% probability choose a new random segment,with 70% probability choose the segment of your previous request, the line requested from the segment is completely random. After each request each child process will write in its log file the time it took to get the requested line and the server logs when each segment entered the buffer and when each segment exited the buffer.

# Compiling and running

To compile the files run ` make ` and to run the executable ` server ` type ` ./server file.txt X Y `  where file.txt is the file from which lines will be requested, X is an integer that tells us how many lines each segment will have and Y is an integer that corresponds to the amount of requests each child process will make.As an example run ` make run ` to see the program running with the file ` test.txt ` and 2 lines per segment and 10 requests per child. In order the clean the log files  and .o files run `make clean`.

# Server
The server in this implementation is only responsible for changing the segment currently inside the buffer. Before the main loop begins the server calls the helper functions to get the number of lines per segment,the maximum line and the number of requests per child. After that it initiates the semaphores as well as the shared memory(posix library used). For shared memory we use a struct shared_obj which holds information about the segment currently in use,the next segment to be put as well as the number of children waiting in queue, in addition we have a shared memory string that will be used as our buffer. To synchronize the processes we use 4 semaphores:
- `write_server`: signals the server so that it changes the segment to the next requested segment or quits if all children are done requesting.
- `ready`: signals to the child processes that the server is done changing the segment and they may continue reading.
- `critical`: mutex semaphore used so only one process writes at the shared_obj struct.
- `queue`: semaphore used to block child processes that are waiting in queue.

After the initialization of the semaphores and the shared memory the server starts creating child processes and waits for the signal to change the current segment. After all childrens' requests have been granted the server is signaled to exit.


# Client
Each client runs the file `client.c` where lines per segment,max line and total requests are passed as arguments as well as the id of the shared_obj, After linking to the shared memory and to the named semaphores the client starts sending requests in a loop until they have completed sending the requests.
- If the flag `new` is set to 1 then the client makes a new request
- Then they check whether the current segment is the same as the one they are requesting
- If it is they read the line and `continue` to create another request
- If it isn't they go to wait in the queue. In case they are the first to enter the queue they change the `next_seg` to their requested segment and wait in the queue. In case by them going to the queue now all alive processes(meaning all client processes still running) are waiting in the queue they signal the server to change the segment and they release all the other processes from the blocked semaphore. At the end they release the `critical` mutex semaphore so other clients can enter the queue.

After exiting the loop and completing the requests they check the following:
- If by them exiting now all other processes are waiting in queue they signal the server and release the other children.
- In the other case that they are the last child process to exit they signal the server to also exit from the loop of changing segments.

