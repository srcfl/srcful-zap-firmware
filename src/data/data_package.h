#ifndef DATA_PACKAGE_H
#define DATA_PACKAGE_H

// Maximum data size (adjust as needed)
#define MAX_DATA_SIZE 2048

// Define a struct for the data package using char arrays instead of Strings
typedef struct {
    char data[MAX_DATA_SIZE];  // Generic data array (can store JWT or any other data)
    unsigned long timestamp;   // Timestamp when the data was created
} DataPackage;

#endif // DATA_PACKAGE_H