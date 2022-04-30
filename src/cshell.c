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

