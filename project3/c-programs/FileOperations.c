#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include "FileOperations.h"

//create file
int create_file(const char *path) 
{
    // O_CREAT -> Create file if it doesn't exist, O_EXCL -> Fail if file exists
    int fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if(fd == -1) 
    {
        return -1; // errno already set
    }

    close(fd);
    return 0;
}

// create directory
int create_directory(const char *path) 
{
    if(mkdir(path, 0755) == -1) 
    {
        return -1; // errno already set
    }
    return 0;
}

// read file
char *read_file(const char *path) 
{
    // Open file in read-only
    int fd = open(path, O_RDONLY);
    if(fd == -1) 
    {
        return NULL; // errno already set
    }

    // temp buffer to store file data
    char temp[4096];

    // reads up to sizeof(temp)-1 bytes from the file
    ssize_t bytes = read(fd, temp, sizeof(temp) - 1);

    // check if operation (read) failed
    if(bytes == -1) 
    {
        close(fd);
        return NULL;
    }

    // null-terminate so it's a valid string
    temp[bytes] = '\0';

    close(fd);

    // Allocate memory for returned string and +1 for null terminator
    char *result = malloc(bytes + 1);

    // check if memory allocation failed
    if(!result)
        return NULL;

    // copy data from temp to result
    strcpy(result, temp);

    // note to self: remember to free() the string (malloc)
    return result;
}

// update file
int update_file(const char *path, const char *content) 
{
    // O_WRONLY -> Write only, O_TRUNC -> overwrite existing file contents
    int fd = open(path, O_WRONLY | O_TRUNC);
    if (fd == -1)
    {
        return -1; // errno already set
    }

    // Write new content into the file
    write(fd, content, strlen(content));

    close(fd);
    return 0;
}

// delete file
int delete_file(const char *path)
{
    if (unlink(path) == -1)
    {
        return -1; // errno already set
    }

    return 0;
}

// delete directory
int delete_directory(const char *path)
{
    if (rmdir(path) == -1)
    {
        return -1; // errno already set
    }

    return 0;
}

// renames file/directory
int rename_item(const char *old_path, const char *new_path) 
{
    if (rename(old_path, new_path) == -1)
    {
        return -1; // errno already set
    }

    return 0;
}
