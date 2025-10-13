#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

#define SHM_NAME "/processtable_shm"

typedef struct {
    char command[1024];
    pid_t pid;
    unsigned int tslices_on_cpu;
    unsigned int tslices_off_cpu;
    int state;
    //state is 0: ready 1: running 2: finished
} Process;

typedef struct {
    Process table[256];
    int count;
    sem_t sem;
} SharedProcessTable;

SharedProcessTable* processtable;
int shm_fd;
int ncpu, tslice;
int running = 0;
volatile sig_atomic_t stop_scheduler = 0; //flag to stop the scheduler loop

//signal handler for SIGINT.
static void my_handler(int signal) {
    if (signal == SIGINT) {
        printf("\nScheduler received SIGINT - Completing remaining processses & exiting\n");
	fflush(stdout);
        stop_scheduler = 1;
    }
}

//map the existing shared memory segment into the scheduler's address space.
void setup() {
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(1);
    }

    processtable = mmap(NULL, sizeof(SharedProcessTable), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (processtable == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
}

//clean up resources
void cleanup() {
    munmap(processtable, sizeof(SharedProcessTable));
    close(shm_fd);
}

int main(int argc, char** argv) {
    if (argc!=3) {
        fprintf(stderr, "Usage: %s <ncpu> <tslice>\n", argv[0]);
        exit(1);
    }

    ncpu = atoi(argv[1]);
    tslice = atoi(argv[2]);

    if (ncpu < 1 || tslice < 1) {
        fprintf(stderr, "ncpu and tslice must be positive integers.\n");
        exit(1);
    }

    //sigint signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = my_handler;
    sigaction(SIGINT, &sa, NULL);

    setup();

    int current_idx = 0;
    //int running_processes = 0;

    //main scheduler loop.
    while (1) {
        sem_wait(&processtable->sem);

        running = 0;

        //schedule new processes if there are available cpus
        int scheduled_count = 0;
        for (int i = 0; i < processtable->count && running < ncpu; i++) {
            int check_idx = (current_idx + i) % processtable->count;
		//check if process has endeddd
            if (processtable->table[check_idx].state == 0) {
                kill(processtable->table[check_idx].pid, SIGCONT);
                processtable->table[check_idx].state = 1;
                running++;
                scheduled_count++;
            }
        }
        current_idx = (current_idx + scheduled_count) % (processtable->count > 0 ? processtable->count : 1);

         //update time slices off cpu for ready processes.
        for (int i = 0; i < processtable->count; i++) {
            if (processtable->table[i].state == 0) { //ready
                processtable->table[i].tslices_off_cpu++;
            }
        }

        sem_post(&processtable->sem);
        //sleep for the duration of a time slice
        usleep(tslice*1000); //usleep takes microseconds

        sem_wait(&processtable->sem);
        //stop currently running processes
        for (int i = 0; i < processtable->count; i++) {
		//check if finished process
            if (processtable->table[i].state == 1) {
                kill(processtable->table[i].pid, SIGSTOP);
                processtable->table[i].state = 0;
		processtable->table[i].tslices_on_cpu++;
            }
        }

        //check if all processes have finished
        int finished_count = 0;
        for (int i = 0; i < processtable->count; i++) {
            if (processtable->table[i].state == 2) {
                finished_count++;
            }
        }
        if (finished_count == processtable->count && processtable->count > 0 && stop_scheduler) {
            break; //exit loop if all submitted processes are done
        }

        sem_post(&processtable->sem);
    }



    //final cleanup
    cleanup();

    return 0;
}
