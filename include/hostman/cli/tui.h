#ifndef HOSTMAN_TUI_H
#define HOSTMAN_TUI_H

#include <stdbool.h>

bool
tui_available(void);

int
tui_config_editor(void);

int
tui_host_editor(const char *host_name);

#endif
