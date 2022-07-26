#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "LineParser.h"

#define MAX_INPUT_SIZE 2048
#define TERMINATED -1
#define RUNNING 1
#define SUSPENDED 0


typedef struct process{
    cmdLine* cmd; 
    pid_t pid; 
    int status; 
    struct process *next;
} process;

process* process_list;
int debug = 0;
char* strStatus(int status);
void updateProcessList(process **process_list);
void deleteProcess(process **process_list, int pid);
void updateProcessStatus(process* process_list, int pid, int status);
void execute(cmdLine* pCmdLine);
void deleteSingleProc(process* toDelete);

char* strStatus(int status)
{
    switch(status)
    {
        default:
            return "";
        case RUNNING:
            return "Running";
        case SUSPENDED:
            return "Suspended";
        case TERMINATED:
            return "Terminated";
    }
}

void addProcess(process** process_list, cmdLine* cmd, pid_t pid)
{
    process *temp = *process_list;
    if (temp != NULL)
    {
        while (temp->next != NULL)
        {
            temp = temp->next;
        }
    }
    process* toAdd;
    toAdd=(process*)calloc(sizeof(process),1);
    toAdd->cmd = cmd;
    toAdd->pid = pid;
    toAdd->status = RUNNING;
    toAdd->next = NULL;
    if (temp != NULL)
        temp->next = toAdd;
    else
        *process_list = toAdd;
}

void printProcessList(process** process_list)
{
    updateProcessList(process_list);
    process *temp = *process_list;
    puts("PID       Command     Status");
    while(temp != NULL)
    {
		printf("%d      %s      %s\n",temp->pid,temp->cmd->arguments[0],strStatus(temp->status));
        temp = temp->next;
    }

    process* curProce = *process_list;
    while(curProce != NULL)
    {
        if (curProce->status == TERMINATED)
            deleteProcess(process_list, curProce->pid);
        curProce = curProce->next;
    }
    
}


void updateProcessList(process **process_list)
{
    process* curr_process = *process_list;
    
    while(curr_process != NULL){
        
        int status;
        pid_t pid;
        pid = waitpid(curr_process->pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        
        if(pid != 0)
            updateProcessStatus(curr_process, curr_process->pid ,status);
        
        curr_process=curr_process->next;

    }
}

void freeProcessList(process* process_list){
    
    process* curr_process = process_list;
    
    if(curr_process->next != NULL)
    {
        freeProcessList(curr_process->next);
        deleteSingleProc(curr_process);
    }
    process_list = NULL;
    
}

void updateProcessStatus(process* process_list, int pid, int status)
{
    int new_status = RUNNING;
    
    if(WIFEXITED(status) || WIFSIGNALED(status))
        new_status = TERMINATED;
    
    else if(WIFSTOPPED(status))
        new_status = SUSPENDED;
    
    else if(WIFCONTINUED(status))
        new_status = RUNNING;
    
    process_list->status = new_status;
}

void deleteSingleProc(process* toDelete)
{
    freeCmdLines(toDelete->cmd);
    toDelete->cmd = NULL;
    free(toDelete);
    toDelete = NULL;
}


void deleteProcess(process **process_list, int pid)
{
    process *temp = *process_list;

    if (temp->pid == pid)  /* we need to delete the first process */
    {
        *process_list = temp->next;
        deleteSingleProc(temp);
    }


    else    
    {
        while (temp != NULL)
        {
            if (temp->next->pid == pid)
            {
                temp->next = temp->next->next;
                deleteSingleProc(temp->next);
                break;
            }
            else
                temp = temp->next;
        }
    }
}

int main(int argc,char** argv)
{
    char pathBuffer[PATH_MAX]; 
    char input[MAX_INPUT_SIZE];
    int i;

    for(i=1; i<argc;i++) 
    {
		if(strncmp(argv[i],"-d",2) == 0)  
			debug = 1;
	}
   while(1){
        getcwd(pathBuffer,PATH_MAX);
        printf("%s ",pathBuffer);
        fgets(input,MAX_INPUT_SIZE,stdin);
        cmdLine* cmd = parseCmdLines(input);
        if(strcmp(cmd->arguments[0],"quit") == 0)
        {
            freeCmdLines(cmd);
            exit(0);
        }
        execute(cmd);
    }

    return 0;
}

int specialProcs(cmdLine* pCmdLine)
{
    int special = 0;
    char* lib;

    if(strcmp(pCmdLine->arguments[0],"cd")==0)
    {
        special = 1;
        lib =pCmdLine->arguments[1];
        if(chdir(lib) < 0)
            perror("Failed to change directory: ");
        freeCmdLines(pCmdLine);
    }

    else if (strcmp(pCmdLine->arguments[0],"showprocs")==0)
    {
        special = 1;
        process** pplist = &process_list;
        printProcessList(pplist);
        freeCmdLines(pCmdLine);
    }

    else if (strncmp(pCmdLine->arguments[0],"stop",strlen("stop"))==0)
    {
        special = 1;
        kill(atoi(pCmdLine->arguments[1]), SIGINT);
        freeCmdLines(pCmdLine);
    }

    else if (strncmp(pCmdLine->arguments[0],"nap",strlen("nap"))==0)
    {
        special = 1;
        int p_id;
        int sleepTime = atoi(pCmdLine->arguments[1]);
        int id = atoi(pCmdLine->arguments[2]);
        p_id = fork();
        freeCmdLines(pCmdLine);
        if(p_id==0)
        {
            kill(id, SIGTSTP);
            sleep(sleepTime);
            kill(id, SIGCONT); 
        }
    }

    return special;
}


void execute(cmdLine* pCmdLine)
{
    int pid, return_val;
    char* lib;

    if (!specialProcs(pCmdLine))
    {

        pid=fork();
        if (pid==0) 
        {
            return_val = execvp(pCmdLine->arguments[0], pCmdLine->arguments);
            if (return_val == -1) 
            {
                perror("execv failed");
                exit(1);
            }
            else if (pid == -1) 
            {
                perror("fork failed");
                exit(1);
            }
        }


        if(pCmdLine->blocking == 1)  
        {
            if(waitpid(pid, NULL, 0) == -1) 
            {
                perror("waitpid error ");
                exit(1);
            }
        }
        
        if (pid != 0 && return_val !=-1)
            addProcess(&process_list, pCmdLine, pid);

        if (debug==1) 
            fprintf(stderr, "PID: %d, Executing command: %s\n", pid, pCmdLine->arguments[0]);
    }

}