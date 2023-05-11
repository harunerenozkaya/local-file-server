#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <semaphore.h>
#include "queue.h"
#include <sys/mman.h>
#include <signal.h>

#define FIFO_SERVER_PATH "/tmp/server_fifo"
#define FIFO_CLIENT_PATH "/tmp/client_fifo"
#define SHM_MAINSERVER_PATH "/tmp/shm_server_main"
#define SEM_MAINSERVER_PATH "/tmp/sem_server_main"
#define SEM_SERVERCLIENT_PATH "/tmp/sem_server_client"
#define SHM_SERVERCLIENT_PATH "/tmp/shm_server_client"

#define MAX_CLIENT 5
#define SHM_SERVERCLIENT_SIZE 1024*1024*100

// Shared memory struct
typedef struct {
    int currentClientCount;
    queue_t* queue;
} shared_serverInfo_t;

void close_server_fifo(int pid){
    //Convert pid to char array 
    char pid_s[32] = "";
    snprintf(pid_s,32,"%d",pid);
    //printf("SERVER PID : %s\n",pid_s);

    // Set server fifo path
    char fifo_server_path[64] = ""; 
    strcat(fifo_server_path,FIFO_SERVER_PATH);
    strcat(fifo_server_path,pid_s);

    unlink(fifo_server_path);
}

void sig_handler(int signo){
    if (signo == SIGINT){
        shm_unlink(SHM_MAINSERVER_PATH);
        sem_unlink(SEM_MAINSERVER_PATH);
        close_server_fifo(getpid());
        exit(1);
    }
    else if (signo == SIGTERM){
        shm_unlink(SHM_MAINSERVER_PATH);
        sem_unlink(SEM_MAINSERVER_PATH);
        close_server_fifo(getpid());
        exit(1);
    }
    else if (signo == SIGQUIT){
        shm_unlink(SHM_MAINSERVER_PATH);
        sem_unlink(SEM_MAINSERVER_PATH);
        close_server_fifo(getpid());
        exit(1);
    }
    else if (signo == SIGTSTP){
        shm_unlink(SHM_MAINSERVER_PATH);
        sem_unlink(SEM_MAINSERVER_PATH);
        close_server_fifo(getpid());
        exit(1);
    }
}

void create_child_server(char* pid){
   int child = fork();
    if(child == 0){
        char* shm_data;

        //Set semaphore path
        char sem_path[64] = ""; 
        strcat(sem_path,SEM_SERVERCLIENT_PATH);
        strcat(sem_path,pid);

        char shm_path[64] = ""; 
        strcat(shm_path,SHM_SERVERCLIENT_PATH);
        strcat(shm_path,pid);

        //Set shared memory path

        /* Semaphore to synchorinize handle operations between server and client*/
        sem_t* sem = sem_open(sem_path, O_CREAT | S_IRUSR | S_IWUSR, 0666, 1);
        if (sem < 0) {
            perror("ERROR : Shared memory for communication between server and client");
            exit(1);
        }

        /*Shared memory to communicate between server and client*/
        int shm_fd = shm_open(shm_path, O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1) {
            perror("ERROR : Shared memory for communication between server and client");
            exit(1);
        }

        // Set shared memory size for client count and queue
        if (ftruncate(shm_fd, SHM_SERVERCLIENT_SIZE) == -1) {
            perror("ERROR : Shared memory size couldnt be set");
            exit(1);
        }

        // Map shared memory into the address space of the parent and child processes
        shm_data = mmap(NULL, SHM_SERVERCLIENT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shm_data == MAP_FAILED) {
            perror("ERROR : Shared memory couldn't be mapped");
            exit(1);
        }
        
        //Response client requests
        while(1){
            printf("bekliyor %s\n",pid);
            sem_wait(sem);
            sleep(1);
            printf("işlem yapıyor\n");
            sem_post(sem);
        }
        

        /* Unmap shared memory segment */
        if (munmap(shm_data, SHM_SERVERCLIENT_SIZE) == -1) {
            perror("munmap");
            exit(1);
        }
        /* Close shared memory file descriptor */
        if (close(shm_fd) == -1) {
            perror("close");
            exit(1);
        }

        /* Remove shared memory segment */
        if (shm_unlink(shm_path) == -1) {
            perror("shm_unlink");
            exit(1);
        }

        /* Remove semaphore segment */
        if (sem_unlink(sem_path) == -1) {
            perror("sem_unlink");
            exit(1);
        }

        exit(1);
    }
}

int main()
{   
    int fd_server;
    int fd_client;
    char buf_read[256];
    char buf_write[256];
    ssize_t n;

    //Determine signal handlers
    if (signal(SIGINT, sig_handler) == SIG_ERR)
        perror("\nERROR : can't catch SIGINT\n");
    if (signal(SIGTERM, sig_handler) == SIG_ERR)
        perror("\nERROR : can't catch SIGTERM\n");
    if (signal(SIGQUIT, sig_handler) == SIG_ERR)
        perror("\nERROR : can't catch SIGQUIT\n");
    if (signal(SIGTSTP, sig_handler) == SIG_ERR)
        perror("\nERROR : can't catch SIGTSTP\n");
    
    shm_unlink(SHM_MAINSERVER_PATH);
    // Create shared memory for client count and queue
    int shm_serverInfo_fd = shm_open(SHM_MAINSERVER_PATH, O_CREAT | O_RDWR, 0666);
    if (shm_serverInfo_fd == -1) {
        perror("ERROR : Shared memory for client count and queue couldn't be opened");
        exit(1);
    }

    // Set shared memory size for client count and queue
    if (ftruncate(shm_serverInfo_fd, sizeof(shared_serverInfo_t)) == -1) {
        perror("ERROR : Shared memory size couldnt be set");
        exit(1);
    }

    // Map shared memory into the address space of the parent and child processes
    shared_serverInfo_t* serverInfo = mmap(NULL, sizeof(shared_serverInfo_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_serverInfo_fd, 0);
    if (serverInfo == MAP_FAILED) {
        perror("ERROR : Shared memory couldn't be mapped");
        exit(1);
    }

    // Initialize shared memory of current client count and queue
    serverInfo->currentClientCount = 0;
    serverInfo->queue = createQueue();

    enqueue(serverInfo->queue,"1111");
    enqueue(serverInfo->queue,"2222");
    enqueue(serverInfo->queue,"3333");

    //Convert pid to char array 
    int pid = getpid();
    char pid_s[32] = "";
    snprintf(pid_s,32,"%d",pid);
    //printf("SERVER PID : %s\n",pid_s);

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

    /* Semaphores to synchorinize handle connection request proces and queue control process*/
    sem_t* semMain = sem_open("/tmp/semMain", O_CREAT | O_EXCL, 0666, 1);
    if (semMain < 0) {
        perror("sem_init");
        exit(1);
    }

    int opType = fork();

    /*Get server connection request and create child servers*/
    if (opType > 0){
        
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
                sem_wait(semMain);
                //If request is tryConnect and currentClientCount is full than reject
                if(connectionType == 't' && serverInfo->currentClientCount == MAX_CLIENT){
                    strcat(response,"ERROR : Server has been reached the maximum client!\n");
                    if(write(fd_client,response,strlen(response)) == -1){
                        printf("ERROR : Server connection response couldn't be writed");
                        return -1;
                    }        
                    printf("Client(%s) rejected\n",clientPid);
                }
                //If request is Connect and currentClientCount is full than put queue
                else if(connectionType == 'c' && serverInfo->currentClientCount == MAX_CLIENT){
                    strcat(response,"WARNING : Server has been reached the maximum client! You are in queue\n");
                    if(write(fd_client,response,strlen(response)) == -1){
                        printf("ERROR : Server connection response couldn't be writed");
                        return -1;
                    }
                    //Insert client queue
                    enqueue(serverInfo->queue, clientPid);
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
                    serverInfo->currentClientCount += 1;
                    //printf("Current client count : %d\n",serverInfo->currentClientCount);

                    create_child_server(clientPid);
                }
                sem_post(semMain);
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
            sem_wait(semMain);
            if(isEmpty(serverInfo->queue) != 1 && serverInfo->currentClientCount < MAX_CLIENT){
                char* clientPid = dequeue(serverInfo->queue);
                int clientPidInt = atoi(clientPid);

                //Control if the client is still waiting or exited
                if (kill(clientPidInt, 15) == -1) {
                    printf("Client (%s) was terminated while waiting. Extracting from queue\n",clientPid);
                }
                else{
                    printf("Client (%s) connected from queue\n",clientPid);
                    serverInfo->currentClientCount += 1;
                    //printf("Current client count : %d\n",serverInfo->currentClientCount);
                    create_child_server(clientPid);
                }

                free(clientPid);
            }
            else{
                sem_post(semMain);
            }
        }
        
    }
    
    close(shm_serverInfo_fd);
    close(fd_client);
    close(fd_server);
    unlink(fifo_server_path);
    return 0;
}



