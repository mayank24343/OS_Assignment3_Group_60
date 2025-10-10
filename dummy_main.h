#include <signal.h>
#include <unistd.h>

int dummy_main(int argc, char** argv);
int main(int argc, char** argv){
	raise(SIGSTOP);//a process should stop by default
	int ret = dummy_main(argc,argv);
	return ret;
}
#define main dummy_main
