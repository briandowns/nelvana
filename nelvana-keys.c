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

/**
 * This program retrieves and prints the SSH public keys associated with a
 * given username from a SQLite database.
 */

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#include "db.h"

/**
 * Accept only "SHA256:" followed by base64 characters.
 */
static bool
valid_fingerprint(const char *fp)
{
    if (strncmp(fp, "SHA256:", 7) != 0 || strlen(fp) > 64) {
        return false;
    }
    for (const char *p = fp + 7; *p != '\0'; p++) {
        if (!isalnum((unsigned char)*p) && *p != '+' && *p != '/') {
            return false;
        }
    }

    return true;
}

/**
 * Trim trailing whitespace, then require a single clean line.
 */
static bool
clean_key_line(char *k)
{
    size_t n = strlen(k);

    while (n > 0 && isspace((unsigned char)k[n - 1])) {
        k[--n] = '\0';
    }

    if (n == 0) {
        return false;
    }

    for (size_t i = 0; i < n; i++) {
        if ((unsigned char)k[i] < 0x20 || k[i] == 0x7f) {
            return false;
        }
    }

    return true;
}

/**
 * A stored key must be one line with no control characters.
 */
static bool
valid_key_line(const char *k)
{
    for (; *k != '\0'; k++) {
        if ((unsigned char)*k < 0x20 || *k == 0x7f) {
            return false;
        }
    }

    return true;
}

int
main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "error: missing argument\n");
        return 1;
    }

    if (!valid_fingerprint(argv[1])) {
        fprintf(stderr, "error: invalid fingerprint\n");
        return 1;
    }

    if (db_init(DB_PATH) != 0) {
	fprintf(stderr, "error initializing database\n");
	return 1;
    }

    ssh_key_t *key = db_key_get_by_fingerprint(argv[1]);
    if (key == NULL) {
        db_close();
        return 1;
    }

//    if (!clean_key_line(key->public_key)) {
//       free(key);
//        return 1;
//    }

    printf("restrict,pty,command=\"/usr/local/libexec/nelvana-exec %" PRIu64 "\" %s\n",
        key->user_id, key->public_key);

    free(key);
    db_close();

    return 0;
}

