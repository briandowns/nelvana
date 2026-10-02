#include <stdio.h>
#include <string.h>

#include "nelvana.h"

static void
usage(const char *program)
{
    fprintf(stderr,
        "usage:\n"
        "  %s user create <username>\n"
        "  %s user list\n"
        "  %s key add <username> <public-key-file>\n"
        "  %s key list <username>\n"
        "  %s key remove <fingerprint>\n",
        program,
        program,
        program,
        program,
        program);
}

int
main(int argc, char **argv)
{
    if (nelvana_db_init() != 0)
        return 1;

    if (argc < 3)
    {
        usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "user") == 0)
    {
        if (strcmp(argv[2], "create") == 0 && argc == 4)
            return nelvana_user_create(argv[3]);

        if (strcmp(argv[2], "list") == 0 && argc == 3)
            return nelvana_user_list();
    }

    if (strcmp(argv[1], "key") == 0)
    {
        if (strcmp(argv[2], "add") == 0 && argc == 5)
            return nelvana_key_add(argv[3], argv[4]);

        if (strcmp(argv[2], "list") == 0 && argc == 4)
            return nelvana_key_list(argv[3]);

        if (strcmp(argv[2], "remove") == 0 && argc == 4)
            return nelvana_key_remove(argv[3]);
    }

    usage(argv[0]);

    return 1;
}

