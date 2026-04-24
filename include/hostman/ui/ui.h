#ifndef HOSTMAN_UI_H
#define HOSTMAN_UI_H

#include <stdbool.h>
#include <stdarg.h>

typedef enum
{
    UI_STYLE_SIMPLE,
    UI_STYLE_NORMAL,
    UI_STYLE_FANCY
} ui_style_t;

typedef enum
{
    UI_COLOR_NONE,
    UI_COLOR_BRIGHT,
    UI_COLOR_DIM
} ui_color_intensity_t;

typedef struct
{
    ui_style_t style;
    bool use_color;
    bool use_unicode;
    bool is_interactive;
    int width;
} ui_context_t;

void ui_init(int *argc, char *argv[]);

ui_context_t *ui_context(void);

bool ui_detect_interactive(void);

ui_style_t ui_detect_style(void);

bool ui_set_style(ui_style_t style);

void ui_force_simple(bool simple);

void ui_set_width(int width);

void ui_enable_color(bool enable);

void ui_header(const char *title);

void ui_subheader(const char *title);

void ui_success(const char *format, ...);

void ui_error(const char *format, ...);

void ui_warn(const char *format, ...);

void ui_info(const char *format, ...);

void ui_item(const char *label, const char *value);

void ui_option(const char *key, const char *description);

void ui_option_num(int num, const char *description);

void ui_text(const char *format, ...);

void ui_label(const char *label, const char *value);

void ui_section(const char *title);

void ui_divider(void);

void ui_spacer(void);

void ui_list_start(void);

void ui_list_item(const char *format, ...);

void ui_list_end(void);

char *ui_read(const char *prompt, bool required);

char *ui_read_default(const char *prompt, const char *default_value);

char *ui_read_password(const char *prompt);

bool ui_confirm(const char *prompt);

int ui_choose(const char *prompt, int count, const char **options);

void ui_progress_start(const char *task, int total);

void ui_progress_update(int current);

void ui_progress_complete(void);

char *ui_prompt(const char *prompt, const char *hint);

#endif