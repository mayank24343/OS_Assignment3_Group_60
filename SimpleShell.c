#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <fcntl.h>

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

char** read_command(char* command){
	//take string command and return tokens (space separated words)
	char** args = malloc(sizeof(char*)*64);
	if (args == NULL){
		perror("Could not allocate memory for args");
		exit(0);
	}
	int i = 0;

	char* token = strtok(command," \n");
	while (token != NULL && i < 63){
		args[i] = token;
		i++;
		token = strtok(NULL," \n");
	}
	args[i] = NULL;//execvp takes NULL terminated

	return args;
}

static void my_handler(int signal){
	int status;
	pid_t pid;
	while ((pid = waitpid(-1,&status,WNOHANG)) > 0){
		//wait to get the pid of terminated child without blocking (WNOHANG)
		sem_wait(processtable->sem);//using shared memory
		for(int i = 0; i < processtable->count; i++){
			if (processtable->table[i].pid == pid){
				processtable->table[i].state = 2; //finishde
			}
		}
		sem_post(processtable->sem);
	}
}

void submit(char* command){
	char** args = read_command(command);//read arguments
	if (strcmp(args[0],"submit") == 0){
		printf("%s",args[1]);
		//executable submitted
		//fork to create child process & execute - execution pauses due to dummy main calling raise(SIGSTOP)
		pid_t child = fork();
		if (child == 0){
			//in child process
			execvp(args[1],++args);//++args to skip the submit word
			perror("exec failed");
			exit(1);
		}
		else if (child > 0){
			//in parent
			sem_wait(processtable->sem);//modifying in shm
			if (processtable->count < 256){
				Process* p = &(ptable->table[count]);
				p->pid = child;
				p->tslices_on_cpu = 0;
				p->tslices_off_cpu = 0;
				p->state = 0; //running
				strcpy(p->command,args[1]);
			//parent adds process to process table in shared memory using semaphores
			}
			else{
				printf("Process table full, cannot execute any more processes!!");
			}
			sem_post(processtable->sem);
	}
	else{
		//shell need not handle other commands for this assignment
		return;
	}
}

void display_info(){
	//to display process name, pid, wait time = no of tslices off cpu * tslice, completion time = tslices_on_cpu+tslices_off_cpu*tslice
}

pid_t scheduler_pid;
int shm_fd;

void setup_shm_sem(){
	//shm setup
	shm_fd = shm_open("/processtable", O_CREAT | O_RDWR, 0666);
	if (shm_fd < 0){
		perror("shm_open failed");
		exit(1);
	}
	ftruncate(shm_fd, sizeof(SharedProcessTable);
	processtable = mmap(NULL, sizeof(SharedProcessTable), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (processtable == MAP_FAILED){
		perror("mmap failed");
		exit(1);
	}
	processtable->count = 0;

	//semaphore setup
	sem_init(processtable->sem, 1, 1);
}

void cleanup(){
	//cleanup semaphore, shared memry
	sem_destroy(processtable->sem);
	munmap(processtable, sizeof(SharedProcessTable));
	close(shm_fd);
	shm_unlink("/processtable");
}

void shell_loop(){
	while (1) {
		printf("group60@os:~$");
		char* command = (char*)malloc(1024*sizeof(char)); //1KB input
		fgets(command,1024,stdin);
		if (strcmp(command,"exit\n") == 0){
			//terminate scheduler by sending sigint (scheduler has a sigint handler to wait for each process to end, display details & then exit)
			kill(scheduler_pid, SIGINT);
			waitpid(scheduler_pid,NULL,0);
			display_info();
			//exit the shell
			break;
		}
		else{
			submit(command);//submit command for execution
		}
		free(command); //free memory for cleanup
	}
}

int main(int argc,char** argv){
	if (argc != 3){
		perror("Usage: ./SimpleShell ncpu tslice");
		exit(1);
	}
	int ncpu, tslice;
	ncpu = atoi(argv[1]);
	tslice = atoi(argv[2]);
	if (ncpu < 1 || tslice < 1){
		perror("Non-positive ncpu or tslice");
		exit(1);
	}
	//initialise a shared memory for storing the process table (linked list) & sempaphore for mutual exclusion while modifying the process table
	setup_shm_sem();
	struct sigaction sig;
	memset(&sig,0,sizeof(sig));
	sig.sa_handler = my_handler;
	sigaction(SIGCHLD,&sig,NULL);
	//also need a signal handler to handle SIGCHLD by the submitted process when they end, need to update process table & mark them as finished

	//process for shceduler
	scheduler_pid = fork();
	if (scheduler_pid == 0){
		//execute scheduler here using exec family
		perror("exec failed");
		exit(1);
	}
	else if (scheduler_pid < 0){
		perror("fork failed");
		exit(1);
	}
	shell_loop();
	cleanup();
	return 0;
}
