#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int
main(void)
{
    char *argv[] = {
        "/usr/local/bin/sudo",
	"/usr/local/bin/podman",
        "run",
        "--rm",
        "--interactive",
        "--tty",
        "ghcr.io/freebsd/freebsd-runtime:15.1",
        "/bin/sh",
        NULL
    };

    execv(argv[0], argv);

    printf("%s\n", strerror(errno));

    return 1;
}

