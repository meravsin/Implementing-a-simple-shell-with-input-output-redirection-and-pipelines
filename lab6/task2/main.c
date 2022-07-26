#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "LineParser.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>


#define MAX_INPUT_SIZE 2048

#define STDIN 0
#define STDOUT 1

void execute(cmdLine* pCmdLine);
int debug = 0;
int isPipe = 0;
int p[2];
char buf;

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

        if (cmd->next != NULL){
            isPipe = 1;
            pipe(p);
        }
        execute(cmd);
        freeCmdLines(cmd);
    }
    return 0;
}

int specialProcs(cmdLine* pCmdLine)
{
    char* lib;
    int isSpecial;

    if(strcmp(pCmdLine->arguments[0],"cd")==0)
    {
        lib =pCmdLine->arguments[1];
        if(chdir(lib) < 0)
            perror("Failed to change directory: ");
        freeCmdLines(pCmdLine);
        isSpecial = 1;
    }

    return isSpecial;
}


void execute(cmdLine* pCmdLine)
{
    int pid1, pid2, return_val, fdOutput, fdInput, fd_writeCopy, fd_readCopy;
    char* lib;

    if (!specialProcs(pCmdLine))
    {
        pid1 = fork();
        if (pid1==0) 
        {
            /* added */
            fdOutput = open(pCmdLine->outputRedirect, O_RDWR, 0600);
            fdInput = open(pCmdLine->inputRedirect, O_RDWR, 0600);
            dup2(fdOutput, STDOUT);
            dup2(fdInput, STDIN);
            close(fdInput);
            close(fdOutput);

            if (isPipe)
            {
                close(STDOUT);
                fd_writeCopy = dup(p[1]);
                close(p[1]);
            }

            return_val = execvp(pCmdLine->arguments[0], pCmdLine->arguments);
            if (return_val == -1) 
            {
                perror("execv failed");
                freeCmdLines(pCmdLine);
                exit(1);
            }
            else if (pid1 == -1) 
            {
                perror("fork failed");
                exit(1);
            }
        }

        else
        {
            if (isPipe)
            {
                close(p[1]); 
                waitpid(pid1, NULL, 0);
                pid2 = fork(); /* child 2 */
                if (pid2 == 0)
                {
                    close(STDIN);
                    fd_readCopy = dup(p[0]);
                    close(p[0]);
                    execvp(pCmdLine->next->arguments[0], pCmdLine->next->arguments);
                    exit(1);
                }
                close(p[0]);
                waitpid(pid2, NULL, 0);
            }

            if(pCmdLine->blocking == 1)  
                if(waitpid(pid1, NULL, 0) == -1)
                {
                    perror("waitpid error ");
                    exit(1);
                }

            if (debug==1) 
                fprintf(stderr, "PID: %d, Executing command: %s\n", pid1, pCmdLine->arguments[0]);
        }
    }
}
   