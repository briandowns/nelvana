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
#include <stdio.h>

#include "nelvana.h"

#include <rattler.h>

static void
user_add_cmd(rattler_cmd *cmd, int argc, char **argv)
{
    for (int i = 0; i < argc; i++) {
        printf("  - %s\n", argv[i]);
    }
}


static void
user_del_cmd(rattler_cmd *cmd, int argc, char **argv)
{
    for (int i = 0; i < argc; i++) {
        printf("  - %s\n", argv[i]);
    }
}

static void
login_cmd(rattler_cmd *cmd, int argc, char **argv)
{
    RATTLER_UNUSED(argc);
    RATTLER_UNUSED(argv);

    const char *user = rattler_flag_string(cmd, "user");

    printf("Logged in as: %s\n", *user ? user : "(anonymous)");
}

int
main(int argc, char **argv)
{
    rattler_cmd *root = rattler_new_command(
        "nelvanactl [command]", "Nelvana CLI Utility",
        "individually required flags.");
    rattler_set_version(root, "v0.1.0");
    rattler_persistent_bool(root, "verbose", 'v', false, "verbose output");

    rattler_cmd *add = rattler_new_command(
        "add [flags] [user]", "Add user",
        "Add a user.");
    add->cmd = user_add_cmd;

    rattler_cmd *del = rattler_new_command(
        "delete [flags] [user]", "Remove user",
        "Delete a user.\nAlso callable as: rm del");
    del->cmd = user_del_cmd;
    rattler_add_alias(del, "rm");
    rattler_add_alias(del, "del");

    rattler_cmd *login = rattler_new_command(
        "login [flags]", "Log in to the service",
        "Authenticate. --user and --pass must be supplied together.");
    login->cmd = login_cmd;
    rattler_flags_string(login, "user", 'u', "", "username");
    rattler_flags_string(login, "pass", 'p', "", "password");
    rattler_mark_flags_required_together(login, "user", "pass", NULL);

    rattler_add_command(root, add);
    rattler_add_command(root, del);
    rattler_add_command(root, login);

    if (rattler_execute(root, argc, argv) != 0) {
        fprintf(stderr, "error: failed to cmd\n");
        return 1;
    }

    rattler_free(root);

    return 0;
}

