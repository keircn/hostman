#include "hostman/cli/cli.h"
#include "hostman/core/config.h"
#include "hostman/core/logging.h"
#include "hostman/core/utils.h"
#include "hostman/crypto/encryption.h"
#include "hostman/network/network.h"
#include "hostman/storage/database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
    if (argc > 1 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0))
    {
        print_version_info();
        return EXIT_SUCCESS;
    }

    logging_init();

    char *config_path = config_get_path();
    struct stat st;
    bool first_run = (stat(config_path, &st) != 0);
    free(config_path);

    if (first_run)
    {
        log_info("First run detected. Starting setup wizard.");
        return run_setup_wizard();
    }

    if (!encryption_init())
    {
        log_error("Failed to initialize encryption system");
        logging_cleanup();
        return EXIT_FAILURE;
    }

    if (!network_init())
    {
        log_error("Failed to initialize network system");
        encryption_cleanup();
        logging_cleanup();
        return EXIT_FAILURE;
    }

    if (!db_init())
    {
        log_error("Failed to initialize database system");
        network_cleanup();
        encryption_cleanup();
        logging_cleanup();
        return EXIT_FAILURE;
    }

    command_args_t args = parse_args(argc, argv);

    int result = execute_command(&args);

    free_command_args(&args);
    encryption_cleanup();
    network_cleanup();
    db_close();
    logging_cleanup();

    return result;
}
