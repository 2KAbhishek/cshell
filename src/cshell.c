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

