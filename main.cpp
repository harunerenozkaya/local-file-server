#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

using namespace std;

// Define the name of the named semaphore
const char *sem_name = "/my_semaphore";

int main()
{
    // Open the named semaphore for reading and writing
    sem_t *sem = sem_open(sem_name, O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) {
        cerr << "Failed to open semaphore" << endl;
        return 1;
    }

    // Fork five child processes
    for (int i = 0; i < 5; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            cerr << "Failed to fork child process" << endl;
            return 1;
        }
        else if (pid == 0) {
            // Child process
            cout << "Child process " << i << " started" << endl;

            // Wait on the semaphore
            sem_wait(sem);
            cout << "Child process " << i << " acquired semaphore" << endl;

            // Open the file for writing
            int fd = open("my_file.txt",  O_CREAT | O_WRONLY | O_APPEND);
            if (fd < 0) {
                cerr << "Child process " << i << " failed to open file" << endl;
                sem_post(sem); // Release the semaphore before exiting
                return 1;
            }

            // Write to the file
            dprintf(fd, "Child process %d wrote to the file\n", i);

            // Close the file and release the semaphore
            close(fd);
            sem_post(sem);
            cout << "Child process " << i << " released semaphore" << endl;

            return 0;
        }
    }

    // Parent process
    cout << "Parent process started" << endl;

    // Wait for all child processes to exit
    for (int i = 0; i < 5; i++) {
        wait(NULL);
    }

    // Close the named semaphore
    sem_close(sem);
    sem_unlink(sem_name);

    cout << "Parent process finished" << endl;

    return 0;
}
