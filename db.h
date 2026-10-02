#ifndef __NELVANA_H
#define __NELVANA_H

#include <stdint.h>

#define DB_PATH "/usr/local/var/db/nelvana.db"

typedef struct {
    uint64_t id;
    char *username;
    char *email;
    char *first_name;
    char *last_name;
} user_t;

uint8_t
db_init(void);

void
db_close(void);

uint8_t
db_user_add(const *user_t);

uint8_t
db_user_get(const char *id, user_t *out_user)

uint8_t
db_user_list(void);

uint8_t
db_key_add(const char *username, const char *path);

uint8_t
db_key_list(const char *username);

uint8_t
db_key_del(const char *fingerprint);

#endif /** end __NELVANA_H */

