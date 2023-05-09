#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#define FIFO_PATH "/tmp/clientFifo"
#define SHM_PATH "tmp/shm"
#define SEM_PATH "tmp/sem"

int main()
{
    int fd;
    char buf[256];
    ssize_t n;

    //Convert pid to char array 
    int pid = getpid();
    char pid_s[32] = "";
    snprintf(pid_s,32,"%d",pid);
    //printf("%s\n",pid_s);
 
    
    // Set fifo path 
    char fifo_path[64] = ""; 
    strcat(fifo_path,FIFO_PATH);
    strcat(fifo_path,pid_s);
    //printf("%s\n",fifo_path);

    
    // Create the named FIFO 
    if(mkfifo(fifo_path, 0666) == -1){
        printf("ERROR : clientFifo couldn't be created");
        return -1;
    }
    
    printf("Server Started PID : %d\n",pid);
    printf("Waiting for clients...\n");


    // Open the named FIFO for reading 
    fd = open(fifo_path, O_RDONLY);
    if(fd == -1){
        printf("ERROR : clientFifo couldn't be opened");
        return -1;
    }

    // Read data from the FIFO 
    while (1) {
        ssize_t n = read(fd, buf, sizeof(buf));
        
        if(n == -1){
            printf("ERROR : clientFifo couldn't be read!");
        } 
        else if (n > 0) {
            buf[n] = '\0';  // Add null terminator
            printf("Received data: %s\n", buf);
        }
        //clear buffer
        memset(buf, 0, sizeof(buf));  
    }

    close(fd);
    unlink(fifo_path);
    return 0;
}



