#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

//how the queue will work
//maintain like a pointer to start of queue & the current pointer
//current pointer moves to give the next ncpu processes with state ready
//if ncpu not found till end of table, it loops back to the start using the start pointer
//the first time u encounter an already running process - means u have seen all the processes in the table - so stop now
//signal handler required, which handles SIGINT: will wait for the processes to finish, display information & then exit

int main(int argc, char** argv){
	//sigint signal handler
	//access the shared memory which has the process table
	while (1){
		//use semaphore, read the process table to find the next ncpu processes to run as per round robin policy, send SIGCONT to these for running & set state = 1 (running) in table
		//nano sleep for tslice miliseconds
		//use semaphore, read the ncpu running processes, send SIGSTOP to pause, set state to ready
	}
	return 0;
}
