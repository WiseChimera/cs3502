/*
Darren Ni
*/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Configuration - experiment with different values!
#define NUM_ACCOUNTS 5
#define NUM_THREADS 8
#define TRANSACTIONS_PER_THREAD 15
#define INITIAL_BALANCE 2000.0

// Updated Account structure with mutex (GIVEN)
typedef struct {
    int account_id;
    double balance;
    int transaction_count;
    pthread_mutex_t lock; // NEW: Mutex for this account
} Account;

// Global shared array - THIS CAUSES RACE CONDITIONS!
Account accounts[NUM_ACCOUNTS];

// STRATEGY 1: Lock Ordering
// transfer_safe() function to transfer money between accounts
void safe_transfer_ordered(int to_id, int from_id, double amount) {
	// locks lower account ID first, unlock in reverse order
	int first = (from_id < to_id) ? from_id : to_id;
	int second = (from_id > to_id) ? from_id : to_id;
	pthread_mutex_lock(&accounts[first].lock);
	// Simulate processing delay
	usleep(100);
	pthread_mutex_lock(&accounts[second].lock);
	// ===== CRITICAL SECTION =====
	// tranfers money from the fromAccount to toAccount 
	accounts[from_id].balance -= amount;
	accounts[from_id].transaction_count++;
	accounts[to_id].balance += amount;
	accounts[to_id].transaction_count++;
	// Release lock AFTER modifying shared data
	pthread_mutex_unlock(&accounts[second].lock);
	pthread_mutex_unlock(&accounts[first].lock);
}


// STRATEGY 3: Try-Lock with Backoff
void safe_transfer_trylock(int to_id, int from_id, double amount) {
	while(1) {
		if(pthread_mutex_trylock(&accounts[from_id].lock) != 0) {
			usleep(rand() % 500); // random backoff time between 0-499
			continue;
		}
		if(pthread_mutex_trylock(&accounts[to_id].lock) != 0) {
			pthread_mutex_unlock(&accounts[from_id].lock);
			usleep(rand() % 500); // random backoff time between 0-499
			continue;
		}
		break;
	}
	accounts[from_id].balance -= amount;
	accounts[from_id].transaction_count++;
	accounts[to_id].balance += amount;
	accounts[to_id].transaction_count++;
	pthread_mutex_unlock(&accounts[to_id].lock);
	pthread_mutex_unlock(&accounts[from_id].lock);
}


// Implement the thread function, Reference: OSTEP Ch. 27 for pthread function signature, 
// Appendix A.2 for void* parameter explanation
// tests lock ordering
void* teller_thread1(void* arg) {
	int teller_id = *(int*)arg; // GIVEN: Extract thread ID
	// Initialize thread-safe random seed, Reference: Section 7.2 "Random Numbers per Thread"
	unsigned int seed = time(NULL) ^ pthread_self();
	for (int i = 0; i < TRANSACTIONS_PER_THREAD; i++) {
		// Randomly select an account (0 to NUM_ACCOUNTS-1)
		int account_idTo = rand_r(&seed) % NUM_ACCOUNTS; // random account_id x selected
		int account_idFrom;
		do { // ensures that account_idFrom != account_idTo
    		account_idFrom = rand_r(&seed) % NUM_ACCOUNTS;
		} while (account_idFrom == account_idTo);
		// Generate random amount (1-100)
		double amount = rand_r(&seed) % 100 + 1;
		// Call appropriate function- safe_transfer_ordered() 
		safe_transfer_ordered(account_idTo, account_idFrom, amount);
		printf("Teller %d: Transferred $%.2f to Account %d from Account %d\n", teller_id, amount, account_idTo, account_idFrom);
	}
	return NULL;
}
// tests trylock
void* teller_thread2(void* arg) {
	int teller_id = *(int*)arg; // GIVEN: Extract thread ID
	// Initialize thread-safe random seed, Reference: Section 7.2 "Random Numbers per Thread"
	unsigned int seed = time(NULL) ^ pthread_self();
	for (int i = 0; i < TRANSACTIONS_PER_THREAD; i++) {
		// Randomly select an account (0 to NUM_ACCOUNTS-1)
		int account_idTo = rand_r(&seed) % NUM_ACCOUNTS; // random account_id x selected
		int account_idFrom;
		do { // ensures that account_idFrom != account_idTo
    		account_idFrom = rand_r(&seed) % NUM_ACCOUNTS;
		} while (account_idFrom == account_idTo);
		// Generate random amount (1-100)
		double amount = rand_r(&seed) % 100 + 1;
		// Call appropriate function- safe_transfer_trylock()
		safe_transfer_trylock(account_idTo, account_idFrom, amount);
		printf("Teller %d: Transferred $%.2f to Account %d from Account %d\n", teller_id, amount, account_idTo, account_idFrom);
	}
	return NULL;
}
// Add mutex cleanup in main(), Reference: man pthread_mutex_destroy
// Important: Destroy mutexes AFTER all threads complete!
void cleanup_mutexes() {
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        pthread_mutex_destroy(&accounts[i].lock);
    }
}

// GIVEN: Example of mutex initialization
void initialize_accounts() {
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        accounts[i].account_id = i;
        accounts[i].balance = INITIAL_BALANCE;
        accounts[i].transaction_count = 0;
        // Initialize the mutex
        pthread_mutex_init(&accounts[i].lock, NULL);
    }
}

int main() {
	int choice;
	int exit = 1;
	while(exit) {
		printf("\n--- Menu ---");
		printf("\n1. Test lock ordering\n2. Test trylock with backoff\n3. Exit\nChoose an option: ");
		scanf("%d", &choice);
		if(choice == 1) {
			printf("=== Phase 4: Deadlock Resolution ===\n");
			printf("=== Testing lock ordering ===\n\n");
			// Initialize all accounts
			initialize_accounts();
			// Display initial state (GIVEN)
			printf("Initial State:\n");
			for (int i = 0; i < NUM_ACCOUNTS; i++) {
				printf(" Account %d: $%.2f\n", i, accounts[i].balance);
			}
			// Calculate expected final balance; Total money in system should remain constant!
			double expected_total = NUM_ACCOUNTS * INITIAL_BALANCE; 
			printf("\nExpected total: $%.2f\n\n", expected_total);
			// Create thread and thread ID arrays, Reference: man pthread_create for pthread_t type
			pthread_t threads[NUM_THREADS];
			int thread_ids[NUM_THREADS]; // Separate array for IDs
			struct timespec start, end;
			// start clock for performance measuring
			clock_gettime(CLOCK_MONOTONIC, &start);
			// Create all threads, Reference: man pthread_create
			for (int i = 0; i < NUM_THREADS; i++) {
				thread_ids[i] = i; // Store ID persistently
				pthread_create(&threads[i], NULL, teller_thread1, & thread_ids[i]);
			}
			// Wait for all threads to complete, Reference: man pthread_join
			for (int i = 0; i < NUM_THREADS; i++) {
				pthread_join(threads[i], NULL);
			}
			// end clock
			clock_gettime(CLOCK_MONOTONIC, &end);
			double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
			cleanup_mutexes();
			// Calculate and display results
			printf("\n=== Final Results (Lock Ordering) ===\n");
			double actual_total = 0.0;
			for (int i = 0; i < NUM_ACCOUNTS; i++) {
				printf("Account %d: $%.2f (%d transactions)\n", i, accounts[i].balance, accounts[i].transaction_count);
				actual_total += accounts[i].balance;
			}
			printf("\nExpected total: $%.2f\n", expected_total);
			printf("Actual total: $%.2f\n", actual_total);
			printf("Difference: $%.2f\n", actual_total - expected_total);
			printf("Time taken: %.4f seconds\n", elapsed);
			// Add race condition detection message; Instruct user to run multiple times
			if(expected_total != actual_total) {
				printf("RACE CONDITION DETECTED!\n");
				printf("Run the program multiple times to observe.\n");
			}
		}
		else if(choice == 2){
			printf("=== Phase 4: Deadlock Resolution ===\n");
			printf("=== Testing trylock with backoff ===\n\n");
			// Initialize all accounts
			initialize_accounts();
			// Display initial state (GIVEN)
			printf("Initial State:\n");
			for (int i = 0; i < NUM_ACCOUNTS; i++) {
				printf(" Account %d: $%.2f\n", i, accounts[i].balance);
			}
			// Calculate expected final balance; Total money in system should remain constant!
			double expected_total = NUM_ACCOUNTS * INITIAL_BALANCE; 
			printf("\nExpected total: $%.2f\n\n", expected_total);
			// Create thread and thread ID arrays, Reference: man pthread_create for pthread_t type
			pthread_t threads[NUM_THREADS];
			int thread_ids[NUM_THREADS]; // Separate array for IDs
			struct timespec start, end;
			// start clock for performance measuring
			clock_gettime(CLOCK_MONOTONIC, &start);
			// Create all threads, Reference: man pthread_create
			for (int i = 0; i < NUM_THREADS; i++) {
				thread_ids[i] = i; // Store ID persistently
				pthread_create(&threads[i], NULL, teller_thread2, & thread_ids[i]);
			}
			// Wait for all threads to complete, Reference: man pthread_join
			for (int i = 0; i < NUM_THREADS; i++) {
				pthread_join(threads[i], NULL);
			}
			// end clock
			clock_gettime(CLOCK_MONOTONIC, &end);
			double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
			cleanup_mutexes();
			// Calculate and display results
			printf("\n=== Final Results (Trylock with backoff) ===\n");
			double actual_total = 0.0;
			for (int i = 0; i < NUM_ACCOUNTS; i++) {
				printf("Account %d: $%.2f (%d transactions)\n", i, accounts[i].balance, accounts[i].transaction_count);
				actual_total += accounts[i].balance;
			}
			printf("\nExpected total: $%.2f\n", expected_total);
			printf("Actual total: $%.2f\n", actual_total);
			printf("Difference: $%.2f\n", actual_total - expected_total);
			printf("Time taken: %.4f seconds\n", elapsed);
			// Add race condition detection message; Instruct user to run multiple times
			if(expected_total != actual_total) {
				printf("RACE CONDITION DETECTED!\n");
				printf("Run the program multiple times to observe.\n");
			}
		}
		else if(choice == 3) {
			exit = 0;
		}
		else {
			printf("Invalid choice, try again.\n");
		}
	}
	return 0;
}
