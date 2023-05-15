#include "file_operations.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int openFile(const char* filename) {
    int fileDescriptor = open(filename, O_RDWR | O_CREAT, 0644);
    if (fileDescriptor == -1) {
        perror("Error opening file");
        return -1;
    }
    return fileDescriptor;
}

void closeFile(int fileDescriptor) {
    if (close(fileDescriptor) == -1) {
        perror("Error closing file");
    }
}

int readFile(const char* filename,char** fileContent) {
    int fileDescriptor = open(filename, O_RDONLY);
    if (fileDescriptor == -1) {
        perror("Error opening file");
        return -1;
    }

    off_t size = lseek(fileDescriptor, 0, SEEK_END);
    lseek(fileDescriptor, 0, SEEK_SET);

    *fileContent = malloc(size + 1);
    if (*fileContent == NULL) {
        perror("Memory allocation error");
        close(fileDescriptor);
        return -1;
    }

    ssize_t bytesRead = read(fileDescriptor, *fileContent, size);
    if (bytesRead == -1) {
        perror("Error reading file");
        free(*fileContent);
        close(fileDescriptor);
        return -1;
    }

    (*fileContent)[size] = '\0';

    close(fileDescriptor);
    return 0;
}

char* readTextFileLine(const char* filename, int lineNumber) {
    int fileDescriptor = open(filename, O_RDONLY);
    if (fileDescriptor == -1) {
        perror("Error opening file");
        return NULL;
    }

    char* buffer = NULL;
    size_t bufsize = 0;
    int currentLine = 0;

    while (getline(&buffer, &bufsize, fdopen(fileDescriptor, "r")) != -1) {
        if (++currentLine == lineNumber) {
            break;
        }
    }

    close(fileDescriptor);
    return buffer;
}

char* readBinaryFileLine(const char* filename, int lineNumber) {
    int fileDescriptor = open(filename, O_RDONLY);
    if (fileDescriptor == -1) {
        perror("Error opening file");
        return NULL;
    }

    char* buffer = NULL;
    size_t bufsize = 0;
    int currentLine = 0;

    while (getline(&buffer, &bufsize, fdopen(fileDescriptor, "r")) != -1) {
        if (++currentLine == lineNumber) {
            break;
        }
    }

    close(fileDescriptor);
    return buffer;
}

int writeFile(const char* path ,const char* filename, const char* content ,op_type op) {
    char* dir = malloc(sizeof(path) + sizeof(filename) + 1);
    sprintf(dir,"%s/%s",path,filename);
    int fileDescriptor;

    if(op == UPDATE_OP){
        fileDescriptor = open(dir, O_WRONLY | O_CREAT | O_EXCL, 0644);
    }
    else if(op == WRITE_OP){
        fileDescriptor = open(dir, O_APPEND | O_CREAT, 0644);
    }

    if (fileDescriptor == -1) {
        //perror("Error opening file");
        return -1;
    }

    ssize_t bytesWritten = write(fileDescriptor, content, strlen(content));
    if (bytesWritten == -1) {
        //perror("Error writing to file");
        return -1;
    }

    close(fileDescriptor);

    return 0;
}

void writeTextFileLine(const char* filename, const char* content, int lineNumber) {
    int fileDescriptor = open(filename, O_RDWR);
    if (fileDescriptor == -1) {
        perror("Error opening file");
        return;
    }

    char* buffer = NULL;
    size_t bufsize = 0;
    int currentLine = 0;
    off_t offset = 0;
    off_t start = -1;
    off_t end = -1;

    while (getline(&buffer, &bufsize, fdopen(fileDescriptor, "r")) != -1) {
        if (++currentLine == lineNumber) {
            start = offset;
            end = start + strlen(buffer);
            break;
        }
        offset = lseek(fileDescriptor, 0, SEEK_CUR);
    }

    if (start >= 0 && end >= 0) {
        lseek(fileDescriptor, start, SEEK_SET);
        ssize_t bytesWritten = write(fileDescriptor, content, strlen(content));
        if (bytesWritten == -1) {
            perror("Error writing to file");
        }
        off_t remaining = offset - end;
        if (remaining > 0) {
            char* temp = malloc(remaining);
            ssize_t bytesRead = read(fileDescriptor, temp, remaining);
            if (bytesRead == -1) {
                perror("Error reading file");
                free(temp);
            } else {
                lseek(fileDescriptor, start + strlen(content), SEEK_SET);
                ssize_t bytesWritten = write(fileDescriptor, temp, bytesRead);
                if (bytesWritten == -1) {
                    perror("Error writing to file");
                }
                free(temp);
            }
        }
    }

    close(fileDescriptor);
    if (buffer != NULL) {
        free(buffer);
    }
}
