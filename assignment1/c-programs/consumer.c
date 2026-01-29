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
        int opt;
        long max_lines = -1;
        int verbose = 0;
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

        while ((opt = getopt(argc, argv, "n:v")) != -1) {
                switch (opt) {
                        case 'n':
                                max_lines = atoi(optarg); // optarg contains the argument value
                                break;
                        case 'v': // turns on verbose mode
                                verbose = 1;
                                break;
                        default:
                                fprintf(stderr, "Usage: %s [-n max] [-v]\n", argv[0]);
                                exit(1);
                }
        }
        char *line = NULL;
        size_t len = 0;
        ssize_t read;
        long line_count = 0;
        long char_count = 0;
        // reads from stdin
        while (!shutdown_flag && (read = getline(&line, &len, stdin)) != -1) {
                if (max_lines != -1 && line_count >= max_lines) // max lines reached
                        break;
                line_count++; // counts line read
                char_count += read; // adds total character in that line
                if(verbose == 1)
                        fputs(line, stdout); // echoes lines as read
		if(printstats_flag == 1) {
			fprintf(stderr, "# of lines read: %ld\n", line_count);
			fprintf(stderr, "# of characters read: %ld\n", char_count);
			fflush(stderr);
			printstats_flag = 0;
		}
        }
        // print statistics to stderr 
        fprintf(stderr, "# of lines: %ld\n", line_count);
        fprintf(stderr, "# of characters: %ld\n", char_count);
}
