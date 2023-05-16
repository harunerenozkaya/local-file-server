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

int copyFile(const char* path, const char* filePath, op_type op);

int writeWholeContent(const char* filePath, char* tokens[], int tokenCount, char* shm_data);

int writeLineContent(const char* filePath, char* tokens[], int tokenCount, int lineNumber ,char* shm_data);

#endif
