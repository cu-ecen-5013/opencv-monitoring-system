#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

char END_PROGRAM;
char RECORD_VIDEO;

// Handles SIGINT and SIGTERM
// Used to close the stream_socket
void intHandler(int sig)
{
    END_PROGRAM = 1;
}

void pipeHandler(int sig){}

// Sig alarm triggered when count down reaches zero
// Flips flag to write timestamp to AESDLOG
void alarm_handler (int signum)
{
	RECORD_VIDEO = false;
}

// Innitialize timer for 0 interval
// Initial delay time: <t> second
void timer_init(int t)
{
	TRACE_LOG("TIMER INNITIALIZED");
	struct itimerval delay;
	int ret;
	signal (SIGALRM, alarm_handler);
	delay.it_value.tv_sec = t;
	delay.it_value.tv_usec = 0;
	delay.it_interval.tv_sec = 0;
	delay.it_interval.tv_usec = 0;
	ret = setitimer(ITIMER_REAL, &delay, NULL);
	if (ret)
	{
		perror ("setitimer");
		return;
	}
	return;
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
    struct sigaction sact_pipe;
    memset (&sact, '\0', sizeof(sact_pipe));
    memset (&sact, '\0', sizeof(sact));
    sact_pipe.sa_handler = pipeHandler;
    sact_pipe.sa_flags = 1;
    sact.sa_handler = intHandler;       //
    sact.sa_flags = 1;                  //
    sigaction(SIGINT, &sact, NULL);     //
    sigaction(SIGTERM, &sact, NULL);
    sigaction(SIGPIPE, &sact_pipe, NULL);

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
