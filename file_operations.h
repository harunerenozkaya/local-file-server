#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

typedef enum  {
    UPDATE_OP,
    WRITE_OP
} op_type;

// Function to open a file
int openFile(const char* filename);

// Function to close a file
void closeFile(int fileDescriptor);

// Function to read all content from a file
int readFile(const char* filename, char** fileContent);

// Function to read a single line from a text file
char* readTextFileLine(const char* filename, int lineNumber);

// Function to read a single line from a binary file
char* readBinaryFileLine(const char* filename, int lineNumber);

// Function to write content to a file
int copyFile(const char* path, const char* filePath, op_type op);

// Function to write content to a specific line in a text file
void writeTextFileLine(const char* filename, const char* content, int lineNumber);

#endif
