#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

typedef enum  {
    UPLOAD_OP,
    WRITE_OP,
    DOWNLOAD_OP
} op_type;

// Function to open a file
int openFile(const char* filename);

// Function to close a file
void closeFile(int fileDescriptor);

int readWholeContent(char* fileDir,char* shm_data);

int readSpecificLineContent(const char* fileDir, char* shm_data , int lineNumber);

// Function to read a single line from a text file
char* readTextFileLine(const char* filename, int lineNumber);

// Function to read a single line from a binary file
char* readBinaryFileLine(const char* filename, int lineNumber);

int copyFile(const char* path, const char* filePath, op_type op);

// Function to write content to a specific line in a text file
void writeTextFileLine(const char* filename, const char* content, int lineNumber);

#endif
