/*
Darren Ni
CS3502 A2
producer.c
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
	int id = atoi(argv[1]); // producer id in the 2nd argument
	int num_items = atoi(argv[2]); // number of items in the 3rd argument

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
	// initialized fields
	ptr->head = 0;
	ptr->tail = 0;
	ptr->count = 0;
	// opening three semaphores
	sem_t* empty = sem_open("/sem_empty", O_CREAT, 0644, BUFFER_SIZE);
	sem_t* full = sem_open("/sem_full", O_CREAT, 0644, 0);
	sem_t* mutex = sem_open("/sem_mutex", O_CREAT, 0644, 1);

	// writes to shared buffer
	for(int i = 0; i < num_items; i++) {
		sem_wait(empty); // wait for an empty spot
		sem_wait(mutex); // enter critical section

		item_t item;
		item.producer_id = id;	// producer id of the item
		item.value = item.producer_id * 1000 + i; // value of the item
		ptr->buffer[ptr->head] = item; // adds/overwrites item to the circular buffer
		ptr->head = (ptr->head + 1) % BUFFER_SIZE; // redirects head of buffer
		ptr->count++; // increases item count

		sem_post(mutex); // exit critical section
		sem_post(full); // signal that an item is available
		printf("Producer %d: Produced value %d\n", item.producer_id, item.value);
	}
	shmdt(ptr); // detach
	// close semaphores
	sem_close(empty);
	sem_close(full);
	sem_close(mutex);
}
