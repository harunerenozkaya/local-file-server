#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <semaphore.h>
#include "queue.h"

#define FIFO_SERVER_PATH "/tmp/server_fifo"
#define FIFO_CLIENT_PATH "/tmp/client_fifo"
#define SHM_PATH "/tmp/shm"
#define SEM_PATH_SERVER "/tmp/semServer"
#define SEM_PATH_CLIENT "/tmp/semClient"

#define MAX_CLIENT 5


int currentClientCount = 0; //TODO , currentClientCount isn't shared , make shared
queue_t* queue; //TODO , queue isn't shared , make shared

int main()
{
    int fd_server;
    int fd_client;
    char buf_read[256];
    char buf_write[256];
    ssize_t n;

    queue = createQueue();
    //TODO trying (delete)
    enqueue(queue, "1111");
    enqueue(queue, "2222");
    enqueue(queue, "3333");

    //Convert pid to char array 
    int pid = getpid();
    char pid_s[32] = "";
    snprintf(pid_s,32,"%d",pid);
    //printf("SERVER PID : %s\n",pid_s);

    // Open the named semaphore to synchorinize all child server process operations
    char sem_path[32] = "";
    strcat(sem_path,SEM_PATH_SERVER);
    strcat(sem_path,pid_s);
    sem_t *sem = sem_open(sem_path, O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) {
        printf("Failed to open semaphore\n");
        return -1;
    }

    // Set server fifo path
    char fifo_server_path[64] = ""; 
    strcat(fifo_server_path,FIFO_SERVER_PATH);
    strcat(fifo_server_path,pid_s);
    //printf("FIFO_SERVER_PATH : %s\n",fifo_server_path);

    // Create server FIFO 
    if(mkfifo(fifo_server_path, 0666) == -1){
        printf("ERROR : fifo_swcr couldn't be created");
        return -1;
    }
   
    printf("Server Started PID : %d\n",pid);
    printf("Waiting for clients...\n");

    // Open the server FIFO for reading 
    fd_server = open(fifo_server_path, O_RDONLY);
    if(fd_server == -1){
        printf("ERROR : clientFifo couldn't be opened");
        return -1;
    }
    sem_t* sem1 = sem_open("/tmp/sem1", O_CREAT | O_EXCL, 0666, 0);
    if (sem1 < 0) {
        perror("sem_init");
        exit(1);
    }
    sem_t* sem2 = sem_open("/tmp/sem2", O_CREAT | O_EXCL, 0666, 1);
    if (sem2 < 0) {
        perror("sem_init");
        exit(1);
    }

    int opType = fork();

    /*Get server connection request and create child servers*/
    if (opType == 0){
        
        // Read data from the client via fd_server fifo
        while (1) {
            ssize_t n = read(fd_server, buf_read, sizeof(buf_read));
            
            if(n == -1){
                printf("ERROR : clientFifo couldn't be read!");
            } 
            else if (n > 0) {
                char response[128] = "";

                char connectionType = buf_read[strlen(buf_read)-1];
                buf_read[strlen(buf_read) -1] = '\0';
                char* clientPid = buf_read;

                // Set client fifo path and open client fifo
                char fifo_client_path[64] = ""; 
                strcat(fifo_client_path,FIFO_CLIENT_PATH);
                strcat(fifo_client_path,clientPid);

                //printf("%c",connectionType);
                //printf("Received data: %s\n", clientPid);
                //printf("FIFO_CLIENT_PATH : %s\n",fifo_client_path);

                // Open clientFifo thanks to clientPID in the request
                fd_client = open(fifo_client_path, O_RDWR);
                if(fd_client == -1){
                    printf("ERROR : clientFifo couldn't be opened");
                    return -1;
                }

                //Response to connection request
                sem_wait(sem2);
                //If request is tryConnect and currentClientCount is full than reject
                if(connectionType == 't' && currentClientCount == MAX_CLIENT){
                    strcat(response,"ERROR : Server has been reached the maximum client!\n");
                    if(write(fd_client,response,strlen(response)) == -1){
                        printf("ERROR : Server connection response couldn't be writed");
                        return -1;
                    }        
                    printf("Client(%s) rejected\n",clientPid);
                }
                //If request is Connect and currentClientCount is full than put queue
                else if(connectionType == 'c' && currentClientCount == MAX_CLIENT){
                    strcat(response,"WARNING : Server has been reached the maximum client! You are in queue\n");
                    if(write(fd_client,response,strlen(response)) == -1){
                        printf("ERROR : Server connection response couldn't be writed");
                        return -1;
                    }
                    //Insert client queue
                    enqueue(queue, clientPid);
                    printf("Client(%s) inserted queue\n",clientPid);
                }
                //If currentClientCount is appropriate than approve
                else{
                    strcat(response,"SUCCESS : Connection established\n");
                    if(write(fd_client,response,strlen(response)) == -1){
                        printf("ERROR : Server connection response couldn't be writed");
                        return -1;
                    }
                    
                    printf("Client(%s) connected\n",clientPid);
                    currentClientCount += 1;
                    printf("Current client count : %d\n",currentClientCount);

                }
                sem_post(sem1);
                fflush(stdout);
            }
            
            //clear buffer
            memset(buf_read, 0, sizeof(buf_read));  
        }
    }
    /*Control the queue and if it is okay then response the clients*/
    else{
        while (1)
        {
            sem_wait(sem1);
            if(isEmpty(queue) != 1 && currentClientCount < MAX_CLIENT){
                char* clientPid = dequeue(queue);
                printf("Client (%s) connected from queue\n",clientPid);
                currentClientCount += 1;
                printf("Current client count : %d\n",currentClientCount);
                free(clientPid);
            }else{
                sem_post(sem2);
            }
        }
        
    }
    
    close(fd_client);
    close(fd_server);
    unlink(fifo_server_path);
    return 0;
}



