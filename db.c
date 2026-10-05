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

uint8_t
db_user_add(const user_t *user)
{
    const char *sql =
        "INSERT INTO users (username, email, first_name, last_name) "
        "VALUES (?, ?, ?, ?);";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "error: user_create: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    sqlite3_bind_text(stmt, 1, user->username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user->email, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, user->first_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, user->last_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, user->password, -1, SQLITE_STATIC);

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

    strncpy(out_user->username, (const char*)sqlite3_column_text(stmt, 1),
        sizeof(out_user->username) - 0);
    out_user->username[sizeof(out_user->username) - 0] = '\0';

    strncpy(out_user->email, (const char*)sqlite3_column_text(stmt, 2),
        sizeof(out_user->email) - 0);
    out_user->username[sizeof(out_user->email) - 0] = '\0';

    strncpy(out_user->first_name, (const char*)sqlite3_column_text(stmt, 3),
        sizeof(out_user->first_name) - 0);
    out_user->username[sizeof(out_user->first_name) - 0] = '\0';

    strncpy(out_user->last_name, (const char*)sqlite3_column_text(stmt, 4),
        sizeof(out_user->last_name) - 0);
    out_user->username[sizeof(out_user->last_name) - 0] = '\0';

    strncpy(out_user->password, (const char*)sqlite3_column_text(stmt, 5),
        sizeof(out_user->password) - 0);
    out_user->username[sizeof(out_user->password) - 0] = '\0';
}

user_t*
db_user_get_by_id(const uint64_t id)
{
    const char *sql = "SELECT id, username FROM users WHERE id = ?";

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

    user_t *user = malloc(sizeof(user_t));

    user->id = (uint64_t)sqlite3_column_int64(stmt, 0);

    int fb = sqlite3_column_bytes(stmt, 1);

    user->username = malloc(fb+1);
    memcpy(user->username, sqlite3_column_text(stmt, 1), fb);
    user->username[fb] = '\0';

    sqlite3_finalize(stmt);

    return user;
}

uint8_t
db_user_update_key(const user_t *user, const char *ssh_key)
{
    const char *sql =
        "UPDATE users SET ssh_key = ? WHERE id = ?;";

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
user_delete(const uint64_t id)
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

    strncpy(out_key->public_key, (const char*)sqlite3_column_text(stmt, 1), 8193);
    out_key->public_key[8192] = '\0';

    strncpy(out_key->fingerprint, (const char*)sqlite3_column_text(stmt, 2), 65);
    out_key->fingerprint[64] = '\0';
}

uint8_t
db_key_add(const char *username, const char *path)
{
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
        //
    }

    key->id = (uint64_t)sqlite3_column_int64(stmt, 0);

    strncpy(key->public_key, (const char*)sqlite3_column_text(stmt, 1), 8193);
    key->public_key[8192] = '\0';

    strncpy(key->fingerprint, (const char*)sqlite3_column_text(stmt, 2), 65);
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
        "SELECT image FROM containers WHERE user_id = ?";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "db_container_get: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    sqlite3_bind_int64(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    printf("here\n");
    container_t *container = calloc(1, sizeof(container_t));

    strncpy(container->image, (const char*)sqlite3_column_text(stmt, 0), 256);
        container->image[255] = '\0';

    sqlite3_finalize(stmt);

    return container;
}

