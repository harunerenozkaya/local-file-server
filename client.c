#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "file_operations.h"

#define FIFO_SERVER_PATH "/tmp/server_fifo"
#define FIFO_CLIENT_PATH "/tmp/client_fifo"
#define SEM_SERVERCLIENT_PATH "/tmp/sem_server_client"
#define SHM_SERVERCLIENT_PATH "/tmp/shm_server_client"

#define SHM_SERVERCLIENT_SIZE 1024*1024*100

sem_t* sem;
char* shm_data;

void close_client_fifo(int pid){
    /*Convert pid to char array */
    char pid_s[32] = "";
    snprintf(pid_s,32,"%d",pid);

    // Set fifo client path
    char fifo_client_path[64] = ""; 
    strcat(fifo_client_path,FIFO_CLIENT_PATH);
    strcat(fifo_client_path,pid_s);

    unlink(fifo_client_path);
}

void sig_handler(int signo){
    if (signo == SIGINT){
        close_client_fifo(getpid());
        shm_data[0] = '\0';     
        sprintf(shm_data,"5.quit");
        sem_post(sem);
        printf("Quitting..\n");
        exit(1);
    }
    else if (signo == SIGTERM){

    }
    else if (signo == SIGQUIT){
        close_client_fifo(getpid());
        shm_data[0] = '\0';     
        sprintf(shm_data,"5.quit");
        sem_post(sem);
        printf("Quitting..\n");
        exit(1);
    }
    else if (signo == SIGTSTP){
        close_client_fifo(getpid());
        shm_data[0] = '\0';     
        sprintf(shm_data,"5.quit");
        sem_post(sem);
        printf("Quitting..\n");
        exit(1);
    }
}

int tokenizeRequest(char* request, char*** tokens) {
    int tokenCount = 0;
    char* token;
    char** tokenArray = NULL;
    
    token = strtok(request, " ");
    while (token != NULL) {
        tokenArray = (char**)realloc(tokenArray, sizeof(char*) * (tokenCount + 1));
        tokenArray[tokenCount] = (char*)malloc(strlen(token) + 1);
        strcpy(tokenArray[tokenCount], token);
        tokenCount++;
        token = strtok(NULL, " ");
    }

    // Check if the last token has a newline character at the end
    int lastTokenLength = strlen(tokenArray[tokenCount - 1]);
    if (lastTokenLength > 0 && tokenArray[tokenCount - 1][lastTokenLength - 1] == '\n') {
        tokenArray[tokenCount - 1][lastTokenLength - 1] = '\0'; // Remove the newline character
    }
    
    *tokens = tokenArray;
    return tokenCount;
}

void handleUploadRequest(const char* filename, char* shm_data, sem_t* sem) {
    // If there is no file name, then return with an error message
    if (filename == NULL || strlen(filename) == 0) {
        printf("Error: Please provide a file name\n");
        return;
    }

    // If the file cannot be opened, then return with an error message
    if (openFile(filename) == -1) {
        return;
    }

    // Get the file path
    char path[100] = "";
    if (realpath(filename, path) == NULL) {
        perror("Error retrieving program path");
        return;
    }

    // Write the file path to shm_data
    char* uploadRequestWithPath = malloc(strlen(path) + strlen("upload") + 2);
    sprintf(uploadRequestWithPath, "upload %s", path);
    shm_data[0] = '\0';
    sprintf(shm_data, "%lu.%s", strlen(uploadRequestWithPath) + 1, uploadRequestWithPath);

    sem_post(sem);
}


int main(int argc, char *argv[])
{   
    int fd_server;
    int fd_client;
    char buf_read[256] = "";
    char buf_write[256] = "";
    char connectionStatus[5] = "";
    
    //Determine signal handlers
    if (signal(SIGINT, sig_handler) == SIG_ERR)
        perror("\nERROR : can't catch SIGINT\n");
    if (signal(SIGTERM, sig_handler) == SIG_ERR)
        perror("\nERROR : can't catch SIGTERM\n");
    if (signal(SIGQUIT, sig_handler) == SIG_ERR)
        perror("\nERROR : can't catch SIGQUIT\n");
    if (signal(SIGTSTP, sig_handler) == SIG_ERR)
        perror("\nERROR : can't catch SIGTSTP\n");

    /*Convert pid to char array */
    int pid = getpid();
    char pid_s[32] = "";
    snprintf(pid_s,32,"%d",pid);
    //printf("PID : %s\n",pid_s);

    // Set fifo server path
    char fifo_server_path[64] = ""; 
    strcat(fifo_server_path,FIFO_SERVER_PATH);
    strcat(fifo_server_path,argv[2]);
    //printf("FIFO_SERVER_PATH : %s\n",fifo_server_path);

    // Set fifo client path
    char fifo_client_path[64] = ""; 
    strcat(fifo_client_path,FIFO_CLIENT_PATH);
    strcat(fifo_client_path,pid_s);
    //printf("FIFO_CLIENT_PATH : %s\n",fifo_client_path);

    // Create client FIFO 
    if(mkfifo(fifo_client_path, 0666) == -1){
        printf("%d",errno);
        return -1;
    }

    /* Open the named FIFO for writing */
    fd_server = open(fifo_server_path, O_WRONLY);
    if(fd_server == -1){
        printf("ERROR : Server PID is wrong\n");
        return -1;
    }

    // Open the named FIFO for reading 
    
    fd_client = open(fifo_client_path, O_RDWR);
    if(fd_client == -1){
        printf("ERROR : clientFifo couldn't be opened");
        return -1;
    }

    //Set connect data
    
    char connectData[33];
    strcat(connectData,pid_s);
    if(strcmp("Connect",argv[1]) == 0){
        strcat(connectData,"c");
    }
    else if(strcmp("tryConnect",argv[1]) == 0){
        strcat(connectData,"t");
    }
    else{
        perror("ERROR : Wrong connection type!");
        return -1;
    }
    
    // Write data to the FIFO 
    if(write(fd_server,connectData, strlen(connectData)) == -1){
        printf("ERROR : couldn't be writed to fifo");
        return -1;
    }
    fflush(stdout);
    
    //Get response from server
    ssize_t n = read(fd_client, buf_read, sizeof(buf_read));
    if(n == -1){
        printf("ERROR : Server response couldn't be read!");
        return -1;
    }

    //Get connection status and print response message        
    printf("%s",buf_read);
    strncpy(connectionStatus,buf_read,5);
    
    //clear buffer
    memset(connectData, 0, sizeof(connectData)); 
    memset(buf_read, 0, sizeof(buf_read)); 
    
    /* Close the FIFO */
    close(fd_server);
    close(fd_client);
    unlink(fifo_client_path);

    //If queue is full and it is ERROR than exit
    if(strcmp(connectionStatus,"ERROR") == 0)
        return -1;
    
    // Control if sem and shm is openend from server
    // If it is not opened then wait , probably client is in queue

    //Set semaphore path
    char sem_path[64] = ""; 
    strcat(sem_path,SEM_SERVERCLIENT_PATH);
    strcat(sem_path,pid_s);

    //Set shared memory path
    char shm_path[64] = ""; 
    strcat(shm_path,SHM_SERVERCLIENT_PATH);
    strcat(shm_path,pid_s);

    /* Semaphore to synchorinize handle operations between server and client*/
    sem = sem_open(sem_path,1);
    while (sem == SEM_FAILED) {
        sem = sem_open(sem_path,1);
    }

    /* Shared memory to communicate between server and client */
    int shm_fd = -1;
    while(shm_fd == -1){
        shm_fd = shm_open(shm_path, O_RDWR, 0666);
    }
    printf("\n");

    /* Map shared memory into the address space of the parent and child processes */
    shm_data = mmap(NULL, SHM_SERVERCLIENT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shm_data < 0){
        perror("Error : shared memory mmap");
    }

    /* Requests to server */
    int isFirst = 1;
    int requestLength = 0;

    while(1){
        char buffRequest[1024*4];
        char copyBuffRequest[1024*4];
        char** tokens = NULL;
        int tokenCount = 0;

        memset(buffRequest, 0, sizeof(buffRequest)); 
        memset(copyBuffRequest, 0, sizeof(copyBuffRequest));

        //Other than first request dont give turn server
        if (isFirst == 1){
            sem_wait(sem);
        }  
        
        //Read request from user
        do{
            printf("\nEnter command : ");
            fflush(stdout);
            requestLength = read(0,buffRequest,sizeof(buffRequest));
            if(requestLength == -1){
                perror("ERROR : read() failed\n");
            }
        }while((requestLength-1) <= 0);

        //Copy request because when tokenizing it is broken
        strcpy(copyBuffRequest,buffRequest);

        //Tokenize the request
        tokenCount = tokenizeRequest(buffRequest, &tokens);

        //Handle upload request
        if(strcmp(tokens[0],"upload") == 0){
            //If there is no file name then pass
            if(tokenCount == 1){
                printf("Error : Please provide a file name\n");
                isFirst = 0;
                continue;
            }

            //If file can not be opened then pass
            int fd = openFile(tokens[1]);
            if(fd == -1){
                perror("Error opening file: No such file or directory");
                isFirst = 0;                
                continue;
            }
            closeFile(fd);
            
            //Get file path
            char path[100] = "";
            if (realpath(tokens[1], path) == NULL) {
                perror("Error retrieving program path");
                continue;
            }

            //Write file path to shm_data
            char* uploadRequestWithPath = malloc(sizeof(path) + sizeof(tokens[0] + 1));
            sprintf(uploadRequestWithPath,"%s %s",tokens[0],path);
            shm_data[0] = '\0';
            sprintf(shm_data,"%lu.%s",strlen(uploadRequestWithPath)+1,uploadRequestWithPath);

            free(uploadRequestWithPath);
        }
        else if(strcmp(tokens[0],"download") == 0){
            //If there is no file name then pass
            if(tokenCount == 1){
                printf("Error : Please provide a file name\n");
                isFirst = 0;
                continue;
            }

            //Get file path
            char path[100] = "";
            if (realpath(".", path) == NULL) {
                perror("Error retrieving program path");
                isFirst = 0;
                continue;
            }

            //Write file path to shm_data
            char* downloadRequestWithPath = malloc(sizeof(path) + sizeof(tokens[0] + sizeof(tokens[1]) + 2));
            sprintf(downloadRequestWithPath,"%s %s %s",tokens[0],tokens[1],path);
            shm_data[0] = '\0';
            sprintf(shm_data,"%lu.%s",strlen(downloadRequestWithPath)+1,downloadRequestWithPath);
            
            free(downloadRequestWithPath);
        }
        //Handle other requests
        else{
            //Write request to data
            shm_data[0] = '\0';     
            sprintf(shm_data,"%d.%s",requestLength,copyBuffRequest);
        }
        sem_post(sem);

        //Get response from data
        sem_wait(sem);
        printf("\n%s",shm_data);
        fflush(stdout);

        if(strcmp(shm_data,"Quitting...\n") == 0){
            sem_post(sem);
            break;
        }

        //Make isFirst 0 to dont turn server
        if (isFirst == 1){
            isFirst = 0;
        }  
    }

    /*Close fifo*/
    close_client_fifo(getpid());

    /* Unmap shared memory segment */
    if (munmap(shm_data, SHM_SERVERCLIENT_SIZE) == -1) {
        perror("munmap");
    }

    /* Close shared memory file descriptor */
    if (close(shm_fd) == -1) {
        perror("close");
    }
    
    return 0;
}
