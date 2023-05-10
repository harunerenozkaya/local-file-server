#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>


#define FIFO_SERVER_PATH "/tmp/server_fifo"
#define FIFO_CLIENT_PATH "/tmp/client_fifo"
#define SHM_PATH "tmp/shm"
#define SEM_PATH_SERVER "tmp/semServer"
#define SEM_PATH_CLIENT "tmp/semClient"


int main(int argc, char *argv[])
{   
    int fd_server;
    int fd_client;
    char buf_read[256] = "";
    char buf_write[256] = "";

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
    
    while(1){
        ssize_t n = read(fd_client, buf_read, sizeof(buf_read));
        if(n == -1){
            printf("ERROR : Server response couldn't be read!");
            return -1;
        }
        else if(n > 0){
            char connectionStatus[5] = "";
            
            printf("%s",buf_read);

            strncpy(connectionStatus,buf_read,5);
            if(strcmp(connectionStatus,"ERROR") == 0)
                return -1;
            
        }
        //clear buffer
        memset(connectData, 0, sizeof(connectData)); 
        memset(buf_read, 0, sizeof(buf_read)); 
    }

    /* Close the FIFO */
    close(fd_server);
    close(fd_client);
    unlink(fifo_client_path);
    
    return 0;
}
