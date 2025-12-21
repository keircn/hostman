#include "hostman/cli/cli.h"
#include "hostman/cli/tui.h"
#include "hostman/core/config.h"
#include "hostman/core/logging.h"
#include "hostman/core/utils.h"
#include "hostman/network/hosts.h"
#include "hostman/network/network.h"
#include "hostman/storage/database.h"
#include <dirent.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define EXIT_INVALID_ARGS 2
#define EXIT_NETWORK_ERROR 3
#define EXIT_FILE_ERROR 4
#define EXIT_CONFIG_ERROR 5

static bool use_color = true;
static output_mode_t current_output_mode = OUTPUT_NORMAL;

static void
init_color_support(void)
{
    if (getenv("NO_COLOR") != NULL)
    {
        use_color = false;
    }
}

void
print_section_header(const char *text)
{
    if (use_color)
    {
        printf(
          "\033[1;36m┌─ %s ───────────────────────────────────────────────────────────┐\033[0m\n",
          text);
    }
    else
    {
        printf("--- %s ---\n", text);
    }
}

void
print_success(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    if (use_color)
        printf("\033[1;32m");
    vprintf(format, args);
    if (use_color)
        printf("\033[0m");
    va_end(args);
}

void
print_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    if (use_color)
        fprintf(stderr, "\033[1;31m");
    vfprintf(stderr, format, args);
    if (use_color)
        fprintf(stderr, "\033[0m");
    va_end(args);
}

void
print_info(const char *format, ...)
{
    if (current_output_mode == OUTPUT_QUIET)
        return;

    va_list args;
    va_start(args, format);
    if (use_color)
        printf("\033[0;36m");
    vprintf(format, args);
    if (use_color)
        printf("\033[0m");
    va_end(args);
}

void
print_command_syntax(const char *command, const char *args)
{
    if (use_color)
    {
        printf("  \033[1;33m%s\033[0m %s\n", command, args);
    }
    else
    {
        printf("  %s %s\n", command, args);
    }
}

void
print_option(const char *option, const char *description)
{
    if (use_color)
    {
        printf("  \033[1;35m%-20s\033[0m %s\n", option, description);
    }
    else
    {
        printf("  %-20s %s\n", option, description);
    }
}

void
print_command_help(const char *command)
{
    printf("\n");

    if (!command || strcmp(command, "general") == 0)
    {
        print_section_header("HOSTMAN");
        printf("  A simple tool for managing file uploads to various hosting services\n\n");

        print_section_header("USAGE  ");
        printf("  hostman <command> [options]\n\n");

        print_section_header("GENERAL OPTIONS");
        print_option("--version, -v", "Display version information");
        print_option("--help, -h", "Display this help message");
        printf("\n");

        print_section_header("COMMANDS");
        print_command_syntax("upload", "<file_path> [file_path...]"),
          printf("   Upload one or more files to a hosting service\n");
        print_command_syntax("list-uploads", ""), printf("   List upload history\n");
        print_command_syntax("delete-upload", "<id>"),
          printf("   Delete an upload record from history\n");
        print_command_syntax("delete-file", "<id>"),
          printf("   Delete a file from the remote host\n");
        print_command_syntax("list-hosts", ""), printf("   List configured hosts\n");
        print_command_syntax("add-host", ""), printf("   Add a new host configuration\n");
        print_command_syntax("import-host", "<sxcu_file>"),
          printf("   Import host from ShareX SXCU file\n");
        print_command_syntax("remove-host", "<name>"), printf("   Remove a host configuration\n");
        print_command_syntax("set-default-host", "<name>"), printf("   Set the default host\n");
        print_command_syntax("config", "<get|set> <key> [value]"),
          printf("   View or modify configuration\n");
        print_command_syntax("help", "[command]"), printf("   Show help for a specific command\n");

        printf("\nFor more information about a specific command, run: hostman help <command>\n");
        return;
    }

    if (strcmp(command, "upload") == 0)
    {
        print_section_header("UPLOAD");
        printf("Upload one or more files to a configured hosting service\n\n");

        print_section_header("USAGE");
        printf("  hostman upload [options] <file_path> [file_path...]\n");
        printf("  hostman upload [options] --directory <path>\n\n");

        print_section_header("OPTIONS");
        print_option("--host <name>",
                     "Specify which host to use. If not provided, the default host will be used");
        print_option("--directory, -d <path>", "Upload all files from a directory");
        print_option("--continue-on-error, -c", "Continue uploading if a file fails (batch mode)");
        print_option("--help", "Show this help message");

        print_section_header("EXAMPLES");
        printf("  hostman upload image.png\n");
        printf("  hostman upload file1.png file2.jpg file3.gif\n");
        printf("  hostman upload --directory ./screenshots/\n");
        printf("  hostman upload -d ./images/ --continue-on-error\n");
        return;
    }

    if (strcmp(command, "list-uploads") == 0)
    {
        print_section_header("LIST-UPLOADS");
        printf("List previous file uploads\n\n");

        print_section_header("USAGE");
        printf("  hostman list-uploads [options]\n\n");

        print_section_header("OPTIONS");
        print_option("--host <name>", "Filter uploads by host");
        print_option("--page <number>", "Page number for pagination (default: 1)");
        print_option("--limit <count>", "Number of records per page (default: 20)");
        print_option("--help", "Show this help message");
        return;
    }

    if (strcmp(command, "delete-upload") == 0)
    {
        print_section_header("DELETE-UPLOAD");
        printf("Delete an upload record by ID\n\n");

        print_section_header("USAGE");
        printf("  hostman delete-upload <id>\n\n");

        print_section_header("OPTIONS");
        print_option("--help", "Show this help message");
        return;
    }

    if (strcmp(command, "delete-file") == 0)
    {
        print_section_header("DELETE-FILE");
        printf("Delete a file from the remote host using the deletion URL\n\n");

        print_section_header("USAGE");
        printf("  hostman delete-file <id>\n\n");

        print_section_header("OPTIONS");
        print_option("--help", "Show this help message");
        return;
    }

    if (strcmp(command, "list-hosts") == 0)
    {
        print_section_header("LIST-HOSTS");
        printf("List all configured hosts\n\n");

        print_section_header("USAGE");
        printf("  hostman list-hosts [options]\n\n");

        print_section_header("OPTIONS");
        print_option("--help", "Show this help message");
        return;
    }

    if (strcmp(command, "add-host") == 0)
    {
        print_section_header("ADD-HOST");
        printf("Add a new host configuration interactively\n\n");

        print_section_header("USAGE");
        printf("  hostman add-host [options]\n\n");

        print_section_header("OPTIONS");
        print_option("--help", "Show this help message");
        return;
    }

    if (strcmp(command, "import-host") == 0)
    {
        print_section_header("IMPORT-HOST");
        printf("Import a host configuration from a ShareX SXCU file\n\n");

        print_section_header("USAGE");
        printf("  hostman import-host <sxcu_file>\n\n");

        print_section_header("OPTIONS");
        print_option("--help", "Show this help message");

        print_section_header("DESCRIPTION");
        printf("  Imports host configuration from a ShareX Custom Uploader (.sxcu) file.\n");
        printf("  The SXCU file is a JSON format that defines upload endpoints.\n\n");

        print_section_header("EXAMPLES");
        printf("  hostman import-host myhost.sxcu\n");
        printf("  hostman import-host ~/Downloads/uploader.sxcu\n");
        return;
    }

    if (strcmp(command, "remove-host") == 0)
    {
        print_section_header("REMOVE-HOST");
        printf("Remove a host configuration\n\n");

        print_section_header("USAGE");
        printf("  hostman remove-host <host_name> [options]\n\n");

        print_section_header("OPTIONS");
        print_option("--help", "Show this help message");
        return;
    }

    if (strcmp(command, "set-default-host") == 0)
    {
        print_section_header("SET-DEFAULT-HOST");
        printf("Set the default host for uploads\n\n");

        print_section_header("USAGE");
        printf("  hostman set-default-host <host_name> [options]\n\n");

        print_section_header("OPTIONS");
        print_option("--help", "Show this help message");
        return;
    }

    if (strcmp(command, "config") == 0)
    {
        print_section_header("CONFIG");
        printf("View or modify configuration settings\n\n");

        print_section_header("USAGE");
        printf("  hostman config                     Interactive configuration editor\n");
        printf("  hostman config get <key>           Get a configuration value\n");
        printf("  hostman config set <key> <value>   Set a configuration value\n\n");

        print_section_header("OPTIONS");
        print_option("--help", "Show this help message");

        print_section_header("KEYS");
        print_option("log_level", "Log level (DEBUG, INFO, WARN, ERROR)");
        print_option("log_file", "Path to log file");
        print_option("default_host", "Default host for uploads");
        print_option("hosts.<name>.<prop>", "Host-specific settings");

        print_section_header("EXAMPLES");
        printf("  hostman config\n");
        printf("  hostman config get log_level\n");
        printf("  hostman config set log_level DEBUG\n");
        printf("  hostman config get hosts.myhost.api_endpoint\n");
        return;
    }

    print_error("Unknown command: %s\n", command);
    printf("Run 'hostman help' for a list of available commands.\n");
}

command_args_t
parse_args(int argc, char *argv[])
{
    command_args_t args = { 0 };
    args.type = CMD_UNKNOWN;
    args.page = 1;
    args.limit = 20;
    args.output_mode = OUTPUT_NORMAL;

    init_color_support();

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--json") == 0)
        {
            args.output_mode = OUTPUT_JSON;
            current_output_mode = OUTPUT_JSON;
            use_color = false;
        }
        else if (strcmp(argv[i], "--quiet") == 0 || strcmp(argv[i], "-q") == 0)
        {
            args.output_mode = OUTPUT_QUIET;
            current_output_mode = OUTPUT_QUIET;
        }
        else if (strcmp(argv[i], "--verbose") == 0)
        {
            args.output_mode = OUTPUT_VERBOSE;
            current_output_mode = OUTPUT_VERBOSE;
        }
        else if (strcmp(argv[i], "--no-color") == 0)
        {
            use_color = false;
        }
    }

    if (argc < 2)
    {
        print_command_help("general");
        return args;
    }

    int cmd_index = 1;
    while (cmd_index < argc && argv[cmd_index][0] == '-')
    {
        cmd_index++;
    }

    if (cmd_index >= argc)
    {
        print_command_help("general");
        return args;
    }

    if (strcmp(argv[cmd_index], "upload") == 0)
    {
        args.type = CMD_UPLOAD;
    }
    else if (strcmp(argv[cmd_index], "list-uploads") == 0)
    {
        args.type = CMD_LIST_UPLOADS;
    }
    else if (strcmp(argv[cmd_index], "list-hosts") == 0)
    {
        args.type = CMD_LIST_HOSTS;

        static struct option long_options[] = { { "help", no_argument, 0, '?' }, { 0, 0, 0, 0 } };

        int option_index = 0;
        int c;
        optind = cmd_index + 1;

        while ((c = getopt_long(argc, argv, "", long_options, &option_index)) != -1)
        {
            switch (c)
            {
                case '?':
                    print_command_help("list-hosts");
                    exit(EXIT_SUCCESS);
                default:
                    break;
            }
        }
    }
    else if (strcmp(argv[cmd_index], "delete-upload") == 0)
    {
        args.type = CMD_DELETE_UPLOAD;

        static struct option long_options[] = { { "help", no_argument, 0, '?' }, { 0, 0, 0, 0 } };

        int option_index = 0;
        int c;
        optind = cmd_index + 1;

        while ((c = getopt_long(argc, argv, "", long_options, &option_index)) != -1)
        {
            switch (c)
            {
                case '?':
                    print_command_help("delete-upload");
                    exit(EXIT_SUCCESS);
                default:
                    break;
            }
        }

        if (optind < argc)
        {
            args.upload_id = atoi(argv[optind]);
            if (args.upload_id <= 0)
            {
                print_error("Error: Invalid upload ID\n");
                args.type = CMD_UNKNOWN;
            }
        }
        else
        {
            print_error("Error: Upload ID required\n");
            args.type = CMD_UNKNOWN;
        }
    }
    else if (strcmp(argv[cmd_index], "delete-file") == 0)
    {
        args.type = CMD_DELETE_FILE;

        static struct option long_options[] = { { "help", no_argument, 0, '?' }, { 0, 0, 0, 0 } };

        int option_index = 0;
        int c;
        optind = cmd_index + 1;

        while ((c = getopt_long(argc, argv, "", long_options, &option_index)) != -1)
        {
            switch (c)
            {
                case '?':
                    print_command_help("delete-file");
                    exit(EXIT_SUCCESS);
                default:
                    break;
            }
        }

        if (optind < argc)
        {
            args.upload_id = atoi(argv[optind]);
            if (args.upload_id <= 0)
            {
                print_error("Error: Invalid upload ID\n");
                args.type = CMD_UNKNOWN;
            }
        }
        else
        {
            print_error("Error: Upload ID required\n");
            args.type = CMD_UNKNOWN;
        }
    }
    else if (strcmp(argv[cmd_index], "add-host") == 0)
    {
        args.type = CMD_ADD_HOST;
    }
    else if (strcmp(argv[cmd_index], "import-host") == 0)
    {
        args.type = CMD_IMPORT_HOST;
    }
    else if (strcmp(argv[cmd_index], "remove-host") == 0)
    {
        args.type = CMD_REMOVE_HOST;
    }
    else if (strcmp(argv[cmd_index], "set-default-host") == 0)
    {
        args.type = CMD_SET_DEFAULT_HOST;
    }
    else if (strcmp(argv[cmd_index], "config") == 0)
    {
        args.type = CMD_CONFIG;
    }
    else if (strcmp(argv[cmd_index], "help") == 0 || strcmp(argv[cmd_index], "--help") == 0 ||
             strcmp(argv[cmd_index], "-h") == 0)
    {
        args.type = CMD_HELP;
        if (cmd_index + 1 < argc)
        {
            args.command_name = strdup(argv[cmd_index + 1]);
        }
        else
        {
            args.command_name = strdup("general");
        }
    }
    else
    {
        print_error("Unknown command: %s\n", argv[cmd_index]);
        args.type = CMD_UNKNOWN;
        return args;
    }

    switch (args.type)
    {
        case CMD_UPLOAD:
        {
            static struct option long_options[] = { { "host", required_argument, 0, 'h' },
                                                    { "directory", required_argument, 0, 'd' },
                                                    { "continue-on-error", no_argument, 0, 'c' },
                                                    { "help", no_argument, 0, '?' },
                                                    { 0, 0, 0, 0 } };

            int option_index = 0;
            int c;
            optind = cmd_index + 1;

            while ((c = getopt_long(argc, argv, "h:d:c", long_options, &option_index)) != -1)
            {
                switch (c)
                {
                    case 'h':
                        args.host_name = strdup(optarg);
                        break;
                    case 'd':
                        args.directory = strdup(optarg);
                        break;
                    case 'c':
                        args.continue_on_error = true;
                        break;
                    case '?':
                        print_command_help("upload");
                        exit(EXIT_SUCCESS);
                    default:
                        break;
                }
            }

            if (args.directory)
            {
                struct stat dir_stat;
                if (stat(args.directory, &dir_stat) != 0 || !S_ISDIR(dir_stat.st_mode))
                {
                    print_error("Error: '%s' is not a valid directory\n", args.directory);
                    args.type = CMD_UNKNOWN;
                    break;
                }

                DIR *dir = opendir(args.directory);
                if (!dir)
                {
                    print_error("Error: Cannot open directory '%s'\n", args.directory);
                    args.type = CMD_UNKNOWN;
                    break;
                }

                int capacity = 16;
                args.file_paths = malloc(capacity * sizeof(char *));
                args.file_count = 0;

                struct dirent *entry;
                while ((entry = readdir(dir)) != NULL)
                {
                    if (entry->d_name[0] == '.')
                        continue;

                    char full_path[4096];
                    snprintf(full_path, sizeof(full_path), "%s/%s", args.directory, entry->d_name);

                    struct stat file_stat;
                    if (stat(full_path, &file_stat) == 0 && S_ISREG(file_stat.st_mode))
                    {
                        if (args.file_count >= capacity)
                        {
                            capacity *= 2;
                            args.file_paths = realloc(args.file_paths, capacity * sizeof(char *));
                        }
                        args.file_paths[args.file_count++] = strdup(full_path);
                    }
                }
                closedir(dir);

                if (args.file_count == 0)
                {
                    print_error("Error: No files found in directory '%s'\n", args.directory);
                    args.type = CMD_UNKNOWN;
                }
                else if (args.file_count > 0)
                {
                    args.file_path = strdup(args.file_paths[0]);
                }
            }
            else if (optind < argc)
            {
                int remaining = argc - optind;
                if (remaining == 1)
                {
                    args.file_path = strdup(argv[optind]);
                    args.file_count = 1;
                    args.file_paths = malloc(sizeof(char *));
                    args.file_paths[0] = strdup(argv[optind]);
                }
                else
                {
                    args.file_count = remaining;
                    args.file_paths = malloc(remaining * sizeof(char *));
                    for (int i = 0; i < remaining; i++)
                    {
                        args.file_paths[i] = strdup(argv[optind + i]);
                    }
                    args.file_path = strdup(args.file_paths[0]);
                }
            }
            else
            {
                print_error("Error: File path required\n");
                args.type = CMD_UNKNOWN;
            }
            break;
        }

        case CMD_LIST_UPLOADS:
        {
            static struct option long_options[] = { { "host", required_argument, 0, 'h' },
                                                    { "page", required_argument, 0, 'p' },
                                                    { "limit", required_argument, 0, 'l' },
                                                    { "help", no_argument, 0, '?' },
                                                    { 0, 0, 0, 0 } };

            int option_index = 0;
            int c;
            optind = cmd_index + 1;

            while ((c = getopt_long(argc, argv, "h:p:l:", long_options, &option_index)) != -1)
            {
                switch (c)
                {
                    case 'h':
                        args.host_name = strdup(optarg);
                        break;
                    case 'p':
                        args.page = atoi(optarg);
                        if (args.page < 1)
                            args.page = 1;
                        break;
                    case 'l':
                        args.limit = atoi(optarg);
                        if (args.limit < 1)
                            args.limit = 1;
                        break;
                    case '?':
                        print_command_help("list-uploads");
                        exit(EXIT_SUCCESS);
                    default:
                        break;
                }
            }
            break;
        }

        case CMD_LIST_HOSTS:
        {
            break;
        }

        case CMD_ADD_HOST:
        {
            break;
        }

        case CMD_IMPORT_HOST:
        {
            if (cmd_index + 1 < argc)
            {
                args.import_file = strdup(argv[cmd_index + 1]);
            }
            else
            {
                print_error("Error: SXCU file path required\n");
                args.type = CMD_UNKNOWN;
            }
            break;
        }

        case CMD_REMOVE_HOST:
        {
            if (cmd_index + 1 < argc)
            {
                args.host_name = strdup(argv[cmd_index + 1]);
            }
            else
            {
                print_error("Error: Host name required\n");
                args.type = CMD_UNKNOWN;
            }
            break;
        }

        case CMD_SET_DEFAULT_HOST:
        {
            if (cmd_index + 1 < argc)
            {
                args.host_name = strdup(argv[cmd_index + 1]);
            }
            else
            {
                print_error("Error: Host name required\n");
                args.type = CMD_UNKNOWN;
            }
            break;
        }

        case CMD_CONFIG:
        {
            int cfg_idx = cmd_index + 1;
            if (cfg_idx < argc)
            {
                if (strcmp(argv[cfg_idx], "get") == 0)
                {
                    args.config_get = true;
                    cfg_idx++;
                    if (cfg_idx < argc)
                    {
                        args.config_key = strdup(argv[cfg_idx]);
                    }
                    else
                    {
                        print_error("Error: Key required for 'config get'\n");
                        args.type = CMD_UNKNOWN;
                    }
                }
                else if (strcmp(argv[cfg_idx], "set") == 0)
                {
                    args.config_get = false;
                    cfg_idx++;
                    if (cfg_idx < argc)
                    {
                        args.config_key = strdup(argv[cfg_idx]);
                        cfg_idx++;
                        if (cfg_idx < argc)
                        {
                            args.config_value = strdup(argv[cfg_idx]);
                        }
                        else
                        {
                            print_error("Error: Value required for 'config set'\n");
                            args.type = CMD_UNKNOWN;
                        }
                    }
                    else
                    {
                        print_error("Error: Key required for 'config set'\n");
                        args.type = CMD_UNKNOWN;
                    }
                }
                else
                {
                    print_error("Error: 'config' requires 'get' or 'set' subcommand\n");
                    args.type = CMD_UNKNOWN;
                }
            }
            break;
        }

        default:
            break;
    }

    return args;
}

int
execute_command(command_args_t *args)
{
    switch (args->type)
    {
        case CMD_UPLOAD:
        {
            hostman_config_t *config = config_load();
            if (!config)
            {
                log_error("Failed to load configuration");
                return EXIT_CONFIG_ERROR;
            }

            host_config_t *host = NULL;
            if (args->host_name)
            {
                host = config_get_host(args->host_name);
                if (!host)
                {
                    print_error("Error: Host '%s' not found\n", args->host_name);
                    config_free(config);
                    return EXIT_INVALID_ARGS;
                }
            }
            else
            {
                host = config_get_default_host();
                if (!host)
                {
                    print_error("Error: No default host configured\n");
                    config_free(config);
                    return EXIT_CONFIG_ERROR;
                }
            }

            bool is_batch = args->file_count > 1;
            int success_count = 0;
            int failure_count = 0;
            char **success_urls = NULL;
            char **failed_files = NULL;
            char **failed_errors = NULL;

            if (is_batch)
            {
                success_urls = malloc(args->file_count * sizeof(char *));
                failed_files = malloc(args->file_count * sizeof(char *));
                failed_errors = malloc(args->file_count * sizeof(char *));

                print_section_header("BATCH UPLOAD");
                print_info("  Uploading %d files to %s\n\n", args->file_count, host->name);
            }

            for (int i = 0; i < args->file_count; i++)
            {
                const char *current_file = args->file_paths[i];
                char *filename = get_filename_from_path(current_file);
                struct stat file_stat;

                if (stat(current_file, &file_stat) != 0)
                {
                    if (is_batch)
                    {
                        print_error(
                          "  [%d/%d] %s - File not found\n", i + 1, args->file_count, filename);
                        failed_files[failure_count] = strdup(filename);
                        failed_errors[failure_count] = strdup("File not found");
                        failure_count++;
                        free(filename);

                        if (!args->continue_on_error)
                        {
                            print_error(
                              "\nStopping due to error (use --continue-on-error to continue)\n");
                            break;
                        }
                        continue;
                    }
                    else
                    {
                        print_error("Error: File not found: %s\n", current_file);
                        free(filename);
                        config_free(config);
                        return EXIT_FILE_ERROR;
                    }
                }

                if (is_batch)
                {
                    char size_str[32];
                    format_file_size(file_stat.st_size, size_str, sizeof(size_str));
                    print_info("  [%d/%d] Uploading %s (%s)...\n",
                               i + 1,
                               args->file_count,
                               filename,
                               size_str);
                }

                upload_response_t *response = network_upload_file(current_file, host);

                if (!response)
                {
                    if (is_batch)
                    {
                        print_error("        Failed: Network error\n");
                        failed_files[failure_count] = strdup(filename);
                        failed_errors[failure_count] = strdup("Network error");
                        failure_count++;
                        free(filename);

                        if (!args->continue_on_error)
                        {
                            print_error(
                              "\nStopping due to error (use --continue-on-error to continue)\n");
                            break;
                        }
                        continue;
                    }
                    else
                    {
                        print_error("Error: Upload failed\n");
                        free(filename);
                        config_free(config);
                        return EXIT_NETWORK_ERROR;
                    }
                }

                if (response->success)
                {
                    if (is_batch)
                    {
                        print_success("        Success: %s\n", response->url);
                        success_urls[success_count++] = strdup(response->url);
                    }
                    else
                    {
                        print_section_header("UPLOAD SUCCESSFUL");

                        char size_str[32];
                        format_file_size(file_stat.st_size, size_str, sizeof(size_str));

                        print_info("  File: %s (%s)\n", filename, size_str);
                        print_info("  Host: %s\n", host->name);

                        double time_ms = response->request_time_ms;
                        char time_str[32];
                        if (time_ms < 1000)
                        {
                            snprintf(time_str, sizeof(time_str), "%.2f ms", time_ms);
                        }
                        else
                        {
                            snprintf(time_str, sizeof(time_str), "%.2f sec", time_ms / 1000.0);
                        }
                        print_info("  Request time: %s\n", time_str);

                        printf("\n\033[1;32m%s\033[0m\n", response->url);

                        if (response->deletion_url)
                        {
                            printf("\n\033[1;33mDeletion URL: %s\033[0m\n", response->deletion_url);
                            print_info("  Save this URL to delete the file later\n");
                        }
                        printf("\n");

                        const char *clipboard_manager = get_clipboard_manager_name();
                        if (clipboard_manager && copy_to_clipboard(response->url))
                        {
                            print_success("URL copied to clipboard using %s\n", clipboard_manager);
                        }
                    }

                    db_add_upload(host->name,
                                  current_file,
                                  response->url,
                                  response->deletion_url,
                                  filename,
                                  file_stat.st_size);
                }
                else
                {
                    if (is_batch)
                    {
                        print_error("        Failed: %s\n", response->error_message);
                        failed_files[failure_count] = strdup(filename);
                        failed_errors[failure_count] = strdup(
                          response->error_message ? response->error_message : "Unknown error");
                        failure_count++;

                        if (!args->continue_on_error)
                        {
                            network_free_response(response);
                            free(filename);
                            print_error(
                              "\nStopping due to error (use --continue-on-error to continue)\n");
                            break;
                        }
                    }
                    else
                    {
                        print_error("Error: %s\n", response->error_message);
                        network_free_response(response);
                        free(filename);
                        config_free(config);
                        return EXIT_NETWORK_ERROR;
                    }
                }

                network_free_response(response);
                free(filename);
            }

            if (is_batch)
            {
                printf("\n");
                print_section_header("BATCH SUMMARY");
                print_info("  Total files: %d\n", args->file_count);
                print_success("  Successful:  %d\n", success_count);
                if (failure_count > 0)
                {
                    print_error("  Failed:      %d\n", failure_count);
                }
                else
                {
                    print_info("  Failed:      0\n");
                }

                if (success_count > 0)
                {
                    printf("\n");
                    print_section_header("UPLOADED URLs");
                    for (int i = 0; i < success_count; i++)
                    {
                        printf("  \033[1;32m%s\033[0m\n", success_urls[i]);
                    }

                    const char *clipboard_manager = get_clipboard_manager_name();
                    if (clipboard_manager && success_count == 1)
                    {
                        if (copy_to_clipboard(success_urls[0]))
                        {
                            printf("\n");
                            print_success("URL copied to clipboard using %s\n", clipboard_manager);
                        }
                    }
                    else if (clipboard_manager && success_count > 1)
                    {
                        size_t total_len = 0;
                        for (int i = 0; i < success_count; i++)
                        {
                            total_len += strlen(success_urls[i]) + 1;
                        }

                        char *all_urls = malloc(total_len);
                        if (all_urls)
                        {
                            all_urls[0] = '\0';
                            for (int i = 0; i < success_count; i++)
                            {
                                strcat(all_urls, success_urls[i]);
                                if (i < success_count - 1)
                                {
                                    strcat(all_urls, "\n");
                                }
                            }
                            if (copy_to_clipboard(all_urls))
                            {
                                printf("\n");
                                print_success("All URLs copied to clipboard using %s\n",
                                              clipboard_manager);
                            }
                            free(all_urls);
                        }
                    }
                }

                if (failure_count > 0)
                {
                    printf("\n");
                    print_section_header("FAILED FILES");
                    for (int i = 0; i < failure_count; i++)
                    {
                        print_error("  %s: %s\n", failed_files[i], failed_errors[i]);
                    }
                }

                for (int i = 0; i < success_count; i++)
                {
                    free(success_urls[i]);
                }
                for (int i = 0; i < failure_count; i++)
                {
                    free(failed_files[i]);
                    free(failed_errors[i]);
                }
                free(success_urls);
                free(failed_files);
                free(failed_errors);

                printf("\n");
            }

            config_free(config);
            return failure_count > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
        }

        case CMD_LIST_UPLOADS:
        {
            int count = 0;
            upload_record_t **records =
              db_get_uploads(args->host_name, args->page, args->limit, &count);

            if (!records)
            {
                print_error("Error: Failed to retrieve upload records\n");
                return EXIT_FAILURE;
            }

            if (count == 0)
            {
                print_info("No upload records found.\n");
                db_free_records(records, count);
                return EXIT_SUCCESS;
            }

            print_section_header("UPLOAD HISTORY");

            if (args->host_name)
            {
                print_info("Host: %s\n\n", args->host_name);
            }

            printf(
              "\033[1m%-3s %-20s %-15s %-35s %s\033[0m\n", "ID", "Date", "Host", "Filename", "URL");
            printf("%-3s %-20s %-15s %-35s %s\n",
                   "---",
                   "--------------------",
                   "---------------",
                   "-----------------------------------",
                   "----------------------------------------------------");

            for (int i = 0; i < count; i++)
            {
                char time_str[21];
                struct tm *tm_info = localtime(&records[i]->timestamp);
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

                char filename_display[36] = { 0 };
                if (records[i]->filename && strlen(records[i]->filename) > 34)
                {
                    strncpy(filename_display, records[i]->filename, 31);
                    filename_display[31] = '\0';
                    strcat(filename_display, "...");
                }
                else if (records[i]->filename)
                {
                    strncpy(filename_display, records[i]->filename, sizeof(filename_display) - 1);
                    filename_display[sizeof(filename_display) - 1] = '\0';
                }

                printf(
                  "%-3d \033[0;37m%-20s\033[0m \033[0;36m%-15s\033[0m %-35s \033[0;32m%s\033[0m",
                  records[i]->id,
                  time_str,
                  records[i]->host_name,
                  filename_display,
                  records[i]->remote_url);

                if (records[i]->deletion_url && strlen(records[i]->deletion_url) > 0)
                {
                    printf(" \033[1;33m[D]\033[0m");
                }
                printf("\n");
            }

            printf("\n\033[1mPage %d, showing %d record(s)\033[0m\n", args->page, count);

            bool has_deletion_urls = false;
            for (int i = 0; i < count; i++)
            {
                if (records[i]->deletion_url && strlen(records[i]->deletion_url) > 0)
                {
                    has_deletion_urls = true;
                    break;
                }
            }

            if (has_deletion_urls)
            {
                printf("\nRecords marked with \033[1;33m[D]\033[0m have deletion URLs.\n");
                printf("Use the following command to view and use deletion URLs:\n");
                printf("  hostman delete-file <id>\n");
            }

            db_free_records(records, count);
            return EXIT_SUCCESS;
        }

        case CMD_LIST_HOSTS:
        {
            hostman_config_t *config = config_load();
            if (!config)
            {
                log_error("Failed to load configuration");
                return EXIT_CONFIG_ERROR;
            }

            if (config->host_count == 0)
            {
                print_info("No hosts configured.\n");
                config_free(config);
                return EXIT_SUCCESS;
            }

            print_section_header("CONFIGURED HOSTS");

            printf("\033[1m%-20s %-40s %s\033[0m\n", "Name", "API Endpoint", "Default");
            printf("%-20s %-40s %s\n",
                   "--------------------",
                   "----------------------------------------",
                   "-------");

            for (int i = 0; i < config->host_count; i++)
            {
                const bool is_default = (config->default_host &&
                                         strcmp(config->default_host, config->hosts[i]->name) == 0);

                printf("\033[0;36m%-20s\033[0m %-40s %s\n",
                       config->hosts[i]->name,
                       config->hosts[i]->api_endpoint,
                       is_default ? "\033[1;32m✓ Yes\033[0m" : "No");
            }

            config_free(config);
            return EXIT_SUCCESS;
        }

        case CMD_ADD_HOST:
        {
            return hosts_add_interactive();
        }

        case CMD_IMPORT_HOST:
        {
            if (!args->import_file)
            {
                print_error("Error: SXCU file path required\n");
                return EXIT_INVALID_ARGS;
            }
            return hosts_import_sxcu(args->import_file);
        }

        case CMD_REMOVE_HOST:
        {
            if (!args->host_name)
            {
                print_error("Error: Host name required\n");
                return EXIT_INVALID_ARGS;
            }

            if (config_remove_host(args->host_name))
            {
                print_success("Host '%s' removed successfully.\n", args->host_name);
                return EXIT_SUCCESS;
            }
            else
            {
                print_error("Error: Failed to remove host '%s'\n", args->host_name);
                return EXIT_FAILURE;
            }
        }

        case CMD_SET_DEFAULT_HOST:
        {
            if (!args->host_name)
            {
                print_error("Error: Host name required\n");
                return EXIT_INVALID_ARGS;
            }

            if (config_set_default_host(args->host_name))
            {
                print_success("Default host set to '%s'.\n", args->host_name);
                return EXIT_SUCCESS;
            }
            else
            {
                print_error("Error: Failed to set default host to '%s'\n", args->host_name);
                return EXIT_FAILURE;
            }
        }

        case CMD_CONFIG:
        {
            if (!args->config_key)
            {
                if (tui_available())
                {
                    return tui_config_editor();
                }
                else
                {
                    return config_edit_interactive();
                }
            }
            else if (args->config_get)
            {
                char *value = config_get_value(args->config_key);
                if (value)
                {
                    print_success("%s\n", value);
                    free(value);
                    return EXIT_SUCCESS;
                }
                else
                {
                    print_error("Error: Failed to get configuration value for '%s'\n",
                                args->config_key);
                    return EXIT_FAILURE;
                }
            }
            else
            {
                if (config_set_value(args->config_key, args->config_value))
                {
                    print_success("Configuration value '%s' set to '%s'.\n",
                                  args->config_key,
                                  args->config_value);
                    return EXIT_SUCCESS;
                }
                else
                {
                    print_error("Error: Failed to set configuration value for '%s'\n",
                                args->config_key);
                    return EXIT_FAILURE;
                }
            }
        }

        case CMD_DELETE_UPLOAD:
        {
            if (args->upload_id <= 0)
            {
                print_error("Error: Invalid upload ID\n");
                return EXIT_INVALID_ARGS;
            }

            int count = 0;
            upload_record_t **records = db_get_uploads(NULL, 1, 1000, &count);
            bool found = false;

            if (records)
            {
                for (int i = 0; i < count; i++)
                {
                    if (records[i]->id == args->upload_id)
                    {
                        found = true;
                        printf("Delete the following record?\n\n");

                        char time_str[21];
                        struct tm *tm_info = localtime(&records[i]->timestamp);
                        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

                        char size_str[32];
                        format_file_size(records[i]->size, size_str, sizeof(size_str));

                        print_info("ID: %d\n", records[i]->id);
                        print_info("Date: %s\n", time_str);
                        print_info("Host: %s\n", records[i]->host_name);
                        print_info("File: %s (%s)\n", records[i]->filename, size_str);
                        print_info("URL: %s\n\n", records[i]->remote_url);

                        break;
                    }
                }
                db_free_records(records, count);
            }

            if (!found)
            {
                print_error("Error: No upload record found with ID %d\n", args->upload_id);
                return EXIT_FAILURE;
            }

            char response[10];
            printf("Are you sure you want to delete this record? [y/N]: ");
            if (fgets(response, sizeof(response), stdin) == NULL)
            {
                print_error("Error reading response\n");
                return EXIT_FAILURE;
            }

            if (response[0] == 'y' || response[0] == 'Y')
            {
                if (db_delete_upload(args->upload_id))
                {
                    print_success("Upload record deleted successfully.\n");
                    return EXIT_SUCCESS;
                }
                else
                {
                    print_error("Error: Failed to delete upload record.\n");
                    return EXIT_FAILURE;
                }
            }
            else
            {
                print_info("Delete operation cancelled.\n");
                return EXIT_SUCCESS;
            }
        }

        case CMD_DELETE_FILE:
        {
            if (args->upload_id <= 0)
            {
                print_error("Error: Invalid upload ID\n");
                return EXIT_INVALID_ARGS;
            }

            int count = 0;
            upload_record_t **records = db_get_uploads(NULL, 1, 1000, &count);
            bool found = false;
            char *deletion_url = NULL;
            upload_record_t *record = NULL;

            if (records)
            {
                for (int i = 0; i < count; i++)
                {
                    if (records[i]->id == args->upload_id)
                    {
                        found = true;
                        record = records[i];

                        if (!record->deletion_url || strlen(record->deletion_url) == 0)
                        {
                            print_error("Error: This upload doesn't have a deletion URL\n");
                            db_free_records(records, count);
                            return EXIT_FAILURE;
                        }

                        deletion_url = strdup(record->deletion_url);

                        printf("Delete the following file from the remote host?\n\n");

                        char time_str[21];
                        struct tm *tm_info = localtime(&record->timestamp);
                        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

                        char size_str[32];
                        format_file_size(record->size, size_str, sizeof(size_str));

                        print_info("ID: %d\n", record->id);
                        print_info("Date: %s\n", time_str);
                        print_info("Host: %s\n", record->host_name);
                        print_info("File: %s (%s)\n", record->filename, size_str);
                        print_info("URL: %s\n", record->remote_url);
                        print_info("Deletion URL: %s\n\n", record->deletion_url);
                        break;
                    }
                }
            }

            if (!found || !deletion_url)
            {
                print_error("Error: No upload record found with ID %d\n", args->upload_id);
                if (records)
                    db_free_records(records, count);
                return EXIT_FAILURE;
            }

            char response[10];
            printf("Are you sure you want to delete this file from the remote host? [y/N]: ");
            if (fgets(response, sizeof(response), stdin) == NULL)
            {
                print_error("Error reading response\n");
                if (records)
                    db_free_records(records, count);
                free(deletion_url);
                return EXIT_FAILURE;
            }

            if (response[0] != 'y' && response[0] != 'Y')
            {
                print_info("Delete operation cancelled.\n");
                if (records)
                    db_free_records(records, count);
                free(deletion_url);
                return EXIT_SUCCESS;
            }

            CURL *curl;
            CURLcode res;

            curl = curl_easy_init();
            if (!curl)
            {
                print_error("Error: Failed to initialize cURL\n");
                if (records)
                    db_free_records(records, count);
                free(deletion_url);
                return EXIT_NETWORK_ERROR;
            }

            print_info("Sending deletion request...\n");

            curl_easy_setopt(curl, CURLOPT_URL, deletion_url);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

            res = curl_easy_perform(curl);

            if (res != CURLE_OK)
            {
                print_error("Error: %s\n", curl_easy_strerror(res));
                curl_easy_cleanup(curl);
                if (records)
                    db_free_records(records, count);
                free(deletion_url);
                return EXIT_NETWORK_ERROR;
            }

            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

            curl_easy_cleanup(curl);
            free(deletion_url);

            bool success = (http_code >= 200 && http_code < 300);

            if (success)
            {
                print_success("File deleted successfully from the remote host!\n");

                char confirm[10];
                printf("Do you want to remove the record from the local database too? [y/N]: ");
                if (fgets(confirm, sizeof(confirm), stdin) != NULL &&
                    (confirm[0] == 'y' || confirm[0] == 'Y'))
                {
                    if (db_delete_upload(args->upload_id))
                    {
                        print_success("Upload record deleted from local database.\n");
                    }
                    else
                    {
                        print_error("Failed to delete upload record from local database.\n");
                    }
                }
            }
            else
            {
                print_error("Failed to delete file. HTTP status code: %ld\n", http_code);
                print_info("The file server might require a specific request method or additional "
                           "parameters.\n");
                print_info("You can try visiting the deletion URL in your browser: %s\n",
                           record->deletion_url);
            }

            if (records)
                db_free_records(records, count);
            return success ? EXIT_SUCCESS : EXIT_NETWORK_ERROR;
        }

        case CMD_HELP:
        {
            print_command_help(args->command_name);
            return EXIT_SUCCESS;
        }

        default:
            return EXIT_INVALID_ARGS;
    }
}

int
run_setup_wizard(void)
{
    print_info("Welcome to Hostman!\n\n");
    print_info("This appears to be your first time running the application.\n");
    print_info("Let's set up your initial configuration.\n\n");

    char *config_dir = get_config_dir();
    if (!config_dir)
    {
        print_error("Error: Failed to determine config directory.\n");
        return EXIT_FAILURE;
    }

    if (access(config_dir, F_OK) != 0)
    {
        print_info("Creating configuration directory: %s\n", config_dir);
        if (mkdir(config_dir, 0755) != 0)
        {
            print_error("Error: Failed to create configuration directory.\n");
            free(config_dir);
            return EXIT_FAILURE;
        }
    }
    free(config_dir);

    char *cache_dir = get_cache_dir();
    if (!cache_dir)
    {
        print_error("Error: Failed to determine cache directory.\n");
        return EXIT_FAILURE;
    }

    if (access(cache_dir, F_OK) != 0)
    {
        print_info("Creating cache directory: %s\n", cache_dir);
        if (mkdir(cache_dir, 0755) != 0)
        {
            print_error("Error: Failed to create cache directory.\n");
            free(cache_dir);
            return EXIT_FAILURE;
        }
    }

    char log_file[512];
    snprintf(log_file, sizeof(log_file), "%s/hostman.log", cache_dir);

    char input[512];
    printf("Where would you like to store logs? [%s]: ", log_file);
    if (fgets(input, sizeof(input), stdin) != NULL)
    {
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) > 0)
        {
            strncpy(log_file, input, sizeof(log_file) - 1);
            log_file[sizeof(log_file) - 1] = '\0';
        }
    }

    hostman_config_t *config = calloc(1, sizeof(hostman_config_t));
    if (!config)
    {
        print_error("Error: Failed to allocate memory for configuration.\n");
        free(cache_dir);
        return EXIT_FAILURE;
    }

    config->version = 1;
    config->log_level = strdup("INFO");
    config->log_file = strdup(log_file);
    config->hosts = NULL;
    config->host_count = 0;
    config->default_host = NULL;

    if (!config_save(config))
    {
        print_error("Error: Failed to save initial configuration.\n");
        config_free(config);
        free(cache_dir);
        return EXIT_FAILURE;
    }

    config_free(config);

    print_success("\nInitial configuration set up successfully.\n");
    print_info("Let's add your first host configuration.\n\n");

    int result = hosts_add_interactive();

    if (result == EXIT_SUCCESS)
    {
        print_success("\nSetup completed successfully!\n");
        print_info("You can now use hostman to upload files.\n");
    }
    else
    {
        print_error("\nSetup encountered an issue, but you can still use hostman.\n");
        print_info("Use 'hostman add-host' to add a host when ready.\n");
    }

    free(cache_dir);
    return result;
}

void
free_command_args(command_args_t *args)
{
    if (args)
    {
        free(args->host_name);
        free(args->file_path);
        free(args->directory);
        if (args->file_paths)
        {
            for (int i = 0; i < args->file_count; i++)
            {
                free(args->file_paths[i]);
            }
            free(args->file_paths);
        }
        free(args->config_key);
        free(args->config_value);
        free(args->command_name);
        free(args->import_file);
    }
}
