#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "LineParser.h"
#include <sys/wait.h>


#define MAX_INPUT_SIZE 2048

void execute(cmdLine* pCmdLine);
int debug = 0;

int main(int argc,char** argv){
    char pathBuffer[PATH_MAX]; 
    char input[MAX_INPUT_SIZE];
    int i;

    cmdLine* cmd;

    for(i=1; i<argc;i++) 
    {
		if(strncmp(argv[i],"-d",2) == 0)  
			debug = 1;
	}
   while(1){
        getcwd(pathBuffer,PATH_MAX);
        printf("%s ",pathBuffer);
        fgets(input,MAX_INPUT_SIZE,stdin);
        cmd = parseCmdLines(input);
        if(strcmp(cmd->arguments[0],"quit") == 0)
        {
            freeCmdLines(cmd);
            exit(0);
        }
        execute(cmd);
        freeCmdLines(cmd);
    }

    return 0;
}

void execute(cmdLine* pCmdLine)
{
    int pid, return_val;
    char* lib;

    if(strcmp(pCmdLine->arguments[0],"cd")==0)
    {
        lib =pCmdLine->arguments[1];
        if(chdir(lib) < 0)
            perror("Failed to change directory: ");
    }

    else
    {
        pid=fork();
        if (pid==0) 
        {
            return_val = execvp(pCmdLine->arguments[0], pCmdLine->arguments);
            if (return_val == -1) 
            {
                perror("execv failed");
                freeCmdLines(pCmdLine);
                exit(1);
            }
            else if (pid == -1) 
            {
                perror("fork failed");
                exit(1);
            }
        }

        if(pCmdLine->blocking == 1)  
            if(waitpid(pid, NULL, 0) == -1) {
                perror("waitpid error ");
                exit(1);
            }
        if (debug==1) 
            fprintf(stderr, "PID: %d, Executing command: %s\n", pid, pCmdLine->arguments[0]);
    }

}