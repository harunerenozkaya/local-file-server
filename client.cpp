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

using namespace std;

int main(int argc, char *argv[])
{   
    int fd;

    /*Convert pid to char array */
    int pid = getpid();
    char pid_s[32] = "";
    snprintf(pid_s,32,"%d",pid);
    printf("PID : %s\n",pid_s);

    /* Set fifo path */
    char fifo_path[64] = ""; 
    strcat(fifo_path,FIFO_PATH);
    strcat(fifo_path,argv[2]);
    printf("FIFO_PATH : %s\n",fifo_path);

    /* Open the named FIFO for writing */
    fd = open(fifo_path, O_WRONLY);

    if(fd == -1){
        printf("ERROR : clientFifo couldn't be opened");
        return -1;
    }
    
    /* Write data to the FIFO */
    if(write(fd,pid_s, strlen(pid_s)) == -1){
        printf("ERROR : couldn't be writed to fifo");
        return -1;
    }

    /* Close the FIFO */
    close(fd);
    
    return 0;
}
