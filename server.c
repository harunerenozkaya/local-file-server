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

int main()
{
    int fd_server;
    int fd_client;
    char buf_read[256];
    char buf_write[256];
    ssize_t n;

    int currentClientCount = 5;
    queue_t* queue = createQueue();
    
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
            //If currentClientCode is appropriate than approve
            else{
                strcat(response,"SUCCESS : Connection established\n");
                if(write(fd_client,response,strlen(response)) == -1){
                    printf("ERROR : Server connection response couldn't be writed");
                    return -1;
                }
                
                printf("Client PID %s connected\n",clientPid);
                currentClientCount += 1;
            }
            fflush(stdout);
        }
        //clear buffer
        memset(buf_read, 0, sizeof(buf_read));  
    }

    close(fd_client);
    close(fd_server);
    unlink(fifo_server_path);
    return 0;
}



