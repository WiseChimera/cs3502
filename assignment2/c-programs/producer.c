#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
	int opt;
	if(argc != 3) {		// if the argument count != 3, wrong command usage
		fprintf(stderr, "Usage: %s <id> <num_items>\n", argv[0]); // prints error message
		exit(1);
	}
	if(id < 0 || num_items < 0) {
		fprintf(stderr, "Invalid ID and num_items\n");
		exit(1);
	}
	int shm_id = shmget(SHM_KEY, sizeof(my_struct_t), IPC_CREAT | 0666); // create/get existing shared memory segment
	if(shm_id == -1) { // check shmget
		perror("shmget failed");
		exit(1);
	}

	shared_buffer_t* ptr = (shared_buffer_t*)shmat(shm_id, NULL, 0); // attach to the process's address space
	if(ptr == (void*)-1) { //  Check that shmat() didn't return (void*)-1
		perror("shmat failed");
		exit(1);
	}

	shmdt(ptr); // detach 
	// opening three semaphores
	sem_t* sem1 = sem_open("/sem_empty", O_CREAT, 0644, 0);
	sem_t* sem2 = sem_open("/sem_full", O_CREAT, 0644, 0);	
	sem_t* sem3 = sem_open("/sem_mutex", O_CREAT, 0644, 1);
	
	int producer_id = atoi(argv[1]); // producer id in the 2nd argument
	int num_items = atoi(argv[2]); // number of items in the 3rd argument

	// writes to shared buffer
	for(int i = 0; i < num_items; i++) {
		item_t item;
		item.id = producer_id;	// producer id of the item
		item.value = producer_id * 1000 + num_items; // value of the item 
		ptr->buffer[ptr->head] = item; // adds item to the circular buffer
		ptr->head = (buffer->head + 1) % BUFFER_SIZE;
	}
}
