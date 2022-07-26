#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>


void sig_handler(int signum); 


int main(int argc, char **argv){ 

	printf("Starting the program\n");
	signal(SIGINT, sig_handler);
	signal(SIGTSTP, sig_handler);
	signal(SIGCONT, sig_handler);
	while(1) {
		sleep(2);
	}

	return 0;
}

void sig_handler(int signum) 
{
	char* signame = strsignal(signum);
	printf("\nreceived signal: %s\n", signame);

	 if(signum == SIGTSTP){
        signal(SIGCONT,sig_handler);
    }

    if(signum == SIGCONT){
        signal(SIGTSTP,sig_handler);
    }

	signal(signum, SIG_DFL); 
	raise(signum);
}