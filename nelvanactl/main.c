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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <papago.h>
#include <rattler.h>

#include "../nelvana.h"

#define STR1(x) #x
#define STR(x) STR1(x)

typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE
} http_method;

typedef struct {
    char *body;
    long status_code;
    CURLcode curl_code;
} http_resp;

struct resp_buffer {
    char *memory;
    size_t size;
};

static const char *server;
static const char *token;

static size_t
write_callback(void *contents, size_t size, size_t nmemb, void *user_data)
{
    size_t realsize = size * nmemb;
    struct resp_buffer *mem = (struct resp_buffer *)user_data;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

static void
user_add_cmd(rattler_cmd *cmd, int argc, char **argv)
{
    RATTLER_UNUSED(cmd);
    RATTLER_UNUSED(argc);
    RATTLER_UNUSED(argv);

    for (int i = 0; i < argc; i++) {
        printf("  - %s\n", argv[i]);
    }
}

http_resp*
api_request(const char *url, http_method method, const char *token, const char *payload)
{
    http_resp *response = malloc(sizeof(http_resp));
    if (response == NULL) {
        return NULL;
    }

    response->body = NULL;
    response->status_code = 0;
    response->curl_code = CURLE_OK;

    struct resp_buffer chunk = {0};
    chunk.memory = malloc(1);
    if (chunk.memory == NULL) {
        free(response);
        return NULL;
    }
    chunk.size = 0;

    CURL *curl = curl_easy_init();
    if (!curl) {
        free(chunk.memory);
        free(response);
        return NULL;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    if (token) {
        char auth_header[512];
        snprintf(auth_header, sizeof(auth_header), "X-Nelvana-Token: %s", token);
        headers = curl_slist_append(headers, auth_header);
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    switch (method) {
        case HTTP_POST:
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            if (payload != NULL) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
            }
            break;
        case HTTP_PUT:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            if (payload) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
            }
            break;
        case HTTP_DELETE:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            break;
        case HTTP_GET:
        default:
            break;
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    CURLcode res = curl_easy_perform(curl);
    response->curl_code = res;

    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->status_code);
        response->body = chunk.memory;
    } else {
        free(chunk.memory);
        response->body = NULL;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return response;
}

static void
free_http_response(http_resp *resp)
{
    if (resp != NULL) {
        free(resp->body);
        free(resp);
    }
}

static uint8_t
base_flag_config(rattler_cmd *cmd)
{
    server = rattler_flag_string(cmd, "server");
    printf("XXX - %s\n", server);
    if (server == NULL || strlen(server) == 0) {
        server = getenv("NELVANA_SERVER");
        if (server == NULL || strlen(server) == 0) {
            fprintf(stderr, "error: server not specified\n");
            return 1;
        }
        printf("server: %s\n", server);
    }
    token = rattler_flag_string(cmd, "token");
    printf("%s - %s\n", server, token);
    return 0;
}

static void
user_del_cmd(rattler_cmd *cmd, int argc, char **argv)
{
    RATTLER_UNUSED(cmd);
    RATTLER_UNUSED(argc);
    RATTLER_UNUSED(argv);

    for (int i = 0; i < argc; i++) {
        printf("  - %s\n", argv[i]);
    }
}

static void
list_cmd(rattler_cmd *cmd, int argc, char **argv)
{
    RATTLER_UNUSED(argc);
    RATTLER_UNUSED(argv);

    if (argc < 1) {
        fprintf(stderr, "error: missing subcommand\n");
        return;
    }

    base_flag_config(cmd);

    const char *user = rattler_flag_string(cmd, "user");
    const char *format = rattler_flag_string(cmd, "format");

    const char *list_item = argv[0];

    if (strcmp(list_item, "user") == 0) {
        const char *username = rattler_flag_string(cmd, "user");
        const char *user_id = rattler_flag_string(cmd, "user-id");


        http_resp *res = api_request("http://192.168.122.81:8080/api/v1/user/", HTTP_GET, token, NULL);
        if (res == NULL) {
            fprintf(stderr, "error: failed to retrieve user\n");
            return;
        }

        if (res) {
            if (res->body) {
                printf("Response JSON: %s\n", res->body);
            }
            free_http_response(res);
        }

        return;
    } else if (strcmp(list_item, "users") == 0) {
        return;
    } else if (strcmp(list_item, "keys") == 0) {
        return;
    } else {
        fprintf(stderr, "error: unknown resource\n");
        return;
    }
}

int
main(int argc, char **argv)
{
    rattler_cmd *root = rattler_new_command(
        "nelvanactl [command]", "Nelvana Client CLI", "");
    rattler_set_version(root, STR(nelvanactl_version));
    rattler_persistent_bool(root, "verbose", 'v', false, "verbose output");
    rattler_persistent_string(root, "server", 's', "",
        "IP Address and Port (colon seperated)");
    rattler_persistent_string(root, "token", 't', "", "API token");

    curl_global_init(CURL_GLOBAL_DEFAULT);

    rattler_cmd *add = rattler_new_command(
        "add [flags] [user]", "Add a resource",
        "Add a resource.");
    add->cmd = user_add_cmd;

    rattler_cmd *del = rattler_new_command(
        "delete [flags] [key]", "Delete a resource",
        "Delete a user.\nAlso callable as: rm del");
    del->cmd = user_del_cmd;
    rattler_add_alias(del, "rm");
    rattler_add_alias(del, "del");
    rattler_flags_string(del, "user-id", 'u', "", "User ID");
    rattler_flags_string(del, "key-id", 'k', "", "Key ID");

    rattler_cmd *list = rattler_new_command(
        "list [flags] [resource]", "List resources",
        "List resources.\nAlso callable as: ls");
    list->cmd = list_cmd;
    rattler_add_alias(list, "ls");
    rattler_set_args(list, 1, 1);
    rattler_flags_string(list, "user-id", 'u', "", "User ID");
    rattler_flags_string(list, "username", 'n', "", "Username");
    rattler_flags_string(list, "format", 'f', "table",
        "Format output (json, yaml, table (default))");

    rattler_add_command(root, add);
    rattler_add_command(root, del);
    rattler_add_command(root, list);

    if (rattler_execute(root, argc, argv) != 0) {
        fprintf(stderr, "error: failed\n");
        goto CLEANUP;
        return 1;
    }

CLEANUP:
    curl_global_cleanup();
    rattler_free(root);

    return 0;
}

