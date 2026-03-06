# Project 1 Multi-Threaded Banking System
## Phase 1 - Basic Thread Operations
- Threads perform transactions
- Tests and observe race conditions
- Calculate and display the expeced vs actual balances
- Run multiple times to show different results
## Build instructions- Compile
''gcc -Wall -pthread phase1.c -o phase1''
## Run multiple times to see different results
''./phase1<br />
./phase1<br />
./phase1''

## Phase 2 - Resource Protection 
- Applying mutex locks to protect shared data
- Measure execution time with/without locks
- Verify the final balances are as expected

## Build instructions- Compile
''gcc -Wall -pthread phase2.c -o phase2''
## Run multiple times to see different results
''./phase2<br />
./phase2<br />
./phase2''

## Phase 3 - Deadlock Creation
- Reliably create deadlock scenario
- Implement deadlock detection
- Document Coffman conditions

## Build instructions- Compile
''gcc -Wall -pthread phase3.c -o phase3''

## Run to see the deadlock
''./phase3''

## Phase 4 
- Implemented lock ordering
- Implemented trylock with backoff
- Test and compare the performance times

## Build instructions- Compile
''gcc -Wall -pthread phase4.c -o phase4''

## Run multiple times to see different results
''./phase4<br />
./phase4<br />
./phase4''



