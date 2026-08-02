#include "hostman/core/notification.h"
#include "hostman/core/logging.h"

#ifdef HAVE_LIBNOTIFY
#include <glib.h>
#include <libnotify/notify.h>

static bool notification_ready = false;

bool
notification_init(void)
{
    if (!notify_init("hostman"))
    {
        log_warn("Failed to initialize libnotify");
        return false;
    }

    notification_ready = true;
    return true;
}

void
notification_cleanup(void)
{
    if (notification_ready)
    {
        notify_uninit();
        notification_ready = false;
    }
}

static void
notify_show(const char *summary, const char *body, NotifyUrgency urgency)
{
    if (!notification_ready)
    {
        return;
    }

    NotifyNotification *notification = notify_notification_new(summary, body, NULL);
    if (!notification)
    {
        log_warn("Failed to create notification");
        return;
    }

    notify_notification_set_urgency(notification, urgency);
    notify_notification_set_timeout(notification, NOTIFY_EXPIRES_DEFAULT);

    GError *error = NULL;
    if (!notify_notification_show(notification, &error))
    {
        log_warn("Failed to show notification: %s", error ? error->message : "unknown error");
        if (error)
        {
            g_error_free(error);
        }
    }

    g_object_unref(notification);
}

void
notify_send(const char *summary, const char *body)
{
    notify_show(summary, body, NOTIFY_URGENCY_NORMAL);
}

void
notify_send_error(const char *summary, const char *body)
{
    notify_show(summary, body, NOTIFY_URGENCY_CRITICAL);
}

#else

bool
notification_init(void)
{
    return true;
}

void
notification_cleanup(void)
{
}

void
notify_send(const char *summary, const char *body)
{
    (void)summary;
    (void)body;
}

void
notify_send_error(const char *summary, const char *body)
{
    (void)summary;
    (void)body;
}

#endif
