#ifndef HOSTMAN_NOTIFICATION_H
#define HOSTMAN_NOTIFICATION_H

#include <stdbool.h>

bool
notification_init(void);
void
notification_cleanup(void);
void
notify_send(const char *summary, const char *body);
void
notify_send_error(const char *summary, const char *body);

#endif
