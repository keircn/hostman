#include "hostman/cli/tui.h"
#include "hostman/core/config.h"
#include "hostman/core/logging.h"
#include "hostman/crypto/encryption.h"
#include "hostman/network/hosts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_TUI
#include <ncurses.h>

#define COLOR_TITLE 1
#define COLOR_SELECTED 2
#define COLOR_NORMAL 3
#define COLOR_SUCCESS 4
#define COLOR_ERROR 5
#define COLOR_INFO 6

static WINDOW *main_win = NULL;
static WINDOW *status_win = NULL;

static void
tui_init(void)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    if (has_colors())
    {
        start_color();
        use_default_colors();
        init_pair(COLOR_TITLE, COLOR_CYAN, -1);
        init_pair(COLOR_SELECTED, COLOR_BLACK, COLOR_CYAN);
        init_pair(COLOR_NORMAL, -1, -1);
        init_pair(COLOR_SUCCESS, COLOR_GREEN, -1);
        init_pair(COLOR_ERROR, COLOR_RED, -1);
        init_pair(COLOR_INFO, COLOR_YELLOW, -1);
    }

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    main_win = newwin(max_y - 2, max_x, 0, 0);
    status_win = newwin(2, max_x, max_y - 2, 0);
    keypad(main_win, TRUE);
}

static void
tui_cleanup(void)
{
    if (main_win)
        delwin(main_win);
    if (status_win)
        delwin(status_win);
    endwin();
}

static void
tui_status(const char *msg)
{
    werase(status_win);
    wattron(status_win, A_REVERSE);
    int max_x = getmaxx(status_win);
    for (int i = 0; i < max_x; i++)
        mvwaddch(status_win, 0, i, ' ');
    mvwprintw(status_win, 0, 1, "%s", msg);
    wattroff(status_win, A_REVERSE);
    mvwprintw(status_win, 1, 1, "Use arrows to navigate, Enter to select, q to quit");
    wrefresh(status_win);
}

static char *
tui_input_dialog(const char *title, const char *prompt, const char *current_value)
{
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int win_width = max_x - 10;
    if (win_width > 80)
        win_width = 80;
    int win_height = 7;
    int start_y = (max_y - win_height) / 2;
    int start_x = (max_x - win_width) / 2;

    WINDOW *dialog = newwin(win_height, win_width, start_y, start_x);
    box(dialog, 0, 0);

    wattron(dialog, COLOR_PAIR(COLOR_TITLE) | A_BOLD);
    mvwprintw(dialog, 0, 2, " %s ", title);
    wattroff(dialog, COLOR_PAIR(COLOR_TITLE) | A_BOLD);

    mvwprintw(dialog, 2, 2, "%s", prompt);

    int input_width = win_width - 6;
    WINDOW *input_win = derwin(dialog, 1, input_width, 4, 2);

    char *buffer = malloc(512);
    if (!buffer)
    {
        delwin(input_win);
        delwin(dialog);
        return NULL;
    }

    if (current_value)
    {
        strncpy(buffer, current_value, 511);
        buffer[511] = '\0';
    }
    else
    {
        buffer[0] = '\0';
    }

    curs_set(1);
    echo();
    wrefresh(dialog);

    mvwprintw(input_win, 0, 0, "%s", buffer);
    wrefresh(input_win);

    wmove(input_win, 0, 0);
    wclrtoeol(input_win);
    wgetnstr(input_win, buffer, 511);

    noecho();
    curs_set(0);
    delwin(input_win);
    delwin(dialog);

    touchwin(main_win);
    wrefresh(main_win);

    if (strlen(buffer) == 0 && current_value)
    {
        strncpy(buffer, current_value, 511);
        buffer[511] = '\0';
    }

    return buffer;
}

static int
tui_menu(const char *title, char **items, int item_count, int selected)
{
    int max_y = getmaxy(main_win);
    int visible_items = max_y - 6;
    int scroll_offset = 0;

    if (selected >= visible_items)
    {
        scroll_offset = selected - visible_items + 1;
    }

    while (1)
    {
        werase(main_win);
        box(main_win, 0, 0);

        wattron(main_win, COLOR_PAIR(COLOR_TITLE) | A_BOLD);
        mvwprintw(main_win, 0, 2, " %s ", title);
        wattroff(main_win, COLOR_PAIR(COLOR_TITLE) | A_BOLD);

        for (int i = 0; i < visible_items && (i + scroll_offset) < item_count; i++)
        {
            int idx = i + scroll_offset;
            if (idx == selected)
            {
                wattron(main_win, COLOR_PAIR(COLOR_SELECTED) | A_BOLD);
                mvwprintw(main_win, i + 2, 2, " > %-*s ", getmaxx(main_win) - 8, items[idx]);
                wattroff(main_win, COLOR_PAIR(COLOR_SELECTED) | A_BOLD);
            }
            else
            {
                mvwprintw(main_win, i + 2, 2, "   %-*s ", getmaxx(main_win) - 8, items[idx]);
            }
        }

        if (scroll_offset > 0)
        {
            mvwprintw(main_win, 1, getmaxx(main_win) - 4, "^^^");
        }
        if (scroll_offset + visible_items < item_count)
        {
            mvwprintw(main_win, max_y - 2, getmaxx(main_win) - 4, "vvv");
        }

        wrefresh(main_win);

        int ch = wgetch(main_win);
        switch (ch)
        {
            case KEY_UP:
            case 'k':
                if (selected > 0)
                {
                    selected--;
                    if (selected < scroll_offset)
                        scroll_offset = selected;
                }
                break;
            case KEY_DOWN:
            case 'j':
                if (selected < item_count - 1)
                {
                    selected++;
                    if (selected >= scroll_offset + visible_items)
                        scroll_offset = selected - visible_items + 1;
                }
                break;
            case KEY_ENTER:
            case '\n':
            case '\r':
                return selected;
            case 'q':
            case 27:
                return -1;
        }
    }
}

static void
tui_draw_host_config(host_config_t *host, int selected)
{
    werase(main_win);
    box(main_win, 0, 0);

    wattron(main_win, COLOR_PAIR(COLOR_TITLE) | A_BOLD);
    mvwprintw(main_win, 0, 2, " Edit Host: %s ", host->name);
    wattroff(main_win, COLOR_PAIR(COLOR_TITLE) | A_BOLD);

    const char *fields[] = { "API Endpoint",    "Auth Type",         "API Key Header",    "API Key",
                             "File Form Field", "Response URL Path", "Deletion URL Path", "Back" };

    const char *values[] = {
        host->api_endpoint ? host->api_endpoint : "(not set)",
        host->auth_type ? host->auth_type : "(not set)",
        host->api_key_name ? host->api_key_name : "(not set)",
        host->api_key_encrypted ? "********" : "(not set)",
        host->file_form_field ? host->file_form_field : "(not set)",
        host->response_url_json_path ? host->response_url_json_path : "(not set)",
        host->response_deletion_url_json_path ? host->response_deletion_url_json_path : "(not set)",
        ""
    };

    for (int i = 0; i < 8; i++)
    {
        if (i == selected)
        {
            wattron(main_win, COLOR_PAIR(COLOR_SELECTED) | A_BOLD);
        }

        if (i < 7)
        {
            mvwprintw(main_win, i + 2, 2, " %-20s: %-40s ", fields[i], values[i]);
        }
        else
        {
            mvwprintw(main_win, i + 3, 2, " < %s ", fields[i]);
        }

        if (i == selected)
        {
            wattroff(main_win, COLOR_PAIR(COLOR_SELECTED) | A_BOLD);
        }
    }

    wrefresh(main_win);
}

int
tui_host_editor(const char *host_name)
{
    hostman_config_t *config = config_load();
    if (!config)
        return EXIT_FAILURE;

    host_config_t *host = config_get_host(host_name);
    if (!host)
        return EXIT_FAILURE;

    tui_init();
    tui_status("Editing host configuration");

    int selected = 0;
    bool modified = false;

    while (1)
    {
        tui_draw_host_config(host, selected);

        int ch = wgetch(main_win);
        switch (ch)
        {
            case KEY_UP:
            case 'k':
                if (selected > 0)
                    selected--;
                break;
            case KEY_DOWN:
            case 'j':
                if (selected < 7)
                    selected++;
                break;
            case KEY_ENTER:
            case '\n':
            case '\r':
                if (selected == 7)
                {
                    if (modified)
                        config_save(config);
                    tui_cleanup();
                    return EXIT_SUCCESS;
                }
                else
                {
                    char *new_val = NULL;
                    switch (selected)
                    {
                        case 0:
                            new_val = tui_input_dialog(
                              "API Endpoint", "Enter new API endpoint:", host->api_endpoint);
                            if (new_val && strlen(new_val) > 0)
                            {
                                free(host->api_endpoint);
                                host->api_endpoint = new_val;
                                modified = true;
                            }
                            else
                            {
                                free(new_val);
                            }
                            break;
                        case 1:
                            new_val = tui_input_dialog("Auth Type",
                                                       "Enter auth type (none/bearer/header):",
                                                       host->auth_type);
                            if (new_val &&
                                (strcmp(new_val, "none") == 0 || strcmp(new_val, "bearer") == 0 ||
                                 strcmp(new_val, "header") == 0))
                            {
                                free(host->auth_type);
                                host->auth_type = new_val;
                                modified = true;
                            }
                            else
                            {
                                free(new_val);
                            }
                            break;
                        case 2:
                            new_val = tui_input_dialog(
                              "API Key Header", "Enter API key header name:", host->api_key_name);
                            if (new_val && strlen(new_val) > 0)
                            {
                                free(host->api_key_name);
                                host->api_key_name = new_val;
                                modified = true;
                            }
                            else
                            {
                                free(new_val);
                            }
                            break;
                        case 3:
                            new_val = tui_input_dialog("API Key", "Enter new API key:", NULL);
                            if (new_val && strlen(new_val) > 0)
                            {
                                char *encrypted = encryption_encrypt_api_key(new_val);
                                if (encrypted)
                                {
                                    free(host->api_key_encrypted);
                                    host->api_key_encrypted = encrypted;
                                    modified = true;
                                }
                                free(new_val);
                            }
                            else
                            {
                                free(new_val);
                            }
                            break;
                        case 4:
                            new_val = tui_input_dialog("File Form Field",
                                                       "Enter file form field name:",
                                                       host->file_form_field);
                            if (new_val && strlen(new_val) > 0)
                            {
                                free(host->file_form_field);
                                host->file_form_field = new_val;
                                modified = true;
                            }
                            else
                            {
                                free(new_val);
                            }
                            break;
                        case 5:
                            new_val = tui_input_dialog("Response URL Path",
                                                       "Enter JSON path for URL:",
                                                       host->response_url_json_path);
                            if (new_val && strlen(new_val) > 0)
                            {
                                free(host->response_url_json_path);
                                host->response_url_json_path = new_val;
                                modified = true;
                            }
                            else
                            {
                                free(new_val);
                            }
                            break;
                        case 6:
                            new_val = tui_input_dialog("Deletion URL Path",
                                                       "Enter JSON path for deletion URL:",
                                                       host->response_deletion_url_json_path);
                            if (new_val && strlen(new_val) > 0)
                            {
                                free(host->response_deletion_url_json_path);
                                host->response_deletion_url_json_path = new_val;
                                modified = true;
                            }
                            else
                            {
                                free(new_val);
                            }
                            break;
                    }
                    tui_status(modified ? "Configuration modified (will save on exit)"
                                        : "Editing host configuration");
                }
                break;
            case 'q':
            case 27:
                if (modified)
                    config_save(config);
                tui_cleanup();
                return EXIT_SUCCESS;
        }
    }
}

int
tui_config_editor(void)
{
    tui_init();

    while (1)
    {
        hostman_config_t *config = config_load();
        if (!config)
        {
            tui_cleanup();
            fprintf(stderr, "Error: Failed to load configuration\n");
            return EXIT_FAILURE;
        }

        int item_count = config->host_count + 5;
        char **items = malloc(item_count * sizeof(char *));
        if (!items)
        {
            tui_cleanup();
            return EXIT_FAILURE;
        }

        for (int i = 0; i < config->host_count; i++)
        {
            items[i] = malloc(256);
            bool is_default =
              config->default_host && strcmp(config->hosts[i]->name, config->default_host) == 0;
            snprintf(
              items[i], 256, "%-20s %s", config->hosts[i]->name, is_default ? "(default)" : "");
        }

        int base = config->host_count;
        items[base + 0] = strdup("--- Settings ---");
        items[base + 1] = malloc(256);
        snprintf(
          items[base + 1], 256, "Log Level: %s", config->log_level ? config->log_level : "INFO");
        items[base + 2] = strdup("Change Default Host");
        items[base + 3] = strdup("Add New Host");
        items[base + 4] = strdup("Quit");

        tui_status("Select a host to edit or choose an action");
        int selected = tui_menu("Hostman Configuration", items, item_count, 0);

        for (int i = 0; i < item_count; i++)
            free(items[i]);
        free(items);

        if (selected < 0 || selected == base + 4)
        {
            tui_cleanup();
            return EXIT_SUCCESS;
        }

        if (selected < config->host_count)
        {
            tui_host_editor(config->hosts[selected]->name);
        }
        else if (selected == base + 0)
        {
            continue;
        }
        else if (selected == base + 1)
        {
            char **levels = malloc(4 * sizeof(char *));
            levels[0] = strdup("DEBUG");
            levels[1] = strdup("INFO");
            levels[2] = strdup("WARN");
            levels[3] = strdup("ERROR");

            int current = 1;
            if (config->log_level)
            {
                if (strcmp(config->log_level, "DEBUG") == 0)
                    current = 0;
                else if (strcmp(config->log_level, "INFO") == 0)
                    current = 1;
                else if (strcmp(config->log_level, "WARN") == 0)
                    current = 2;
                else if (strcmp(config->log_level, "ERROR") == 0)
                    current = 3;
            }

            tui_status("Select log level");
            int level_sel = tui_menu("Log Level", levels, 4, current);

            if (level_sel >= 0)
            {
                config_set_value("log_level", levels[level_sel]);
            }

            for (int i = 0; i < 4; i++)
                free(levels[i]);
            free(levels);
        }
        else if (selected == base + 2)
        {
            if (config->host_count == 0)
            {
                tui_status("No hosts configured");
                continue;
            }

            char **hosts = malloc(config->host_count * sizeof(char *));
            for (int i = 0; i < config->host_count; i++)
            {
                hosts[i] = strdup(config->hosts[i]->name);
            }

            tui_status("Select default host");
            int host_sel = tui_menu("Set Default Host", hosts, config->host_count, 0);

            if (host_sel >= 0)
            {
                config_set_default_host(hosts[host_sel]);
            }

            for (int i = 0; i < config->host_count; i++)
                free(hosts[i]);
            free(hosts);
        }
        else if (selected == base + 3)
        {
            tui_cleanup();
            hosts_add_interactive();
            tui_init();
        }
    }
}

bool
tui_available(void)
{
    return true;
}

#else

bool
tui_available(void)
{
    return false;
}

int
tui_config_editor(void)
{
    fprintf(stderr, "TUI not available. Rebuild with -DHOSTMAN_USE_TUI=ON\n");
    return EXIT_FAILURE;
}

int
tui_host_editor(const char *host_name)
{
    (void)host_name;
    fprintf(stderr, "TUI not available. Rebuild with -DHOSTMAN_USE_TUI=ON\n");
    return EXIT_FAILURE;
}

#endif
