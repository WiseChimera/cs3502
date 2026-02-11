/*
Darren Ni
CS3502 A2
consumer.c
*/
#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
	if(argc != 3) {		// if the argument count != 3, wrong command usage
		fprintf(stderr, "Usage: %s <id> <num_items>\n", argv[0]); // prints error message
		exit(1);
	}

	int id = atoi(argv[1]); // consumer id in the 2nd argument
	int num_items = atoi(argv[2]); // number of items to consume in 3rd argument

	if(id < 0 || num_items < 0) {
		fprintf(stderr, "Invalid ID and num_items\n");
		exit(1);
	}

	int shm_id = shmget(SHM_KEY, sizeof(shared_buffer_t), IPC_CREAT | 0666); // create/get existing shared memory segment
	if(shm_id == -1) { // check shmget
		perror("shmget failed");
		exit(1);
	}

	shared_buffer_t* ptr = (shared_buffer_t*)shmat(shm_id, NULL, 0); // attach to the process's address space
	if(ptr == (void*)-1) { //  Check that shmat() didn't return (void*)-1
		perror("shmat failed");
		exit(1);
	}
	
	// opening three semaphores
	sem_t* empty = sem_open("/sem_empty", O_CREAT, 0644, BUFFER_SIZE);
	sem_t* full = sem_open("/sem_full", O_CREAT, 0644, 0);	
	sem_t* mutex = sem_open("/sem_mutex", O_CREAT, 0644, 1);
	for(int i = 0; i < num_items; i++) {
		sem_wait(full); // wait for an item
		sem_wait(mutex); // enter critical section

		item_t item = ptr->buffer[ptr->tail]; // reads item from buffer tail
		ptr->tail = (ptr->tail + 1) % BUFFER_SIZE; // redirect tail of buffer
		ptr->count--; // decrease count of items

		sem_post(mutex); // exit critical section
		sem_post(empty); // signal that a slot is available
		printf("Consumer %d: Consumed value %d from Producer %d\n", id, item.value, item.producer_id);
	}
	shmdt(ptr); // detach 
	// close semaphores
	sem_close(empty);
	sem_close(full);
	sem_close(mutex);
}
