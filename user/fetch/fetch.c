//! AIused

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>

/*
 * ASCII art placeholder — replace with your own.
 * Each string in the array is one line.
 * Must all be the same width (padded with spaces).
 */
static const char *logo[] = {
    " ____  __.",
    "|    |/ _|",
    "|      <    ",
    "|    |  \\   ",
    "|____|__ \\  ",
    "        \\/",
};

#define LOGO_LEN  ((int)(sizeof(logo) / sizeof(logo[0])))

int main(void) {
    struct utsname uts;
    char host[256], osline[256];
    struct passwd *pw;
    const char *user, *shell;
    int i;

    uname(&uts);
    gethostname(host, sizeof(host));
    host[sizeof(host) - 1] = '\0';

    pw = getpwuid(getuid());
    user = (pw && pw->pw_name) ? pw->pw_name : getenv("USER");
    if (!user) user = "user";

    shell = getenv("SHELL");
    if (!shell) shell = "/bin/sh";

    snprintf(osline, sizeof(osline), "%s %s", uts.sysname, uts.release);

    /* header */
    {
        int hlen = strlen(user) + 1 + strlen(host);
        printf("%s  \033[1;36m%s@%s\033[0m\n", logo[0], user, host);
        printf("%s  ", logo[1]);
        for (i = 0; i < hlen; i++) putchar('-');
        putchar('\n');
    }

    /* info lines, interleaved with logo rows */
    {
        const char *labels[] = { "OS", "Kernel", "Shell" };
        const char *values[] = { osline, uts.machine, shell };
        int n = (int)(sizeof(labels) / sizeof(labels[0]));
        int maxw = 0;

        for (i = 0; i < n; i++) {
            int w = (int)strlen(labels[i]);
            if (w > maxw) maxw = w;
        }

        for (i = 0; i < n; i++) {
            const char *l = (i + 2 < LOGO_LEN) ? logo[i + 2] : "";
            printf("%s\033[90m%-*s:\033[0m %s\n",
                   l, maxw, labels[i], values[i]);
        }

        /* remaining logo lines */
        for (i = n + 2; i < LOGO_LEN; i++)
            printf("%s\n", logo[i]);
    }

    /* palette bar — aligned to match indent of info values */
    {
        int lw = (int)strlen(logo[0]);
        printf("\n%*s", lw, "");
        printf(" \033[40m  \033[41m  \033[42m  \033[43m  "
               "  \033[44m  \033[45m  \033[46m  \033[47m\033[0m\n");
    }

    return 0;
}
