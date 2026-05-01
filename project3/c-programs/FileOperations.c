#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <sys/file.h>
#include "FileOperations.h"

//create file
int create_file(const char *path) 
{
    // O_CREAT -> Create file if it doesn't exist, O_EXCL -> Fail if file exists
    int fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if(fd == -1) 
    {
        return -1; 
    }

    close(fd);
    return 0;
}

// create directory
int create_directory(const char *path) 
{
    if(mkdir(path, 0755) == -1) 
    {
        return -1; 
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
    // file lock (non-blocking) for reading-- multiple readers allowed
    if (flock(fd, LOCK_SH | LOCK_NB) == -1) {
        close(fd);
        errno = EBUSY;
        return NULL;
    }
    struct stat st;
    // get file info (size, etc.)
    if (fstat(fd, &st) == -1)
    {
        flock(fd, LOCK_UN);
        close(fd);
        return NULL;
    }
    // handles case where size is 0
    if (st.st_size == 0)
    {
        flock(fd, LOCK_UN);
        close(fd);
        char *empty = malloc(1);
        if (empty) empty[0] = '\0';
        return empty;
    }
    // Allocate memory for file content and +1 for null terminator
    char *result = malloc(st.st_size + 1);
    // check if memory allocation failed
    if(!result)
    {
        flock(fd, LOCK_UN);
        close(fd);
        return NULL;
    }
    ssize_t total = 0;
    ssize_t bytes;
    // read entire file (handles partial reads)
    while (total < st.st_size)
    {
        bytes = read(fd, result + total, st.st_size - total);
        // check if operation (read) failed
        if(bytes <= 0) 
        {
            free(result);
            flock(fd, LOCK_UN);
            close(fd);
            return NULL;
        }
        total += bytes;
    }
    // null-terminate so it's a valid string
    result[st.st_size] = '\0';
    flock(fd, LOCK_UN);
    close(fd);
    // note to self: remember to free() the string (malloc)
    return result;
}

// update file
int update_file(const char *path, const char *content) 
{
    // O_WRONLY -> Write only, O_TRUNC -> overwrite existing file contents
    int fd = open(path, O_WRONLY);
    if (fd == -1)
    {
        return -1; 
    }
    // file lock (non-blocking) for writing
    if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
        close(fd);
        errno = EBUSY;
        return -1;
    }
    // truncate after locking
    if (ftruncate(fd, 0) == -1) {
        flock(fd, LOCK_UN);
        close(fd);
        return -1;
    }
    // writes new content into the file
    ssize_t written = write(fd, content, strlen(content));
    if (written == -1)
    {
        flock(fd, LOCK_UN);
        close(fd);
        return -1;
    }
    flock(fd, LOCK_UN);
    close(fd);
    return 0;
}

// delete file
int delete_file(const char *path)
{
    if (unlink(path) == -1)
    {
        return -1; 
    }

    return 0;
}

// delete directory
int delete_directory(const char *path)
{
    if (rmdir(path) == -1)
    {
        return -1; 
    }

    return 0;
}

// renames file/directory
int rename_item(const char *old_path, const char *new_path) 
{
    if (rename(old_path, new_path) == -1)
    {
        return -1; 
    }

    return 0;
}

int navigate(const char *path)
{
    struct stat st;
    if (stat(path, &st) == -1)
    {
        return -1; 
    }
    if (!S_ISDIR(st.st_mode))
    {
        errno = ENOTDIR;
        return -1;
    }
    return 0;
}
