#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#define LIMIT 256
#define MAXLINE 1024

#define TRUE 1
#define FALSE !TRUE

// Shell pid, pgid, terminal modes
static pid_t GBSH_PID;
static pid_t GBSH_PGID;
static int GBSH_IS_INTERACTIVE;
static struct termios GBSH_TMODES;

static char *currentDirectory;
extern char **environ;

struct sigaction act_child;
struct sigaction act_int;
struct sigaction act_quit;
struct sigaction act_eof;

int no_reprint_prmpt;

pid_t pid;

char *history[LIMIT];
int numCommands = 0;

void signalHandler_child(int p);
void signalHandler_int(int p);

void shellInit()
{
    GBSH_PID = getpid();

    GBSH_IS_INTERACTIVE = isatty(STDIN_FILENO);

    if (GBSH_IS_INTERACTIVE)
    {
        while (tcgetpgrp(STDIN_FILENO) != (GBSH_PGID = getpgrp()))
            kill(GBSH_PID, SIGTTIN);

        act_child.sa_handler = signalHandler_child;
        act_int.sa_handler = signalHandler_int;

        sigaction(SIGCHLD, &act_child, 0);
        sigaction(SIGINT, &act_int, 0);

        setpgid(GBSH_PID, GBSH_PID);
        GBSH_PGID = getpgrp();
        if (GBSH_PID != GBSH_PGID)
        {
            printf("Error, the shell is not process group leader");
            exit(EXIT_FAILURE);
        }

        tcsetpgrp(STDIN_FILENO, GBSH_PGID);

        tcgetattr(STDIN_FILENO, &GBSH_TMODES);

        currentDirectory = (char *)calloc(1024, sizeof(char));
    }
    else
    {
        printf("Could not make the shell interactive.\n");
        exit(EXIT_FAILURE);
    }
}

void signalHandler_child(int p)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
    {
    }
    printf("\n");
}

void signalHandler_int(int p)
{
    if (kill(pid, SIGTERM) == 0)
    {
        printf("\nProcess %d received a SIGINT signal\n", pid);
        no_reprint_prmpt = 1;
        exit(EXIT_SUCCESS);
    }
    else
    {
        printf("\n");
    }
}

void fileIOHandler(char *args[], char *inputFile, char *outputFile, int option)
{
    int err = -1;

    int fileDescriptor;

    if ((pid = fork()) == -1)
    {
        printf("Child process could not be created\n");
        return;
    }
    if (pid == 0)
    {
        if (option == 0)
        {
            fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
            dup2(fileDescriptor, STDOUT_FILENO);
            close(fileDescriptor);
        }
        else if (option == 1)
        {
            fileDescriptor = open(inputFile, O_RDONLY, 0600);
            dup2(fileDescriptor, STDIN_FILENO);
            close(fileDescriptor);
            fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
            dup2(fileDescriptor, STDOUT_FILENO);
            close(fileDescriptor);
        }

        setenv("parent", getcwd(currentDirectory, 1024), 1);

        if (execvp(args[0], args) == err)
        {
            printf("err");
            kill(getpid(), SIGTERM);
        }
    }
    waitpid(pid, NULL, 0);
}

void pipeHandler(char *args[])
{
    int filedes[2];
    int filedes2[2];

    int num_cmds = 0;

    char *command[256];

    pid_t pid;

    int err = -1;
    int end = 0;

    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;

    while (args[l] != NULL)
    {
        if (strcmp(args[l], "|") == 0)
        {
            num_cmds++;
        }
        l++;
    }
    num_cmds++;

    while (args[j] != NULL && end != 1)
    {
        k = 0;
        while (strcmp(args[j], "|") != 0)
        {
            command[k] = args[j];
            j++;
            if (args[j] == NULL)
            {
                end = 1;
                k++;
                break;
            }
            k++;
        }
        command[k] = NULL;
        j++;

        if (i % 2 != 0)
        {
            pipe(filedes);
        }
        else
        {
            pipe(filedes2);
        }

        pid = fork();

        if (pid == -1)
        {
            if (i != num_cmds - 1)
            {
                if (i % 2 != 0)
                {
                    close(filedes[1]);
                }
                else
                {
                    close(filedes2[1]);
                }
            }
            printf("Child process could not be created\n");
            return;
        }
        if (pid == 0)
        {
            if (i == 0)
            {
                dup2(filedes2[1], STDOUT_FILENO);
            }
            else if (i == num_cmds - 1)
            {
                if (num_cmds % 2 != 0)
                {
                    dup2(filedes[0], STDIN_FILENO);
                }
                else
                {
                    dup2(filedes2[0], STDIN_FILENO);
                }
            }
            else
            {
                if (i % 2 != 0)
                {
                    dup2(filedes2[0], STDIN_FILENO);
                    dup2(filedes[1], STDOUT_FILENO);
                }
                else
                {
                    dup2(filedes[0], STDIN_FILENO);
                    dup2(filedes2[1], STDOUT_FILENO);
                }
            }

            if (execvp(command[0], command) == err)
            {
                kill(getpid(), SIGTERM);
            }
        }

        if (i == 0)
        {
            close(filedes2[1]);
        }
        else if (i == num_cmds - 1)
        {
            if (num_cmds % 2 != 0)
            {
                close(filedes[0]);
            }
            else
            {
                close(filedes2[0]);
            }
        }
        else
        {
            if (i % 2 != 0)
            {
                close(filedes2[0]);
                close(filedes[1]);
            }
            else
            {
                close(filedes[0]);
                close(filedes2[1]);
            }
        }
        waitpid(pid, NULL, 0);
        i++;
    }
}

void executeCommand(char **args, int background)
{
    int err = -1;

    if ((pid = fork()) == -1)
    {
        printf("Child process could not be created\n");
        return;
    }

    if (pid == 0)
    {

        signal(SIGINT, SIG_IGN);

        setenv("parent", getcwd(currentDirectory, 1024), 1);

        if (execvp(args[0], args) == err)
        {
            printf("Command not found");
            kill(getpid(), SIGTERM);
        }
    }

    if (background == 0)
    {
        waitpid(pid, NULL, 0);
    }
    else
    {
        printf("Process created with PID: %d\n", pid);
    }
}

void shellPrompt()
{
    char hostn[1204] = "";
    gethostname(hostn, sizeof(hostn));
    printf("%s@%s> %s>\n$> ", getenv("LOGNAME"), hostn, getcwd(currentDirectory, 1024));
}

void displayIntro()
{
    printf("\n====================================================\n");
    printf("|            🐚 Welcome to C Shell 🐚              |\n");
    printf("|🐚 Source: https://github.com/2kabhishek/cshell 🐚|\n");
    printf("====================================================\n\n");
    printf("  Type 'help' to see the list of built in commands\n\n\n");
}

void displayHelp()
{
    printf("CShell: A simple shell\n");
    printf("\n");
    printf("Commands:\n");
    printf("exit\n");
    printf("help\n");
    printf("history\n");
    printf("cd\n");
}

void showHistory()
{
    int i;
    for (i = 0; i < numCommands; i++)
    {
        printf("%d: %s\n", i, history[i]);
    }
}

int changeDir(char *args[])
{
    if (args[1] == NULL)
    {
        chdir(getenv("HOME"));
        return 1;
    }

    else
    {
        if (chdir(args[1]) == -1)
        {
            printf(" %s: no such directory\n", args[1]);
            return -1;
        }
    }
    return 0;
}

