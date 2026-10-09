#define _POSIX_C_SOURCE 199309L
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <jansson.h>
#include <papago.h>

#include "db.h"
#include "nelvana.h"


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

    const char *id_str = papago_req_param(req, "id");
    if (id_str == NULL) {
        papago_res_set_status(res, PAPAGO_STATUS_BAD_REQUEST);
        papago_res_json(res, "{\"error\":\"missing user ID\"}");
        return;
    }

    uint64_t id = strtoull(id_str, NULL, 10);
    user_t *user = db_user_get_by_id(id);
    if (user == NULL) {
        papago_res_set_status(res, PAPAGO_STATUS_NOT_FOUND);
        papago_res_json(res, "{\"error\":\"user not found\"}");
        return;
    }

    char payload[512];
    snprintf(payload, sizeof(payload),
        "{\"id\":%" PRIu64 ",\"username\":\"%s\",\"email\":\"%s\","
        "\"first_name\":\"%s\",\"last_name\":\"%s\"}",
        user->id,
        user->username,
        user->email,
        user->first_name,
        user->last_name);

    papago_res_json(res, payload);

    db_user_free(user);
}

void
users_handler(papago_request_t *req, papago_response_t *res, void *user_data)
{
    PAPAGO_UNUSED(req);
    PAPAGO_UNUSED(user_data);

    size_t count = 0;
    user_t *users = db_users_all(&count);
    if (users == NULL) {
        papago_res_set_status(res, PAPAGO_STATUS_INTERNAL_ERROR);
        papago_res_json(res, "{\"error\":\"internal 1 server error\"}");
        return;
    }

    json_error_t error;
    json_t *json_array_root = json_array();
    if (json_array_root == NULL) {
        papago_res_set_status(res, PAPAGO_STATUS_INTERNAL_ERROR);
        papago_res_json(res, "{\"error\":\"internal server error\"}");
        return;
    }

    for (size_t i = 0; i < count; i++) {
        printf("%" PRIu64 "  %s  %s %s  %s\n", users[i].id, users[i].username,
            users[i].first_name, users[i].last_name, users[i].email);

        json_t *json_user = json_pack(
            "{s: %" PRIu64 ", s:s, s:s, s:s, s:s}",
            "id", users[i].id,
            "username", users[i].username,
            "email", users[i].email,
            "first_name", users[i].first_name,
            "last_name", users[i].last_name
        );
        json_array_append_new(json_array_root, json_user);
    }
    db_users_free(users, count);

    char *payload = json_dumps(json_array_root, 0);
    json_decref(json_array_root);

    papago_res_json(res, payload);
    free(users);
}

static bool
logger_before(papago_request_t *req, papago_response_t *res, void *user_data)
{
    PAPAGO_UNUSED(req);
    PAPAGO_UNUSED(res);
    PAPAGO_UNUSED(user_data);

    return true;
}

static void
logger_after(papago_request_t *req, papago_response_t *res, void *user_data)
{
    PAPAGO_UNUSED(user_data);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double duration_ms = (now.tv_sec  - papago_req_start_time(req).tv_sec) 
        * 1000.0
        + (now.tv_nsec - papago_req_start_time(req).tv_nsec) / 1.0e6;

    fprintf(stdout,
        "{\"remote\":\"%s\",\"method\":\"%s\",\"path\":\"%s\","
        "\"version\":\"%s\",\"host\":\"%s\",\"user_agent\":\"%s\","
        "\"status\":%d,\"duration_ms\":%.3f}\n",
        papago_req_client_ip(req) != NULL ? papago_req_client_ip(req) : "-",
        papago_req_method(req) != NULL ? papago_req_method(req) : "-",
        papago_req_path(req) != NULL ? papago_req_path(req) : "-",
        papago_req_version(req) != NULL ? papago_req_version(req) : "-",
        papago_req_host(req) != NULL ? papago_req_host(req) : "-",
        papago_req_user_agent(req) != NULL ? papago_req_user_agent(req) : "-",
        papago_res_status(res),
        duration_ms);
}

int
main(void)
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

//    s_log_init(stdout);

//   s_log(S_LOG_INFO, "msg", "starting nelvana server");

    db_init(DB_PATH);

    server = papago_new();
    if (server == NULL) {
        fprintf(stderr, "failed to create server\n");
        return 1;
    }

    papago_middleware_t structured_logger = {
        .before    = logger_before,
        .after     = logger_after,
        .user_data = NULL,
    };
    papago_middleware_add(server, &structured_logger);

    // papago_route(server, PAPAGO_GET, "/", landing_handler, server);
    // papago_route(server, PAPAGO_GET, "/static", papago_serve_static_handler, server);
    papago_route(server, PAPAGO_GET, NELVANA_API_USER "/:id", user_handler, NULL);
    papago_route(server, PAPAGO_GET, NELVANA_API_USERS, users_handler, NULL);

    papago_config_t config = papago_default_config();
    config.static_dir = "./public/static";
    config.enable_template_rendering = true;

    if (papago_start(server, &config) != 0) {
        fprintf(stderr, "%s\n", papago_error());
        papago_destroy(server);

        return 1;
    }

    // cleanup
    papago_destroy(server);

    return 0;
}

