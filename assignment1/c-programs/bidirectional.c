#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
	int pipe1[2]; // from Parent to Child, pipe1[1] --> parent writes
	int pipe2[2]; // from Child to Parent, pipe2[1] --> child writes
	pid_t pid;
	if(pipe(pipe1) == -1) {
		printf("pipe1 failed to open");
		exit(1);
	}
	if(pipe(pipe2) == -1) {
		printf("pipe2 failed to open");
		exit(1);
	}

	pid = fork();
	if(pid == -1) {
		printf("fork failed");
		exit(1);
	} else if(pid == 0) {
		// Child process
		char buffer[100];
		close(pipe1[1]); // close write from parent to child
		close(pipe2[0]); // close read from child to parent
		read(pipe1[0], buffer, sizeof(buffer)); // receives message from parent
		printf("Child received: %s\n", buffer);
		char reply[] = "Reply from child";
		write(pipe2[1], reply, strlen(reply) + 1); // replies to parent
		printf("Child replies: %s\n", reply);
		close(pipe1[0]);
		close(pipe2[1]);
	} else {
		// Parent process
		char buffer[100];
		close(pipe1[0]); // close read, from parent to child, parent writes
		close(pipe2[1]); // close write, from child to parent, parent to read
		char msg[] = "Hello from parent";
		write(pipe1[1], msg, strlen(msg) + 1); // parent writes to child
		read(pipe2[0], buffer, sizeof(buffer)); // parent receives reply from child
		printf("Parent received: %s\n", buffer);
		
		close(pipe1[1]);
		close(pipe2[0]);
	}
	return 0;
}
