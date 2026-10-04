#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <papago.h>

#include "db.h"
#include "logger.h"

#define API_BASE "/api/v1"
#define API_USER API_BASE "/user"
#define API_USERS API_BASE "/users"

static papago_t *server = NULL;

/**
 * Signal handler for graceful shutdown.
 */
static void
signal_handler(int sig)
{
    PAPAGO_UNUSED(sig);

    printf("\nShutting down...\n");
    if (server != NULL) {
        papago_stop(server);
    }
}

void
user_handler(papago_request_t *req, papago_response_t *res, void *user_data)
{
    PAPAGO_UNUSED(user_data);

    const char *username;
    char json[256];

    username = papago_req_param(req, "username");

    snprintf(json, sizeof(json), "{\"username\":\"%s\",\"id\":123}",
        (username != NULL) ? username : "unknown");

    papago_res_json(res, json);
}

void
users_handler(papago_request_t *req, papago_response_t *res, void *user_data)
{
    PAPAGO_UNUSED(user_data);

    const char *username;
    char json[256];

    username = papago_req_param(req, "username");

    snprintf(json, sizeof(json), "{\"username\":\"%s\",\"id\":123}",
        (username != NULL) ? username : "unknown");

    papago_res_json(res, json);
}

int
main(void)
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    s_log_init(stdout);

    s_log(S_LOG_INFO, "msg", "starting nelvana server");

    db_init(DB_PATH);

    server = papago_new();
    if (server == NULL) {
        fprintf(stderr, "failed to create server\n");
        return 1;
    }

    papago_route(server, PAPAGO_GET, "/", papago_serve_static_handler, server);
    papago_route(server, PAPAGO_GET, API_USER "/:id", user_handler, NULL);
    papago_route(server, PAPAGO_GET, API_USERS, users_handler, NULL);

    papago_config_t config = papago_default_config();
    config.static_dir = "./public";

    if (papago_start(server, &config) != 0) {
        fprintf(stderr, "%s\n", papago_error());
        papago_destroy(server);

        return 1;
    }

    // cleanup
    papago_destroy(server);

    return 0;
}

