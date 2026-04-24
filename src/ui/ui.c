#include "hostman/ui/ui.h"
#include "hostman/core/logging.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MAX_INPUT_LENGTH 512
#define DEFAULT_WIDTH 60

static ui_context_t ctx = {
    .style = UI_STYLE_NORMAL,
    .use_color = true,
    .use_unicode = true,
    .is_interactive = true,
    .width = DEFAULT_WIDTH,
};

static const char *
color_or_empty(const char *code)
{
    return ctx.use_color ? code : "";
}

static void
detect_terminal_size(void)
{
    struct winsize ws;

    if (!isatty(STDOUT_FILENO))
    {
        return;
    }

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
    {
        int width = (int)ws.ws_col;
        if (width < 30)
        {
            ctx.width = 30;
        }
        else if (width > 120)
        {
            ctx.width = 120;
        }
        else
        {
            ctx.width = width;
        }
    }
}

void
ui_init(int *argc, char *argv[])
{
    (void)argc;
    (void)argv;

    ctx.is_interactive = ui_detect_interactive();
    ctx.style = ui_detect_style();

    if (getenv("NO_COLOR") != NULL)
    {
        ctx.use_color = false;
    }

    if (!ctx.is_interactive)
    {
        ctx.style = UI_STYLE_SIMPLE;
        ctx.use_color = false;
        ctx.use_unicode = false;
    }

    detect_terminal_size();
}

ui_context_t *
ui_context(void)
{
    return &ctx;
}

bool
ui_detect_interactive(void)
{
    return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
}

ui_style_t
ui_detect_style(void)
{
    const char *term;

    if (!isatty(STDOUT_FILENO))
    {
        return UI_STYLE_SIMPLE;
    }

    term = getenv("TERM");
    if (term == NULL || strcmp(term, "dumb") == 0)
    {
        return UI_STYLE_SIMPLE;
    }

    if (strstr(term, "256") != NULL || strstr(term, "xterm") != NULL ||
        strstr(term, "kitty") != NULL)
    {
        return UI_STYLE_FANCY;
    }

    return UI_STYLE_NORMAL;
}

bool
ui_set_style(ui_style_t style)
{
    if (style < UI_STYLE_SIMPLE || style > UI_STYLE_FANCY)
    {
        return false;
    }

    ctx.style = style;
    return true;
}

void
ui_force_simple(bool simple)
{
    if (simple)
    {
        ctx.style = UI_STYLE_SIMPLE;
        ctx.use_color = false;
        ctx.use_unicode = false;
    }
}

void
ui_set_width(int width)
{
    if (width >= 30 && width <= 200)
    {
        ctx.width = width;
    }
}

void
ui_enable_color(bool enable)
{
    ctx.use_color = enable;
}

void
ui_header(const char *title)
{
    int line_len;

    if (!title)
    {
        title = "";
    }

    if (ctx.style == UI_STYLE_SIMPLE)
    {
        printf("\n=== %s ===\n\n", title);
        return;
    }

    line_len = ctx.width - 6;
    if (line_len < 10)
    {
        line_len = 10;
    }

    printf("\n%s", color_or_empty("\033[1;36m"));
    printf("--- %s ", title);
    for (int i = 0; i < line_len; i++)
    {
        putchar('-');
    }
    printf("---\n%s", color_or_empty("\033[0m"));
}

void
ui_subheader(const char *title)
{
    if (!title)
    {
        title = "";
    }

    if (ctx.style == UI_STYLE_SIMPLE)
    {
        printf("--- %s ---\n", title);
        return;
    }

    printf("%s%s%s\n", color_or_empty("\033[1;36m"), title, color_or_empty("\033[0m"));
}

void
ui_success(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fputs(color_or_empty("\033[1;32m"), stdout);
    vfprintf(stdout, format, args);
    fputs(color_or_empty("\033[0m"), stdout);
    va_end(args);
}

void
ui_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fputs(color_or_empty("\033[1;31m"), stderr);
    vfprintf(stderr, format, args);
    fputs(color_or_empty("\033[0m"), stderr);
    va_end(args);
}

void
ui_warn(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fputs(color_or_empty("\033[1;33m"), stderr);
    vfprintf(stderr, format, args);
    fputs(color_or_empty("\033[0m"), stderr);
    va_end(args);
}

void
ui_info(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fputs(color_or_empty("\033[0;36m"), stdout);
    vfprintf(stdout, format, args);
    fputs(color_or_empty("\033[0m"), stdout);
    va_end(args);
}

void
ui_item(const char *label, const char *value)
{
    if (!label)
    {
        label = "";
    }
    if (!value)
    {
        value = "(not set)";
    }

    if (ctx.use_color)
    {
        printf("  %s%-30s%s %s%s%s\n",
               "\033[0;37m",
               label,
               "\033[0m",
               "\033[1;32m",
               value,
               "\033[0m");
    }
    else
    {
        printf("  %-30s %s\n", label, value);
    }
}

void
ui_option(const char *key, const char *description)
{
    if (!key)
    {
        key = "";
    }
    if (!description)
    {
        description = "";
    }

    if (ctx.use_color)
    {
        printf("  \033[1;33m%s\033[0m %s\n", key, description);
    }
    else
    {
        printf("  %s %s\n", key, description);
    }
}

void
ui_option_num(int num, const char *description)
{
    if (!description)
    {
        description = "";
    }

    if (ctx.use_color)
    {
        printf("  \033[1;33m%d.\033[0m %s\n", num, description);
    }
    else
    {
        printf("  %d. %s\n", num, description);
    }
}

void
ui_text(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
}

void
ui_label(const char *label, const char *value)
{
    if (!label)
    {
        label = "";
    }
    if (!value)
    {
        value = "";
    }

    if (ctx.use_color)
    {
        printf("\033[0;36m%s:\033[0m %s\n", label, value);
    }
    else
    {
        printf("%s: %s\n", label, value);
    }
}

void
ui_section(const char *title)
{
    ui_subheader(title);
}

void
ui_divider(void)
{
    for (int i = 0; i < ctx.width; i++)
    {
        putchar('-');
    }
    putchar('\n');
}

void
ui_spacer(void)
{
    putchar('\n');
}

void
ui_list_start(void)
{
}

void
ui_list_item(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    putchar('\n');
}

void
ui_list_end(void)
{
}

char *
ui_read(const char *prompt, bool required)
{
    char buffer[MAX_INPUT_LENGTH];

    if (!prompt)
    {
        prompt = "";
    }

    while (1)
    {
        printf("%s", prompt);
        fflush(stdout);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            return NULL;
        }

        buffer[strcspn(buffer, "\n")] = '\0';

        if (buffer[0] == '\0')
        {
            if (!required)
            {
                return NULL;
            }
            ui_error("This field is required.\n");
            continue;
        }

        return strdup(buffer);
    }
}

char *
ui_read_default(const char *prompt, const char *default_value)
{
    char full_prompt[MAX_INPUT_LENGTH * 2];
    char *input;

    if (!prompt)
    {
        prompt = "";
    }
    if (!default_value)
    {
        default_value = "";
    }

    snprintf(full_prompt, sizeof(full_prompt), "%s [%s]: ", prompt, default_value);
    input = ui_read(full_prompt, false);
    if (input == NULL)
    {
        return strdup(default_value);
    }

    return input;
}

char *
ui_read_password(const char *prompt)
{
    return ui_read(prompt, true);
}

bool
ui_confirm(const char *prompt)
{
    char *input;

    if (!prompt)
    {
        prompt = "Continue?";
    }

    input = ui_prompt(prompt, "y/N");
    if (input == NULL)
    {
        return false;
    }

    bool yes = (input[0] == 'y' || input[0] == 'Y');
    free(input);
    return yes;
}

int
ui_choose(const char *prompt, int count, const char **options)
{
    char buffer[32];
    int choice;

    if (count <= 0 || options == NULL)
    {
        return -1;
    }

    if (prompt)
    {
        printf("%s\n", prompt);
    }

    for (int i = 0; i < count; i++)
    {
        ui_option_num(i + 1, options[i]);
    }

    printf("Select option: ");
    fflush(stdout);
    if (fgets(buffer, sizeof(buffer), stdin) == NULL)
    {
        return -1;
    }

    choice = atoi(buffer) - 1;
    if (choice < 0 || choice >= count)
    {
        return -1;
    }

    return choice;
}

void
ui_progress_start(const char *task, int total)
{
    (void)total;
    if (!task)
    {
        task = "Working";
    }
    printf("%s... ", task);
    fflush(stdout);
}

void
ui_progress_update(int current)
{
    (void)current;
    putchar('.');
    fflush(stdout);
}

void
ui_progress_complete(void)
{
    printf("done\n");
}

char *
ui_prompt(const char *prompt, const char *hint)
{
    char full_prompt[MAX_INPUT_LENGTH * 2];

    if (!prompt)
    {
        prompt = "";
    }

    if (hint && hint[0] != '\0')
    {
        snprintf(full_prompt, sizeof(full_prompt), "%s (%s): ", prompt, hint);
        return ui_read(full_prompt, false);
    }

    snprintf(full_prompt, sizeof(full_prompt), "%s: ", prompt);
    return ui_read(full_prompt, false);
}
