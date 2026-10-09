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

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#include "db.h"

static sqlite3 *db;

static const char *SCHEMA_SQL =
    "PRAGMA foreign_keys = ON;"
    "CREATE TABLE IF NOT EXISTS users ("
    "    id INTEGER PRIMARY KEY,"
    "    username TEXT NOT NULL UNIQUE,"
    "    email TEXT,"
    "    first_name TEXT,"
    "    last_name TEXT,"
    "    password TEXT"
    ");"

    "CREATE TABLE IF NOT EXISTS ssh_keys ("
    "    id INTEGER PRIMARY KEY,"
    "    user_id INTEGER NOT NULL,"
    "    public_key TEXT NOT NULL UNIQUE,"
    "    fingerprint TEXT NOT NULL UNIQUE,"
    "    FOREIGN KEY (user_id) REFERENCES users(id) "
    "        ON DELETE CASCADE"
    ");"

    "CREATE TABLE IF NOT EXISTS containers ("
    "    id INTEGER PRIMARY KEY,"
    "    user_id INTEGER NOT NULL,"
    "    name TEXT NOT NULL,"
    "    container_id TEXT,"
    "    image TEXT NOT NULL,"
    "    persistent INTEGER NOT NULL DEFAULT 0,"
    "    last_started_at INTEGER,"
    "    last_stopped_at INTEGER,"
    "    FOREIGN KEY (user_id) REFERENCES users(id) "
    "        ON DELETE CASCADE"
    ");";

static int
create_schema(void)
{
    char *errmsg = NULL;
    if (sqlite3_exec(db, SCHEMA_SQL, NULL, NULL, &errmsg) != SQLITE_OK) {
        fprintf(stderr, "error: create_schema: %s\n", errmsg);
        sqlite3_free(errmsg);
        return 1;
    }

    return 0;
}

uint8_t
db_init(const char *path)
{
    if (sqlite3_open(path, &db) != SQLITE_OK) {
        fprintf(stderr, "error: db_open: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    if (create_schema() != 0) {
        sqlite3_close(db);
        db = NULL;
        return 1;
    }

    return 0;
}

void
db_close(void)
{
    if (db != NULL) {
        sqlite3_close(db);
        db = NULL;
    }
}

/**
 * Copy a text column into a fixed buffer.
 */
static void
col_copy(char *dst, size_t dstsz, sqlite3_stmt *stmt, int col)
{
    const char *s = (const char*)sqlite3_column_text(stmt, col);
    snprintf(dst, dstsz, "%s", s != NULL ? s : "");
}

/**
 * Duplicate a text column into heap memory.
 */
static char*
col_dup(sqlite3_stmt *stmt, int col)
{
    const char *s = (const char*)sqlite3_column_text(stmt, col);
    return strdup(s != NULL ? s : "");
}

uint8_t
db_user_add(const user_t *user)
{
    const char *sql =
        "INSERT INTO users (username, email, first_name, last_name, password) "
        "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "error: user_create: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    sqlite3_bind_text(stmt, 1, user->username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user->email, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, user->first_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, user->last_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, user->password, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "error: user_create: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 1;
    }
    sqlite3_finalize(stmt);

    return 0;
}

static void
user_from_row(sqlite3_stmt *stmt, user_t *out_user)
{
    out_user->id = (uint64_t)sqlite3_column_int64(stmt, 0);
    out_user->username = col_dup(stmt, 1);
    out_user->email = col_dup(stmt, 2);
    out_user->first_name = col_dup(stmt, 3);
    out_user->last_name = col_dup(stmt, 4);
    out_user->password = col_dup(stmt, 5);
}

void
db_user_free(user_t *user)
{
    if (user == NULL) {
        return;
    }

    free(user->username);
    free(user->email);
    free(user->first_name);
    free(user->last_name);
    free(user->password);
    free(user);
}

void
db_users_free(user_t *users, size_t count)
{
    if (users == NULL) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        db_user_free(&users[i]);
    }

    free(users);
}

user_t*
db_users_all(size_t *count)
{
    if (count == NULL) {
        return NULL;
    }
    *count = 0;

    const char *sql =
        "SELECT id, username, email, first_name, last_name, password "
        "FROM users ORDER BY id;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "error: db_users_all: %s\n", sqlite3_errmsg(db));
        return NULL;
    }

    size_t cap = 8;
    size_t n = 0;

    user_t *users = calloc(cap, sizeof(user_t));
    if (users == NULL) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (n == cap) {
            size_t ncap = cap * 2;
            user_t *tmp = realloc(users, ncap * sizeof(user_t));
            if (tmp == NULL) {
                goto CLEANUP;
            }
            users = tmp;
            memset(users + cap, 0, (ncap - cap) * sizeof(user_t));
            cap = ncap;
        }

        user_from_row(stmt, &users[n]);
        n++;
    }

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "error: db_users_all: %s\n", sqlite3_errmsg(db));
        goto CLEANUP;
    }

    sqlite3_finalize(stmt);
    *count = n;

    return users;

CLEANUP:
    sqlite3_finalize(stmt);
    db_users_free(users, n);

    return NULL;
}

user_t*
db_user_get_by_id(const uint64_t id)
{
    const char *sql = "SELECT id, username, email, first_name, last_name, password "
        "FROM users WHERE id = ?";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "user_get: %s\n", sqlite3_errmsg(db));
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    user_t *user = calloc(1, sizeof(user_t));
    if (user == NULL) {
        sqlite3_finalize(stmt);
        return NULL;
    }
    user_from_row(stmt, user); 

    sqlite3_finalize(stmt);

    return user;
}

uint8_t
db_user_update_key(const user_t *user, const char *ssh_key)
{
    const char *sql =
        "UPDATE ssh_keys SET public_key = ? WHERE id = ?;";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "error: user_update: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    sqlite3_bind_text(stmt, 1, ssh_key, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, user->id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "error: user_update: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 1;
    }

    sqlite3_finalize(stmt);

    return 0;
}

uint8_t
db_user_delete(const uint64_t id)
{
    const char *del_user_sql = "DELETE FROM users WHERE id = ?;";

    sqlite3_stmt *stmt;
    sqlite3_step(stmt);

    if (sqlite3_prepare_v2(db, del_user_sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "error: db_user_delete: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    sqlite3_bind_int64(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "error: db_user_delete: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 1;
    }

    sqlite3_finalize(stmt);

    return 0;
}

static void
key_from_row(sqlite3_stmt *stmt, ssh_key_t *out_key)
{
    out_key->id = (uint64_t)sqlite3_column_int64(stmt, 0);
    col_copy(out_key->public_key, sizeof(out_key->public_key), stmt, 1);
    col_copy(out_key->fingerprint, sizeof(out_key->fingerprint), stmt, 2);
}

uint8_t
db_key_add(const char *username, const char *public_key,
           const char *fingerprint)
{
        if (username == NULL || public_key == NULL || fingerprint == NULL) {
        fprintf(stderr, "error: db_key_add: invalid argument\n");
        return 1;
    }

    /* Ignore trailing whitespace such as the newline from a .pub file. */
    size_t klen = strlen(public_key);
    while (klen > 0 && isspace((unsigned char)public_key[klen - 1])) {
        klen--;
    }

    if (klen == 0 || klen >= sizeof(((ssh_key_t *)0)->public_key)) {
        fprintf(stderr, "error: db_key_add: invalid key length\n");
        return 1;
    }

    for (size_t i = 0; i < klen; i++) {
        if ((unsigned char)public_key[i] < 0x20 ||
            public_key[i] == 0x7f) {
            fprintf(stderr, "error: db_key_add: invalid key data\n");
            return 1;
        }
    }

#ifdef __FreedBSD__
    if (strlcmp(fingerprint, "SHA256:", 7) != 0 ||
#else
    if (strncmp(fingerprint, "SHA256:", 7) != 0 ||
#endif
        strlen(fingerprint) >= sizeof(((ssh_key_t *)0)->fingerprint)) {
        fprintf(stderr, "error: db_key_add: invalid fingerprint\n");
        return 1;
    }

    const char *sql =
        "INSERT INTO ssh_keys (user_id, public_key, fingerprint) "
        "SELECT id, ?1, ?2 FROM users WHERE username = ?3;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "error: db_key_add: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    sqlite3_bind_text(stmt, 1, public_key, (int)klen, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, fingerprint, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, username, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_CONSTRAINT) {
        fprintf(stderr, "error: db_key_add: key or fingerprint "
            "already exists\n");
        return 1;
    }

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "error: db_key_add: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    if (sqlite3_changes(db) == 0) {
        fprintf(stderr, "error: db_key_add: user not found: %s\n",
            username);
        return 1;
    }

    return 0;
}

ssh_key_t*
db_key_get_by_username(const char *username)
{
    const char *sql =
        "SELECT ssh_keys.user_id, ssh_keys.public_key, ssh_keys.fingerprint "
        "FROM ssh_keys "
        "JOIN users ON users.id = ssh_keys.user_id "
        "WHERE users.username = ?";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "db_key_get: %s\n", sqlite3_errmsg(db));
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    ssh_key_t *key = calloc(1, sizeof(ssh_key_t));
    if (key == NULL) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    key->id = (uint64_t)sqlite3_column_int64(stmt, 0);

#ifdef __FreeBSD__
    strlcpy(key->public_key, (const char*)sqlite3_column_text(stmt, 1), 8193);
#else
    strncpy(key->public_key, (const char*)sqlite3_column_text(stmt, 1), 8193);
#endif
    key->public_key[8192] = '\0';

#ifdef __FreeBSD__
    strlcpy(key->fingerprint, (const char*)sqlite3_column_text(stmt, 2), 65);
#else
    strncpy(key->fingerprint, (const char*)sqlite3_column_text(stmt, 2), 65);
#endif
    key->fingerprint[64] = '\0';


    sqlite3_finalize(stmt);

    return key;
}

ssh_key_t*
db_key_get_by_fingerprint(const char *fingerprint)
{
    static const char *sql =
        "SELECT id, user_id, public_key, fingerprint "
        "FROM ssh_keys WHERE fingerprint = ?1 LIMIT 1;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, fingerprint, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    ssh_key_t *key = calloc(1, sizeof(ssh_key_t));
    if (key == NULL) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    const char *pk = (const char *)sqlite3_column_text(stmt, 2);
    const char *fp = (const char *)sqlite3_column_text(stmt, 3);
    if (pk == NULL || fp == NULL) {
        sqlite3_finalize(stmt);
        free(key);
        return NULL;
    }

    key->id = (uint64_t)sqlite3_column_int64(stmt, 0);
    key->user_id = (uint64_t)sqlite3_column_int64(stmt, 1);

    snprintf(key->public_key, sizeof(key->public_key), "%s", pk);
    snprintf(key->fingerprint, sizeof(key->fingerprint), "%s", fp);

    sqlite3_finalize(stmt);

    return key;
}

uint8_t
db_key_del(const char *fingerprint)
{
    return 0;
}

container_t*
db_container_get_by_user_id(const uint64_t id)
{
    const char *sql =
        "SELECT id, user_id, name, container_id, image, persistent, "
        "last_started_at, last_stopped_at "
        "FROM containers WHERE user_id = ? LIMIT 1;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "db_container_get: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    sqlite3_bind_int64(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    container_t *c = calloc(1, sizeof(container_t));
    if (c == NULL) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    c->id = (uint64_t)sqlite3_column_int64(stmt, 0);
    c->user_id = (uint64_t)sqlite3_column_int64(stmt, 1);
    col_copy(c->name, sizeof(c->name), stmt, 2);
    col_copy(c->container_id, sizeof(c->container_id), stmt, 3);
    col_copy(c->image, sizeof(c->image), stmt, 4);
    c->persistent = sqlite3_column_int(stmt, 5) != 0;
    c->last_started_at = (time_t)sqlite3_column_int64(stmt, 6);
    c->last_stopped_at = (time_t)sqlite3_column_int64(stmt, 7);

    sqlite3_finalize(stmt);

    return c;
}

