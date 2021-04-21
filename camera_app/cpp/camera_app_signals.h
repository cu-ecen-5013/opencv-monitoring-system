#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

// Handles SIGINT and SIGTERM
// Used to close the stream_socket
void intHandler(int sig)
{
    // No requirements currently
}


void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
   int saved_errno = errno;
   while(waitpid(-1, NULL, WNOHANG) > 0);
   errno = saved_errno;
}

// Initialize signal handlers
void init_sigHandlers()
{

    struct sigaction sact;              //
    memset (&sact, '\0', sizeof(sact));
    sact.sa_handler = intHandler;       //
    sact.sa_flags = 1;                  //
    sigaction(SIGINT, &sact, NULL);     //
    sigaction(SIGTERM, &sact, NULL);
    sigaction(SIGPIPE, &sact, NULL);

    struct sigaction sa;                //
    memset (&sa, '\0', sizeof(sa));

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
}
