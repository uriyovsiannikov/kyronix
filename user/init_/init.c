#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    int fd = open("/dev/tty", O_RDWR);
    if (fd >= 0) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) close(fd);
    }

    signal(SIGCHLD, SIG_IGN);
    signal(SIGINT, SIG_IGN);

    setenv("PATH", "/bin", 1);
    setenv("HOME", "/", 1);
    setenv("SHELL", "/bin/ksh", 1);
    setenv("TERM", "vt100", 1);

    for (;;) {
        pid_t pid = fork();
        if (pid == 0) {
            setsid();
            execl("/bin/ksh", "ksh", NULL);
            write(STDERR_FILENO, "init: exec /bin/ksh failed\n", 27);
            _exit(127);
        }
        if (pid > 0) {
            waitpid(pid, NULL, 0);
        }
    }
}
