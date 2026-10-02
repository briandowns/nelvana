#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

#define DB_PATH "/usr/local/var/db/nelvana.db"

static int
print_keys(sqlite3 *db, const char *username)
{
    sqlite3_stmt *stmt;

    const char *sql =
        "SELECT ssh_keys.public_key "
        "FROM ssh_keys "
        "JOIN users ON users.id = ssh_keys.user_id "
        "WHERE users.username = ?1";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return 1;

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const unsigned char *key;

        key = sqlite3_column_text(stmt, 0);

        if (key != NULL)
            puts((const char *)key);
    }

    sqlite3_finalize(stmt);

    return 0;
}

int
main(int argc, char **argv)
{
    sqlite3 *db;
    int rc;

    if (argc != 2)
        return 1;

    rc = sqlite3_open_v2(
        DB_PATH,
        &db,
        SQLITE_OPEN_READONLY,
        NULL
    );

    if (rc != SQLITE_OK)
        return 1;

    rc = print_keys(db, argv[1]);

    sqlite3_close(db);

    return rc;
}

