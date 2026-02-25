/*
Darren Ni
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Configuration - experiment with different values!
#define NUM_ACCOUNTS 2
#define NUM_THREADS 2
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

// TODO 1: Implement complete transfer function
// Add balance checking (sufficient funds?)
// Add error handling
// transfer_safe() function to transfer money between accounts
void transfer_deadlock(int to_id, int from_id, double amount) {
    // Lock source account
    pthread_mutex_lock(&accounts[from_id].lock);
	printf("Thread %ld: Locked account %d\n", pthread_self(), from_id);
	// Simulate processing delay
	usleep(100);
	// Try to lock destination account
	printf("Thread %ld: Waiting for account %d\n", pthread_self(), to_id);
    pthread_mutex_lock(&accounts[to_id].lock);
    // ===== CRITICAL SECTION =====
	// Checks if source account has sufficient funds
	if(accounts[from_id].balance < amount) {
		printf("Thread %ld: Account %d has Insufficient funds.\n", pthread_self(), from_id);
		pthread_mutex_unlock(&accounts[to_id].lock);
		pthread_mutex_unlock(&accounts[from_id].lock);
		return;
	}
	// Transfer (never reached if deadlocked)
	accounts[from_id].balance -= amount;
	accounts[from_id].transaction_count++;
	accounts[to_id].balance += amount;
	accounts[to_id].transaction_count++;
    // Release lock AFTER modifying shared data
    pthread_mutex_unlock(&accounts[to_id].lock);
    pthread_mutex_unlock(&accounts[from_id].lock);
}

// These two thread creation functions will deadlock
void* thread_deadlock1(void* arg) {
	unsigned int seed = time(NULL) ^ pthread_self();
	double amount = rand_r(&seed) % 100 + 1;
	transfer_deadlock(0, 1, amount);
	return NULL;
}
void* thread_deadlock2(void* arg) {
	unsigned int seed = time(NULL) ^ pthread_self();
	double amount = rand_r(&seed) % 100 + 1;
	transfer_deadlock(1, 0, amount);
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

// TODO 3: Implement deadlock detection
int main() {
	printf("=== Phase 3: Deadlock Creation Demo ===\n\n");
	// Initialize all accounts
	initialize_accounts();
	// Display initial state 
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
	time_t start = time(NULL);
	pthread_create(&threads[0], NULL, thread_deadlock1, & thread_ids[0]);
	pthread_create(&threads[1], NULL, thread_deadlock2, & thread_ids[1]);
	while(1) {
		// check every second
		sleep(1);
		time_t now = time(NULL);
		int count_1 =  accounts[0].transaction_count;
		int count_2 =  accounts[1].transaction_count;
		// no deadlock if transaction count incremented on both accounts
		if(count_1 + count_2 >= 2) {
			printf("\n=== Transaction successfully completed ===\n");
			break;
		}
		// deadlock suspected after more than 5 seconds
		if(now-start > 5) {
			printf("\n=== Deadlock Suspected ===\n");
			printf("Program terminating due to deadlock.\n");
			break;
		}
	}
	// Wait for all threads to complete, Reference: man pthread_join
	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}
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
	return 0;
}
