#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

typedef struct{
        char command[1024];
        pid_t pid;
        unsigned int tslices_on_cpu;
        unsigned int tslices_off_cpu;
        int state;
        //state is 0: ready 1: running 2: finished
} Process;

typedef struct{
        Process table[256];
        int count;
        sem_t* sem;
} SharedProcessTable;

SharedProcessTable* processtable;
int shm_fd;
//how the queue will work
//maintain like a pointer to start of queue & the current pointer
//current pointer moves to give the next ncpu processes with state ready
//if ncpu not found till end of table, it loops back to the start using the start pointer
//the first time u encounter an already running process - means u have seen all the processes in the table - so stop now

//signal handler required, which handles SIGINT: will wait for the processes to finish, display information & then exit
static void my_handler(int signal){
}

void setup(){
	shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
	processtable = mmap(NULL, sizeof(SharedProcessTable), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (processtable == MAP_FAILED){
		perror("mmap failed");
		exit(1);
	}
}

void cleanup(){
	munmap(processtable,sizeof(SharedProcessTable));
	close(shm_fd);
}

int main(int argc, char** argv){
	//sigint signal handler
	struct sigaction sig;
	memset(&sig,0,sizeof(sig));
	sig.sa_handler = my_handler;
	sigaction(SIGINT,&sig,NULL);

	//access the shared memory which has the process table
	setup();
	while (1){
		//use semaphore, read the process table to find the next ncpu processes to run as per round robin policy, send SIGCONT to these for running & set state = 1 (running) in table
		//nano sleep for tslice miliseconds
		//use semaphore, read the ncpu running processes, send SIGSTOP to pause, set state to ready
	}
	cleanup();
	return 0;
}
