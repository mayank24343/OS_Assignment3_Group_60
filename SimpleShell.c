#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <signal.h>

typedef struct{
	char command[1024];
	pid_t* pid;
	unsigned int tslices_on_cpu;
	unsigned int tslices_off_cpu;
	int state;
	//state is 0: ready 1: running 2: finished
} Process;

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

void submit(char* command){
	char** args = read_command(command);//read arguments
	if (strcmp(args[0],"submit") == 0){
		printf("%s",args[1]);
		//executable submitted
		//fork to create child process & execute - execution pauses due to dummy main calling raise(SIGSTOP)
		//parent adds process to process table in shared memory using semaphores
	}
	else{
		//shell need not handle other commands for this assignment
		return;
	}
}

void shell_loop(){
	while (1) {
		printf("group60@os:~$");
		char* command = (char*)malloc(1024*sizeof(char)); //1KB input
		fgets(command,1024,stdin);
		if (strcmp(command,"exit\n") == 0){
			//terminate scheduler by sending sigint (scheduler has a sigint handler to wait for each process to end, display details & then exit)
			//exit the shell
			break;
		}
		else{
			submit(command);//submit command for execution
		}
		free(command); //free memory for cleanup
		//traverse through the process table, if any child has ended - put process state as finished, else move on dont block (waitpid with WNOHANG)
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
	shell_loop();
	return 0;
}
