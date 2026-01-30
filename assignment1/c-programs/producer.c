#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>

volatile sig_atomic_t shutdown_flag = 0;
volatile sig_atomic_t printstats_flag = 0;

// handles ctrl + c
void handle_sigint(int sig){
        shutdown_flag = 1;
}

// handles sigusr1 to print stats
void handle_sigusr1(int sig) {
    printstats_flag = 1;
}

int main(int argc, char *argv[]) {
        char *filename = NULL;
        int buffer_size = 4096;
        int opt;
        struct sigaction sa, sa_usr1;
	// SIGINT
        sa.sa_handler = handle_sigint;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags=0;
        sigaction(SIGINT, &sa,NULL);
	
	// SIGUSR1
	sa_usr1.sa_handler = handle_sigusr1;
	sigemptyset(&sa_usr1.sa_mask);
	sa_usr1.sa_flags = 0;
	sigaction(SIGUSR1, &sa_usr1, NULL);
	
        while ((opt = getopt(argc, argv, "f:b:")) != -1) {
                switch (opt) {
                        case 'f':
                                filename = optarg; // optarg contains the argument value
                                break;
                        case 'b':
                                buffer_size = atoi(optarg);
                                break;
                        default:
                                fprintf(stderr, "Usage: %s [-f file] [-b size]\n", argv[0]);
                        exit(1);
                }
        }
        // read stdin (no -f) or open & read filename
        FILE *input = stdin;
        if (filename != NULL) {
                input = fopen(filename, "r");
                if (input == NULL) {
                        perror("file open error");
                        exit(1);
                }
        }
        char buffer[buffer_size];
        size_t n;
	size_t lines_read = 0;
        // reads data from input and writes it out to stdout
        while (!shutdown_flag && (n = fread(buffer, 1, buffer_size, input)) > 0) {
                fwrite(buffer, 1, n, stdout);
		usleep(1000);
		for(size_t i = 0; i < n; i++) { // keeps track of # of lines read
			if(buffer[i] == '\n')
				lines_read++;
		}
		if(printstats_flag == 1) { // handles SIGUSR1 to print # of lines read
			printf("# of Lines read: %zu\n", lines_read);
			fflush(stdout);
			printstats_flag = 0;
		}
        }
        // closes file if it was opened
        if (input != stdin) {
                fclose(input);
        }
        return 0;
}


