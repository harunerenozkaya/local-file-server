#include "file_operations.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int openFile(const char* filename) {
    int fileDescriptor = open(filename, O_RDWR , 0644);
    if (fileDescriptor == -1) {
        return -1;
    }
    return fileDescriptor;
}

void closeFile(int fileDescriptor) {
    if (close(fileDescriptor) == -1) {
        perror("Error closing file");
    }
}

int readWholeContent(char* fileDir,char* shm_data) {
    FILE* file = fopen(fileDir, "r");
    if (file == NULL) {
        //perror("Error opening file");
        return -1;
    }

    // Seek to the end of the file to determine its size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for file contents
    char* fileContent = malloc(fileSize + 1); // +1 for null terminator
    if (fileContent == NULL) {
        //perror("Error allocating memory");
        fclose(file);
        return -1;
    }

    // Read the entire file into fileContent
    size_t bytesRead = fread(fileContent, 1, fileSize, file);
    if (bytesRead != (size_t)fileSize) {
        //perror("Error reading file");
        free(fileContent);
        fclose(file);
        return -1;
    }

    // Null-terminate the file content
    fileContent[fileSize] = '\0';

    // Copy the file content to shm_data
    strcpy(shm_data, fileContent);

    // Clean up resources
    free(fileContent);
    fclose(file);

    return 0;
}

int readSpecificLineContent(const char* fileDir, char* shm_data , int lineNumber) {
    FILE* file = fopen(fileDir, "r");
    if (file == NULL) {
        sprintf(shm_data, "Error: An error occured when reading content.");
        return -1;
    }

    int currentLine = 1;
    char line[1024]; // Adjust the line length as per your needs

    // Find the desired line
    while (fgets(line, sizeof(line), file) != NULL) {
        if (currentLine == lineNumber) {
            // Copy the line content to shm_data
            strcpy(shm_data, line);
            fclose(file);
            return 0;
        }
        currentLine++;
    }

    // Line number not found
    sprintf(shm_data, "Error: Line number %d not found in the file.\n", lineNumber);
    fclose(file);
    return -1;
}

int copyFile(const char* path, const char* filePath, op_type op) {
    const char* lastSlash = strrchr(filePath, '/');
    const char* fileName = (lastSlash != NULL) ? lastSlash + 1 : filePath;
    char* dir = malloc(strlen(path) + strlen(fileName) + 1);
    sprintf(dir, "%s/%s", path, fileName);

    FILE* readStream = fopen(filePath,"rb");
    if (readStream == NULL) {
        free(dir);
        //perror("Error opening file");
        return -1;
    }

    FILE* writeStream;
    if (op == DOWNLOAD_OP) {
        writeStream = fopen(dir, "wb");
    } 
    else if(op == UPLOAD_OP){
        int fd = open(dir, O_CREAT | O_EXCL | O_WRONLY, 0666);
        if(fd < 0){
            free(dir);
            return -1;
        }
        writeStream = fdopen(fd, "wb");
    }
   
    if (writeStream == NULL) {
        free(dir);
        // perror("Error opening file");
        return -1;
    }

    int buffer_size = 4096;
    char* buffer = malloc(buffer_size);
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, buffer_size, readStream)) > 0) {
        fwrite(buffer, 1, bytesRead, writeStream);
    }

    fclose(writeStream);
    fclose(readStream);
    free(dir);

    return 0;
}

int writeWholeContent(const char* filePath, char* tokens[], int tokenCount, char* shm_data) {
    int file = open(filePath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (file == -1) {
        sprintf(shm_data, "Error: Unable to access to file.\n");
        return -1;
    }

    if (flock(file, LOCK_EX) == -1) {
        sprintf(shm_data, "Error: Unable to acquire lock on the file.\n");
        close(file);
        return -1;
    }

    for (int i = 2; i < tokenCount; i++) {
        ssize_t bytesWritten = write(file, tokens[i], strlen(tokens[i]));
        if (bytesWritten == -1) {
            close(file);
            return -1;
        }
        write(file, " ", 1);
    }

    flock(file, LOCK_UN);
    if (close(file) == -1) {
        return -1;
    }

    return 0;
}

int writeLineContent(const char* filePath, char* tokens[], int tokenCount, int lineNumber ,char* shm_data) {
    FILE* file = fopen(filePath, "r+");
    if (file == NULL) {
        //printf("Unable to open file %s.\n", filePath);
        return -1;
    }

    // Get the file descriptor from the FILE*
    int fileDescriptor = fileno(file);

    if (flock(fileDescriptor, LOCK_EX) == -1) {
        sprintf(shm_data, "Error: Unable to acquire lock on the file.\n");
        close(fileDescriptor);
        return -1;
    }

    // Move the file pointer to the beginning of the target line
    int currentLineNumber = 1;
    while (currentLineNumber < lineNumber) {
        char c = fgetc(file);
        if (c == '\n') {
            currentLineNumber++;
        }
        if(c == EOF){
            break;
        }
    }

    // Check if the specified line number is valid
    if (currentLineNumber != lineNumber) {
        sprintf(shm_data, "Error: Line number %d not found in the file.\n", lineNumber);
        flock(fileDescriptor, LOCK_UN);
        fclose(file);
        return -1;
    }

    // Remember the starting position and length of the target line
    long startPosition = ftell(file);
    long lineLength = 0;
    int ch;
    while ((ch = fgetc(file)) != '\n' && ch != EOF) {
        lineLength++;
    }

    // Go back to the starting position of the target line
    fseek(file, startPosition, SEEK_SET);

    // Replace the line with tokens
    int currentTokenCount = 3;
    while (currentTokenCount < tokenCount) {
        fputs(tokens[currentTokenCount], file);
        fputc(' ',file);
        currentTokenCount++;
    }

    //Replace with space to end of the line
    while(1){
        char c = fgetc(file);
        if(c != '\n' && c != EOF){
            fseek(file,-1,SEEK_CUR);
            fputs(" ",file);
        }
        else
            break;
    }

    flock(fileDescriptor, LOCK_UN);
    fclose(file);
    return 0;
}
