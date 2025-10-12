#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>

#define SHM_NAME "/processtable"

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
    sem_t sem; 
} SharedProcessTable;

SharedProcessTable* processtable;
int tslice;
pid_t scheduler_pid;
int shm_fd;

int length(int n) {
    if (n == 0) {
        return 1;
    }
    int l = 0;
    n = abs(n); 
    while (n > 0) {
        n /= 10; // 
        l++;
    }
    return l;
}


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
        sem_wait(&processtable->sem);//using shared memory
        for(int i = 0; i < processtable->count; i++){
            if (processtable->table[i].pid == pid){
                processtable->table[i].state = 2; //finished
            }
        }
        sem_post(&processtable->sem);
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
			sem_wait(&processtable->sem);//modifying in shm
			if (processtable->count < 256){
				Process* p = &(processtable->table[processtable->count]);
				p->pid = child;
				p->tslices_on_cpu = 0;
				p->tslices_off_cpu = 0;
				p->state = 0; //running
				strcpy(p->command,args[1]);
				processtable->count++;
			//parent adds process to process table in shared memory using semaphores
			}
			else{
				printf("Process table full, cannot execute any more processes!!");
			}
			sem_post(&processtable->sem);
		}
	}
	else{
		//shell need not handle other commands for this assignment
		return;
	}
}


void display_info(){
	int c=length(processtable->count);
	if (c>3){
		printf("%*s", c-3);
	}
	printf("PID   Wait time(ms)   Completion Time(ms)   Command\n");
	for (int i = 0; i < processtable->count; i++){
		Process p=processtable->table[i];
		if (p.state==2){//display only for finished processes
			int wait_time = p.tslices_off_cpu * tslice;
            int completion_time = (p.tslices_on_cpu + p.tslices_off_cpu) * tslice;
			int w = length(wait_time);
			int ct = length(completion_time);
			int l=length(p.pid);
			if (l<3 && c<3){
				printf("%*s",3-l," ");
			}
			else if (c>3){
				printf("%*s",c-l," ");
			}
			printf("%d", p.pid);
			if (w<=13){
				printf("%*s",13-w, " ");
			}
			printf("   %d", wait_time);
			if (ct<=19){
				printf("%*s", 19-ct, " ");
			}
			printf("   %d", completion_time);
			printf("   %s\n", p.command);
		}
	}
}

void setup_shm_sem(){
    //shm setup
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0){
        perror("shm_open failed");
        exit(1);
    }
    ftruncate(shm_fd, sizeof(SharedProcessTable));
    processtable = mmap(NULL, sizeof(SharedProcessTable), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (processtable == MAP_FAILED){
        perror("mmap failed");
        exit(1);
    }
    processtable->count = 0;

    //semaphore setup
    sem_init(&processtable->sem, 1, 1);
}


void cleanup(){
	//cleanup semaphore, shared memory
    sem_destroy(&processtable->sem);
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
        fprintf(stderr, "Usage: ./SimpleShell ncpu tslice\n");
        exit(1);
    }
    int ncpu;
    ncpu = atoi(argv[1]);
    tslice = atoi(argv[2]);
    if (ncpu < 1 || tslice < 1){
        fprintf(stderr, "Non-positive ncpu or tslice\n");
        exit(1);
    }
    
    setup_shm_sem();
    
    //signal handler for SIGCHLD
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = my_handler;
    sigaction(SIGCHLD, &sa, NULL);

    //process for scheduler
    scheduler_pid = fork();
    if (scheduler_pid == 0){
        //execute the scheduler program using execvp
        char ncpu_str[10], tslice_str[10];
        sprintf(ncpu_str, "%d", ncpu);
        sprintf(tslice_str, "%d", tslice);
        
        char* scheduler_args[] = {"./SimpleScheduler", ncpu_str, tslice_str, NULL};
        execvp(scheduler_args[0], scheduler_args);
        
        //only runs if execvp fails
        perror("execvp for scheduler failed");
        exit(1);
    }
    else if (scheduler_pid < 0){
        perror("fork for scheduler failed");
        exit(1);
    }

    shell_loop();
    
    cleanup();
    
    return 0;
}
