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

// Account data structure (GIVEN)
typedef struct {
	int account_id;
	double balance;
	int transaction_count;
} Account;
// Global shared array - THIS CAUSES RACE CONDITIONS!
Account accounts[NUM_ACCOUNTS];

// transfer_unsafe() function to transfer money between accounts
void transfer_unsafe(int to_id, int from_id, double amount) {
	// gets the current balance of the accounts where the money is transferred to and from
	double to_balance = accounts[to_id].balance;
	double from_balance = accounts[from_id].balance;
	// MODIFY (simulate processing time)
	usleep(1); // This increases likelihood of race condition!
	// tranfers money from the fromAccount to toAccount 
	accounts[from_id].balance = from_balance - amount;
	accounts[from_id].transaction_count++;
	accounts[to_id].balance = to_balance + amount;
	accounts[to_id].transaction_count++;
}

// TODO 2: Implement the thread function
// Reference: See OSTEP Ch. 27 for pthread function signature
// Reference: Appendix A.2 for void* parameter explanation
void* teller_thread(void* arg) {
	int teller_id = *(int*)arg; // GIVEN: Extract thread ID
	// TODO 2a: Initialize thread-safe random seed
	// Reference: Section 7.2 "Random Numbers per Thread"
	// Hint: unsigned int seed = time(NULL) ^ pthread_self();
	unsigned int seed = time(NULL) ^ pthread_self();
	for (int i = 0; i < TRANSACTIONS_PER_THREAD; i++) {
		// TODO 2b: Randomly select an account (0 to NUM_ACCOUNTS-1)
		// Hint: Use rand_r(&seed) % NUM_ACCOUNTS
		int account_idx = rand_r(&seed) % NUM_ACCOUNTS; // random account_id x selected
		int account_idy;
		do { // ensures that account_idy != account_idx
    		account_idy = rand_r(&seed) % NUM_ACCOUNTS;
		} while (account_idy == account_idx);
		// TODO 2c: Generate random amount (1-100)
		double amount = rand_r(&seed) % 100 + 1;
		// TODO 2e: Call appropriate function
		transfer_unsafe(account_idx, account_idy, amount);
		printf("Teller %d: Transferred $%.2f to Account %d from Account %d\n", teller_id, amount, account_idx, account_idy);
	}
	return NULL;
}

// TODO 3: Implement main function
// Reference: See pthread_create and pthread_join man pages
int main() {
	printf("=== Phase 1: Race Conditions Demo ===\n\n");
	// TODO 3a: Initialize all accounts
	// Hint: Loop through accounts array
	// Set: account_id = i, balance = INITIAL_BALANCE, transaction_count = 0
	for(int i = 0; i < NUM_ACCOUNTS; i++) {
		accounts[i].account_id = i;
		accounts[i].balance = INITIAL_BALANCE;
		accounts[i].transaction_count = 0;
	}
	// Display initial state (GIVEN)
	printf("Initial State:\n");
	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		printf(" Account %d: $%.2f\n", i, accounts[i].balance);
	}
	// TODO 3b: Calculate expected final balance
	// Question: With random deposits/withdrawals, what should total be? initial_balance - total withdrawls + total deposits
	// Hint: Total money in system should remain constant!
	double expected_total = NUM_ACCOUNTS * INITIAL_BALANCE; 
	printf("\nExpected total: $%.2f\n\n", expected_total);
	// TODO 3c: Create thread and thread ID arrays
	// Reference: man pthread_create for pthread_t type
	pthread_t threads[NUM_THREADS];
	int thread_ids[NUM_THREADS]; // GIVEN: Separate array for IDs
	// TODO 3d: Create all threads
	// Reference: man pthread_create
	// Caution: See Appendix A.2 warning about passing &i in loop!
	for (int i = 0; i < NUM_THREADS; i++) {
		thread_ids[i] = i; // GIVEN: Store ID persistently
		// YOUR pthread_create CODE HERE
		// Format: pthread_create(&threads[i], NULL, teller_thread, & thread_ids[i]);
		pthread_create(&threads[i], NULL, teller_thread, & thread_ids[i]);
	}
	// TODO 3e: Wait for all threads to complete
	// Reference: man pthread_join
	// Question: What happens if you skip this step? 
	for (int i = 0; i < NUM_THREADS; i++) {
		// YOUR pthread_join CODE HERE
		pthread_join(threads[i], NULL);
	}
	// TODO 3f: Calculate and display results
	printf("\n=== Final Results ===\n");
	double actual_total = 0.0;
	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		printf("Account %d: $%.2f (%d transactions)\n", i, accounts[i].balance, accounts[i].transaction_count);
		actual_total += accounts[i].balance;
	}
	printf("\nExpected total: $%.2f\n", expected_total);
	printf("Actual total: $%.2f\n", actual_total);
	printf("Difference: $%.2f\n", actual_total - expected_total);
	// TODO 3g: Add race condition detection message
	// If expected != actual, print "RACE CONDITION DETECTED!"
	// Instruct user to run multiple times
	if(expected_total != actual_total) {
		printf("RACE CONDITION DETECTED!\n");
		printf("Run the program multiple times to observe.\n");
	}
	return 0;
}
