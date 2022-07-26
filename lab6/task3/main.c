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


int ** createPipes(int nPipes){
    int** pipes;
    pipes=(int**) calloc(nPipes, sizeof(int));

    for (int i=0; i<nPipes;i++){
        pipes[i]=(int*) calloc(2, sizeof(int));
        pipe(pipes[i]);
    }  
    return pipes;

    }
void releasePipes(int **pipes, int nPipes){
        for (int i=0; i<nPipes;i++){
            free(pipes[i]);
        
        }  
    free(pipes);
}
int *leftPipe(int **pipes, cmdLine *pCmdLine){
    if (pCmdLine->idx == 0) return NULL;
    return pipes[pCmdLine->idx -1];
}

int *rightPipe(int **pipes, cmdLine *pCmdLine){
    if (pCmdLine->next == NULL) return NULL;
    return pipes[pCmdLine->idx];
}

void execute(cmdLine* pCmdLine);
void runPipedpCmdLines(cmdLine* pCmdLine);
int debug = 0;
int isPipe = 0;
int p[2];
char buf;

int main(int argc,char** argv){
    char pathBuffer[PATH_MAX]; 
    char input[MAX_INPUT_SIZE];
    char last[MAX_INPUT_SIZE];
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
        if (strcmp(cmd->arguments[0],"prtrls") == 0){
            fprintf(stderr, "%s", last);
        }
        strncpy(last, input, sizeof(input));
        if (cmd->next != NULL){
            isPipe = 1;
        }
        runPipedpCmdLines(cmd);
    }
    freeCmdLines(cmd);
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

void Redirect(cmdLine* pCmdLine)
{
    int fdOutput, fdInput;
    fdOutput = open(pCmdLine->outputRedirect, O_RDWR, 0600);
    fdInput = open(pCmdLine->inputRedirect, O_RDWR, 0600);
    dup2(fdOutput, STDOUT);
    dup2(fdInput, STDIN);
    close(fdInput);
    close(fdOutput);
}

void execute(cmdLine* pCmdLine)
{
    int pid1, pid2, return_val, fd_writeCopy, fd_readCopy;
    char* lib;

    if (!specialProcs(pCmdLine))
    {
        
        if (pCmdLine->next != NULL)
             runPipedpCmdLines(pCmdLine);
        else
        {
            pid1 = fork();
            if (pid1==0) 
            {
                Redirect(pCmdLine);
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
                if(pCmdLine->blocking == 1)  
                    if(waitpid(pid1, NULL, 0) == -1)
                    {
                        perror("waitpid error ");
                        exit(1);
                    }

                if (debug==1) 
                    fprintf(stderr, "PID: %d, Executing pCmdLine: %s\n", pid1, pCmdLine->arguments[0]);
            }
        }
    }
}

int numberPipes(cmdLine *pCmdLine)
{
    cmdLine *temp = pCmdLine;
    int cmd_count = 0;
    while (temp != NULL)
    {
        cmd_count++;
        temp = temp->next;
    }
    return cmd_count;
}
   

void runPipedpCmdLines(cmdLine* pCmdLine) 
{
    int cmd_count = numberPipes(pCmdLine);
    int nPipes = cmd_count - 1;
    int **pipes = createPipes(nPipes);
    pid_t *children_pid = (pid_t *)malloc(sizeof(pid_t **) * cmd_count);
    cmdLine *curr_cmd = pCmdLine;

    for (int i = 0; i < cmd_count; i++)
    {
        int *lpipe = leftPipe(pipes, curr_cmd);
        int *rpipe = rightPipe(pipes, curr_cmd);
        int child_pid = children_pid[i] = fork();
        if (child_pid == -1)
        {
            perror("fork failed");
            free(children_pid);
            releasePipes(pipes, nPipes);
            exit(0);
        }
        else if (child_pid == 0)
        {
            if (lpipe) 
            {
                dup2(lpipe[0], 0);
                close(lpipe[0]);
            }
            if (rpipe) 
            {
                dup2(rpipe[1], 1);
                close(rpipe[1]);
            }
            Redirect(curr_cmd);
            execvp(curr_cmd->arguments[0], curr_cmd->arguments);
            free(children_pid);
            releasePipes(pipes, nPipes);
            _exit(1);
        }
        if (rpipe)
            close(rpipe[1]);
        if (lpipe)
            close(lpipe[0]);
        curr_cmd = curr_cmd->next;
    }
    for (int child = 0; child < cmd_count; child++) 
        waitpid(children_pid[child], NULL, 0);
    free(children_pid);
    releasePipes(pipes, nPipes);
}