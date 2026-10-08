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

#ifndef __NELVANA_H
#define __NELVANA_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define DB_PATH "/usr/local/var/db/nelvana.db"

typedef struct {
    uint64_t id;
    char *username;
    char *email;
    char *first_name;
    char *last_name;
    char *password;
} user_t;

typedef struct {
    uint64_t id;
    uint64_t user_id;
    char public_key[8193];
    char fingerprint[65];
} ssh_key_t;

typedef struct {
    uint64_t id;
    uint64_t user_id;
    char name[256];
    char container_id[64];
    char image[256];
    bool persistent;
    time_t last_started_at;
    time_t last_stopped_at;
} container_t;

uint8_t
db_init(const char *path);

void
db_close(void);

uint8_t
db_user_add(const user_t *user);

user_t*
db_user_get_by_id(const uint64_t id);

uint8_t
db_user_list(void);

uint8_t
db_user_delete(const uint64_t id);

void
db_user_free(user_t *user);

uint8_t
db_key_add(const char *username, const char *public_key,
           const char *fingerprint);

ssh_key_t*
db_key_get_by_username(const char *username);

ssh_key_t*
db_key_get_by_fingerprint(const char *fingerprint);

uint8_t
db_key_del(const char *fingerprint);

container_t*
db_container_get_by_user_id(const uint64_t id);

#endif /** end __NELVANA_H */

