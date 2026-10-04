/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 Brian J. Downs
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "db.h"

int
main(void)
{
    char *user_id = getenv("NELVANA_USER_ID");
    if (user_id == NULL || user_id[0] == '\0') {
        return 1;
    }

    char *endptr;
    errno = 0; 

    unsigned long long val = strtoull(user_id, &endptr, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "error: value out of range for an unsigned long long.\n");
        return 1;
    }

    if (user_id == endptr) {
        fprintf(stderr, "error: no digits found in the string.\n");
        return 1;
    }

    uint64_t result = (uint64_t)val;

    db_init(DB_PATH);

    user_t *user = db_user_get_by_id(val);
    if (user == NULL) {
	fprintf(stderr, "error: retrieving user\n");
        return 1;
    }

    db_close();

    // from here, user the info to look up the container
    printf("XXX - test\n");

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
