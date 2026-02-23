/*
Darren Ni
*/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Configuration - experiment with different values!
#define NUM_ACCOUNTS 4
#define NUM_THREADS 6
#define TRANSACTIONS_PER_THREAD 10
#define INITIAL_BALANCE 1000.0

// Updated Account structure with mutex (GIVEN)
typedef struct {
    int account_id;
    double balance;
    int transaction_count;
    pthread_mutex_t lock; // NEW: Mutex for this account
} Account;

// Global shared array - THIS CAUSES RACE CONDITIONS!
Account accounts[NUM_ACCOUNTS];

// transfer_safe() function to transfer money between accounts
void transfer_safe(int to_id, int from_id, double amount) {
    // Acquire lock BEFORE accessing shared data
    pthread_mutex_lock(&accounts[to_id].lock);
    pthread_mutex_lock(&accounts[from_id].lock);
    // ===== CRITICAL SECTION =====
    // Only ONE thread can execute this at a time for this account
    // tranfers money from the fromAccount to toAccount 
	accounts[from_id].balance -= amount;
	accounts[from_id].transaction_count++;
	accounts[to_id].balance += amount;
	accounts[to_id].transaction_count++;
    // Release lock AFTER modifying shared data
    pthread_mutex_unlock(&accounts[to_id].lock);
    pthread_mutex_unlock(&accounts[from_id].lock);
}

// Implement the thread function, Reference: OSTEP Ch. 27 for pthread function signature, 
// Appendix A.2 for void* parameter explanation
void* teller_thread(void* arg) {
	int teller_id = *(int*)arg; // GIVEN: Extract thread ID
	// Initialize thread-safe random seed, Reference: Section 7.2 "Random Numbers per Thread"
	unsigned int seed = time(NULL) ^ pthread_self();
	for (int i = 0; i < TRANSACTIONS_PER_THREAD; i++) {
		// Randomly select an account (0 to NUM_ACCOUNTS-1)
		int account_idx = rand_r(&seed) % NUM_ACCOUNTS; // random account_id x selected
		int account_idy;
		do { // ensures that account_idy != account_idx
    		account_idy = rand_r(&seed) % NUM_ACCOUNTS;
		} while (account_idy == account_idx);
		// Generate random amount (1-100)
		double amount = rand_r(&seed) % 100 + 1;
		// Call appropriate function
		transfer_safe(account_idx, account_idy, amount);
		printf("Teller %d: Transferred $%.2f to Account %d from Account %d\n", teller_id, amount, account_idx, account_idy);
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
	struct timespec start, end;
	printf("=== Phase 2: Mutex Protection Demo ===\n\n");
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
	// Create all threads, Reference: man pthread_create
	clock_gettime(CLOCK_MONOTONIC, &start);
	for (int i = 0; i < NUM_THREADS; i++) {
		thread_ids[i] = i; // Store ID persistently
		pthread_create(&threads[i], NULL, teller_thread, & thread_ids[i]);
	}
	// Wait for all threads to complete, Reference: man pthread_join
	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
	cleanup_mutexes();
	// Calculate and display results
	printf("\n=== Final Results ===\n");
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
	return 0;
}
