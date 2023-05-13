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

/**
 * The function takes a string and returns the length of the substring before the first occurrence of
 * the dot character, converted to an integer and decremented by 1.
 * 
 * @param str A pointer to a string that represents a request. The request is expected to have a number
 * followed by a dot character as its first substring. The function extracts this number and returns it
 * as an integer.
 * 
 * @return an integer value which represents the length of the substring before the first occurrence of
 * the dot character in the input string, minus one.
 */
int getRequestLength(char *str) {
    char *dot_ptr = strchr(str, '.'); // Find the first occurrence of the dot character
    if (dot_ptr == NULL) { // If the dot character is not found, return 0
        return 0;
    }
    int len = dot_ptr - str; // Calculate the length of the substring before the dot
    char *int_str = malloc(len + 1); // Allocate memory for the integer string
    strncpy(int_str, str, len); // Copy the substring before the dot into the integer string
    int_str[len] = '\0'; // Null-terminate the integer string
    int result = atoi(int_str); // Convert the integer string to an integer
    free(int_str); // Free the memory allocated for the integer string
    return result-1;
}

/**
 * The function deletes the substring after the first occurrence of the dot character in a given
 * string.
 * 
 * @param str a pointer to a character array (string) that contains a dot character '.' and possibly
 * other characters after it.
 * 
 * @return If the dot character is not found in the input string, the function returns without doing
 * anything. If the dot character is found, the function removes the substring after the dot and moves
 * the remaining characters to the beginning of the string. The function does not return anything.
 */
void deleteLengthPart(char *str) {
    char *dot_ptr = strchr(str, '.'); // Find the first occurrence of the dot character
    if (dot_ptr == NULL) { // If the dot character is not found, do nothing
        return;
    }
    int len = strlen(dot_ptr); // Calculate the length of the substring after the dot
    memmove(str, dot_ptr + 1, len); // Move the substring after the dot to the beginning of the string
}

/**
 * The function takes a string and tokenizes it into an array of strings based on spaces.
 * 
 * @param request The request string that needs to be tokenized.
 * @param tokens The "tokens" parameter is a pointer to a pointer to a char array. This function will
 * allocate memory for an array of char pointers and assign it to the memory location pointed to by
 * "tokens". The array will contain the individual tokens parsed from the "request" string.
 * 
 * @return The function `tokenizeRequest` is returning an integer value which represents the number of
 * tokens in the request string.
 */
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
    
    *tokens = tokenArray;
    return tokenCount;
}

/**
 * The function `printTokens` prints an array of strings and its length.
 * 
 * @param tokens tokens is a pointer to an array of strings (char**), where each string represents a
 * token.
 * @param tokenCount The parameter `tokenCount` is an integer that represents the number of tokens in
 * the `tokens` array.
 */
void printTokens(char** tokens, int tokenCount) {
    printf("tokens = [");
    for (int i = 0; i < tokenCount; i++) {
        printf("\"%s\"", tokens[i]);
        if (i < tokenCount - 1) {
            printf(",");
        }
    }
    printf("]\n");
    
    printf("tokenCount = %d\n", tokenCount);
}

/**
 * The function frees memory allocated for an array of tokens.
 * 
 * @param tokens tokens is a pointer to a pointer of characters (i.e. a pointer to a string array).
 * @param tokenCount The parameter `tokenCount` is an integer that represents the number of tokens in
 * the `tokens` array.
 */
void freeTokens(char** tokens, int tokenCount) {
    for (int i = 0; i < tokenCount; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

/**
 * This function reads a request from shared memory, removes the length part, tokenizes the request,
 * and returns the number of tokens.
 * 
 * @param shm_data A pointer to a shared memory segment that contains the request data.
 * @param tokens a pointer to a char** variable that will hold the tokens of the parsed request
 * @param tokenCount a pointer to an integer variable that will store the number of tokens in the
 * request after it has been tokenized.
 */
void readRequest(char* shm_data, char*** tokens, int* tokenCount) {
    int requestLength;

    //Get request length by parsing wrote request length at start
    requestLength = getRequestLength(shm_data);

    //Delete length part from request
    deleteLengthPart(shm_data);

    //Parse part of just in length
    char* requestWithoutNoise = strndup(shm_data, requestLength);

    //Tokenize the request
    *tokenCount = tokenizeRequest(requestWithoutNoise, tokens);

    // Free memory
    free(requestWithoutNoise);
}

/**
 * The function handles help requests by providing information on available commands and their usage.
 * 
 * @param tokens An array of strings containing the command and its arguments, split by spaces.
 * @param tokenCount The number of tokens (or arguments) passed to the function.
 * @param shm_data A pointer to a shared memory segment where the response message will be written.
 */
void handleHelp(char** tokens, int tokenCount, char* shm_data) {
    /* Write help response */
    if (tokenCount == 1) {
        sprintf(shm_data, "%s", "Available comments are : \nhelp, list, readF, writeT, upload, download, quit, killServer\n");
    }
    /* Write help response for specific command*/
    else {
        //help help
        if (strcmp(tokens[1], "help") == 0) {
            sprintf(shm_data, "%s", "Usage : help or help <command>\n\thelp command give information about how <command> run and what <command> do.\n");
        }
        //help list
        else if (strcmp(tokens[1], "list") == 0) {
            sprintf(shm_data, "%s", "Usage : list \n\tlist command lists the files in the server directory\n");
        }
        //help readF
        else if (strcmp(tokens[1], "readF") == 0) {
            sprintf(shm_data, "%s", "Usage : readF <file> <line #> \n\treadF command displays the #th line of the <file>, returns with an error if <file> does not exists\n\tif no line number is given the whole contents of the file is requested\n");
        }
        //help writeT
        else if (strcmp(tokens[1], "writeT") == 0) {
            sprintf(shm_data, "%s", "Usage : writeT <file> <line #> <string> \n\twrite command writes the content of “string” to the  #th  line the <file>, \n\tif the line # is not given writes to the end of file\n");
        }
        //help upload
        else if (strcmp(tokens[1], "upload") == 0) {
            sprintf(shm_data, "%s", "Usage : upload <file> \n\tupload command uploads the <file> from the current working directory of client to the server's directory\n");
        }
        //help download
        else if (strcmp(tokens[1], "download") == 0) {
            sprintf(shm_data, "%s", "Usage : download <file> \n\tdownload command downloads the <file> from the server's directory to the current working directory of client\n");
        }
        //help quit
        else if (strcmp(tokens[1], "quit") == 0) {
            sprintf(shm_data, "%s", "Usage : quit \n\tquit command quits client and request server save log file\n");
        }
        //help killServer
        else if (strcmp(tokens[1], "killServer") == 0) {
            sprintf(shm_data, "%s", "Usage : killServer \n\tquit command kill the server and quit\n");
        }
        //help undefined
        else {
            sprintf(shm_data, "%s", "Not correct command!\n");
        }
    }
}



void run_child_server(char* pid){
   int child = fork();
    if(child == 0){
        //Set semaphore path
        char sem_path[64] = ""; 
        strcat(sem_path,SEM_SERVERCLIENT_PATH);
        strcat(sem_path,pid);

        //Set shared memory path
        char shm_path[64] = ""; 
        strcat(shm_path,SHM_SERVERCLIENT_PATH);
        strcat(shm_path,pid);
        

        /* Semaphore to synchorinize handle operations between server and client*/
        sem_t* sem = sem_open(sem_path, O_CREAT, 0666, 1);
        if (sem == SEM_FAILED) {
            perror("ERROR : Shared memory for communication between server and client");
            exit(1);
        }

        /*Shared memory to communicate between server and client*/
        int shm_fd = shm_open(shm_path, O_CREAT | O_RDWR | O_TRUNC, 0666);
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
        char* shm_data = mmap(NULL, SHM_SERVERCLIENT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shm_data == MAP_FAILED) {
            perror("ERROR : Shared memory couldn't be mapped");
            exit(1);
        }


        /* Join main loop */
        while(1){
            fflush(stdout);

            sem_wait(sem);

            /*Read request*/
            char** tokens = NULL;
            int tokenCount = 0;
            readRequest(shm_data, &tokens, &tokenCount);
            printTokens(tokens,tokenCount);

            shm_data[0] = '\0';

            /*Handle requests*/
            if(tokenCount > 0){

                /*Handle help request*/
                if(strcmp(tokens[0],"help") == 0)
                    handleHelp(tokens, tokenCount, shm_data);

                else if(strcmp(tokens[0],"list") == 0){
                    /* Write response */
                    shm_data[0] = '\0'; 
                    sprintf(shm_data,"%s","received : list\n");
                }
                else if(strcmp(tokens[0],"readF") == 0){
                    /* Write response */
                    shm_data[0] = '\0'; 
                    sprintf(shm_data,"%s","received : readF\n");
                }
                else if(strcmp(tokens[0],"writeT") == 0){
                    /* Write response */
                    shm_data[0] = '\0'; 
                    sprintf(shm_data,"%s","received : writeT\n");
                }
                else if(strcmp(tokens[0],"upload") == 0){
                    /* Write response */
                    shm_data[0] = '\0'; 
                    sprintf(shm_data,"%s","received : upload\n");
                }
                else if(strcmp(tokens[0],"download") == 0){
                    /* Write response */
                    shm_data[0] = '\0'; 
                    sprintf(shm_data,"%s","received : download\n");
                }
                else if(strcmp(tokens[0],"quit") == 0){
                    /* Write response */
                    shm_data[0] = '\0'; 
                    sprintf(shm_data,"%s","received : quit\n");
                }
                else if(strcmp(tokens[0],"killServer") == 0){
                    /* Write response */
                    shm_data[0] = '\0'; 
                    sprintf(shm_data,"%s","received : killServer\n");
                }
                else{
                    /* Write response */
                    shm_data[0] = '\0'; 
                    sprintf(shm_data,"%s","Not correct command!\n");
                }
            }
            
            //Free tokens
            freeTokens(tokens, tokenCount);

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

                    run_child_server(clientPid);
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
                    run_child_server(clientPid);
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



