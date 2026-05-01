#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <stdio.h>

int main()
{
    int fd = open("TestDirectory/smallTestFile.txt", O_WRONLY);
    if (fd == -1) return 1;

    if (flock(fd, LOCK_EX) == -1)
    {
        perror("lock failed");
        return 1;
    }

    printf("File locked. Press Enter to release...\n");
    getchar();

    flock(fd, LOCK_UN);
    close(fd);
}
