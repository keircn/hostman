#include "hostman/core/utils.h"
#include "hostman/core/logging.h"
#include <ctype.h>
#include <math.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <cJSON.h>

char *
get_filename_from_path(const char *path)
{
    if (!path)
    {
        return NULL;
    }

    const char *last_slash = strrchr(path, '/');
    if (last_slash)
    {
        return strdup(last_slash + 1);
    }
    else
    {
        return strdup(path);
    }
}

void
format_file_size(size_t size, char *buffer, size_t buffer_size)
{
    const char *units[] = { "B", "KB", "MB", "GB", "TB" };
    int unit_index = 0;
    double size_double = (double)size;

    while (size_double >= 1024.0 && unit_index < 4)
    {
        size_double /= 1024.0;
        unit_index++;
    }

    if (unit_index == 0)
    {
        snprintf(buffer, buffer_size, "%zu %s", size, units[unit_index]);
    }
    else
    {
        snprintf(buffer, buffer_size, "%.1f %s", size_double, units[unit_index]);
    }
}

char *
get_config_dir(void)
{
    const char *xdg_config = getenv("XDG_CONFIG_HOME");
    if (xdg_config && *xdg_config)
    {
        size_t len = strlen(xdg_config) + strlen("/hostman") + 1;
        char *dir = malloc(len);
        if (dir)
        {
            snprintf(dir, len, "%s/hostman", xdg_config);
            return dir;
        }
    }

    const char *home = getenv("HOME");
    if (!home || !*home)
    {
        struct passwd *pwd = getpwuid(getuid());
        if (pwd)
        {
            home = pwd->pw_dir;
        }
        else
        {
            return NULL;
        }
    }

    size_t len = strlen(home) + strlen("/.config/hostman") + 1;
    char *dir = malloc(len);
    if (dir)
    {
        snprintf(dir, len, "%s/.config/hostman", home);
    }

    return dir;
}

char *
get_cache_dir(void)
{
    const char *xdg_cache = getenv("XDG_CACHE_HOME");
    if (xdg_cache && *xdg_cache)
    {
        size_t len = strlen(xdg_cache) + strlen("/hostman") + 1;
        char *dir = malloc(len);
        if (dir)
        {
            snprintf(dir, len, "%s/hostman", xdg_cache);
            return dir;
        }
    }

    const char *home = getenv("HOME");
    if (!home || !*home)
    {
        struct passwd *pwd = getpwuid(getuid());
        if (pwd)
        {
            home = pwd->pw_dir;
        }
        else
        {
            return NULL;
        }
    }

    size_t len = strlen(home) + strlen("/.cache/hostman") + 1;
    char *dir = malloc(len);
    if (dir)
    {
        snprintf(dir, len, "%s/.cache/hostman", home);
    }

    return dir;
}

char *
extract_json_string(const char *json, const char *path)
{
    if (!json || !path)
    {
        return NULL;
    }

    char *result = NULL;

    cJSON *root = cJSON_Parse(json);
    if (!root)
    {
        log_error("Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return NULL;
    }

    cJSON *current = root;
    const char *p = path;

    while (*p != '\0' && current)
    {
        if (*p == '.')
        {
            p++;
            continue;
        }

        if (*p != '[')
        {
            const char *key_start = p;
            while (*p != '\0' && *p != '.' && *p != '[')
            {
                p++;
            }

            size_t key_len = (size_t)(p - key_start);
            if (key_len == 0)
            {
                current = NULL;
                break;
            }

            char *key = malloc(key_len + 1);
            if (!key)
            {
                cJSON_Delete(root);
                return NULL;
            }

            memcpy(key, key_start, key_len);
            key[key_len] = '\0';
            current = cJSON_GetObjectItem(current, key);
            free(key);

            if (!current)
            {
                break;
            }
        }

        while (*p == '[' && current)
        {
            p++;

            if (!isdigit((unsigned char)*p))
            {
                current = NULL;
                break;
            }

            int index = 0;
            while (isdigit((unsigned char)*p))
            {
                index = index * 10 + (*p - '0');
                p++;
            }

            if (*p != ']')
            {
                current = NULL;
                break;
            }

            p++;

            if (!cJSON_IsArray(current))
            {
                current = NULL;
                break;
            }

            current = cJSON_GetArrayItem(current, index);
        }
    }

    if (current && cJSON_IsString(current))
    {
        result = strdup(current->valuestring);
    }

    cJSON_Delete(root);

    return result;
}

static const char *
find_command_in_path(const char *command)
{
    if (!command || command[0] == '\0')
    {
        return NULL;
    }

    static char command_found[64] = { 0 };

    if (strlen(command) >= sizeof(command_found))
    {
        return NULL;
    }

    const char *path_env = getenv("PATH");
    if (!path_env)
    {
        return NULL;
    }

    char *path_copy = strdup(path_env);
    if (!path_copy)
    {
        return NULL;
    }

    char *dir = strtok(path_copy, ":");
    while (dir != NULL)
    {
        char cmd_path[256];
        snprintf(cmd_path, sizeof(cmd_path), "%s/%s", dir, command);

        if (access(cmd_path, X_OK) == 0)
        {
            free(path_copy);
            strncpy(command_found, command, sizeof(command_found) - 1);
            command_found[sizeof(command_found) - 1] = '\0';
            return command_found;
        }

        dir = strtok(NULL, ":");
    }

    free(path_copy);
    return NULL;
}

static const char *
pick_clipboard_manager(void)
{
    const char *session_type = getenv("XDG_SESSION_TYPE");
    const bool is_x11_session = session_type && strcmp(session_type, "x11") == 0;
    const bool is_wayland_session = session_type && strcmp(session_type, "wayland") == 0;
    const bool has_display = getenv("DISPLAY") != NULL;
    const bool has_wayland_display = getenv("WAYLAND_DISPLAY") != NULL;

    const char *preferred_managers[6];
    size_t manager_count = 0;

    if (is_x11_session)
    {
        preferred_managers[manager_count++] = "xclip";
        preferred_managers[manager_count++] = "xsel";
    }
    else if (is_wayland_session)
    {
        preferred_managers[manager_count++] = "wl-copy";
    }
    else
    {
        if (has_display)
        {
            preferred_managers[manager_count++] = "xclip";
            preferred_managers[manager_count++] = "xsel";
        }
        if (has_wayland_display)
        {
            preferred_managers[manager_count++] = "wl-copy";
        }
    }

    preferred_managers[manager_count++] = "pbcopy";
    preferred_managers[manager_count++] = "clip.exe";
    preferred_managers[manager_count++] = "fish_clipboard_copy";

    for (size_t i = 0; i < manager_count; i++)
    {
        const char *resolved = find_command_in_path(preferred_managers[i]);
        if (resolved)
        {
            return resolved;
        }
    }

    return NULL;
}

static char *clipboard_override = NULL;

void
set_clipboard_override(const char *name)
{
    free(clipboard_override);
    clipboard_override = name ? strdup(name) : NULL;
}

static const char *
detect_clipboard_manager(void)
{
    static char command_found[64] = { 0 };

    if (command_found[0] != '\0')
    {
        return command_found;
    }

    if (clipboard_override)
    {
        const char *resolved = find_command_in_path(clipboard_override);
        if (resolved)
        {
            return resolved;
        }
        log_warn("Clipboard override '%s' not found in PATH, falling back to auto-detection",
                 clipboard_override);
    }

    const char *manager = pick_clipboard_manager();
    if (!manager)
    {
        return NULL;
    }

    strncpy(command_found, manager, sizeof(command_found) - 1);
    command_found[sizeof(command_found) - 1] = '\0';
    return command_found;
}

const char *
get_clipboard_manager_name(void)
{
    return detect_clipboard_manager();
}

bool
copy_to_clipboard(const char *text)
{
    if (!text || strlen(text) == 0)
        return false;

    if (strlen(text) > 4096)
    {
        log_error("Text too long for clipboard (max 4096 characters)");
        return false;
    }

    const char *clipboard_cmd = detect_clipboard_manager();
    if (!clipboard_cmd)
    {
        log_error("No clipboard manager found. Install wl-copy, xclip, xsel, or pbcopy");
        return false;
    }

    const char *cmd = NULL;
    if (strcmp(clipboard_cmd, "wl-copy") == 0)
    {
        cmd = "wl-copy";
    }
    else if (strcmp(clipboard_cmd, "xclip") == 0)
    {
        cmd = "xclip -selection clipboard";
    }
    else if (strcmp(clipboard_cmd, "xsel") == 0)
    {
        cmd = "xsel -ib";
    }
    else if (strcmp(clipboard_cmd, "pbcopy") == 0)
    {
        cmd = "pbcopy";
    }
    else if (strcmp(clipboard_cmd, "clip.exe") == 0)
    {
        cmd = "clip.exe";
    }
    else if (strcmp(clipboard_cmd, "fish_clipboard_copy") == 0)
    {
        cmd = "fish_clipboard_copy";
    }
    else
    {
        return false;
    }

    FILE *pipe = popen(cmd, "w");
    if (!pipe)
    {
        log_error("Failed to open pipe to clipboard command");
        return false;
    }

    size_t text_len = strlen(text);
    size_t written = fwrite(text, 1, text_len, pipe);
    int status = pclose(pipe);

    if (written != text_len)
    {
        log_error("Failed to write text to clipboard (wrote %zu of %zu bytes)", written, text_len);
        return false;
    }

    if (status != 0)
    {
        log_error("Clipboard command exited with status %d", status);
        return false;
    }

    return true;
}

static const char *
detect_clipboard_reader(void)
{
    static char command_found[64] = { 0 };

    if (command_found[0] != '\0')
    {
        return command_found;
    }

    const char *session_type = getenv("XDG_SESSION_TYPE");
    const bool is_wayland_session = session_type && strcmp(session_type, "wayland") == 0;
    const bool has_wayland_display = getenv("WAYLAND_DISPLAY") != NULL;
    const bool has_display = getenv("DISPLAY") != NULL;

    const char *preferred[6];
    size_t manager_count = 0;

    if (is_wayland_session)
    {
        preferred[manager_count++] = "wl-paste";
    }
    else
    {
        if (has_display)
        {
            preferred[manager_count++] = "xclip";
            preferred[manager_count++] = "xsel";
        }
        if (has_wayland_display)
        {
            preferred[manager_count++] = "wl-paste";
        }
    }

    preferred[manager_count++] = "wl-paste";
    preferred[manager_count++] = "xclip";
    preferred[manager_count++] = "xsel";
    preferred[manager_count++] = "pbpaste";

    for (size_t i = 0; i < manager_count; i++)
    {
        const char *resolved = find_command_in_path(preferred[i]);
        if (resolved)
        {
            strncpy(command_found, preferred[i], sizeof(command_found) - 1);
            command_found[sizeof(command_found) - 1] = '\0';
            return command_found;
        }
    }

    return NULL;
}

static bool
run_command_capture(const char *cmd, unsigned char **data, size_t *size)
{
    FILE *pipe = popen(cmd, "r");
    if (!pipe)
    {
        log_error("Failed to open pipe to command: %s", cmd);
        return false;
    }

    size_t capacity = 4096;
    size_t used = 0;
    unsigned char *buf = malloc(capacity);
    if (!buf)
    {
        pclose(pipe);
        return false;
    }

    while (1)
    {
        if (used == capacity)
        {
            capacity *= 2;
            unsigned char *nbuf = realloc(buf, capacity);
            if (!nbuf)
            {
                free(buf);
                pclose(pipe);
                return false;
            }
            buf = nbuf;
        }
        size_t n = fread(buf + used, 1, capacity - used, pipe);
        if (n == 0)
            break;
        used += n;
    }

    int status = pclose(pipe);

    if (status != 0 || used == 0)
    {
        free(buf);
        return false;
    }

    *data = buf;
    *size = used;
    return true;
}

static const char *
detect_image_extension(const unsigned char *data, size_t size)
{
    if (!data || size == 0)
        return NULL;

    if (size >= 8 && data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G')
        return "png";
    if (size >= 3 && data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF)
        return "jpg";
    if (size >= 4 && memcmp(data, "GIF8", 4) == 0)
        return "gif";
    if (size >= 12 && memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "WEBP", 4) == 0)
        return "webp";
    if (size >= 2 && data[0] == 'B' && data[1] == 'M')
        return "bmp";

    return NULL;
}

char *
read_clipboard_to_temp_file(void)
{
    const char *reader = detect_clipboard_reader();
    if (!reader)
    {
        log_error("No clipboard reader found. Install wl-paste, xclip, or xsel");
        return NULL;
    }

    static const char *image_types[] = {
        "image/png", "image/jpeg", "image/gif", "image/webp", "image/bmp"
    };

    unsigned char *data = NULL;
    size_t size = 0;
    const char *ext = NULL;

    for (size_t i = 0; i < sizeof(image_types) / sizeof(image_types[0]); i++)
    {
        char cmd[256];
        if (strcmp(reader, "wl-paste") == 0)
        {
            snprintf(cmd, sizeof(cmd), "wl-paste --type %s --no-newline", image_types[i]);
        }
        else if (strcmp(reader, "xclip") == 0)
        {
            snprintf(cmd, sizeof(cmd), "xclip -selection clipboard -t %s -o", image_types[i]);
        }
        else if (strcmp(reader, "xsel") == 0)
        {
            snprintf(cmd, sizeof(cmd), "xsel --clipboard --output");
        }
        else if (strcmp(reader, "pbpaste") == 0)
        {
            snprintf(cmd, sizeof(cmd), "pbpaste");
        }
        else
        {
            continue;
        }

        if (run_command_capture(cmd, &data, &size))
        {
            ext = detect_image_extension(data, size);
            if (ext)
            {
                break;
            }
            free(data);
            data = NULL;
            size = 0;
        }
    }

    if (!data || !ext)
    {
        log_error("No image found in clipboard");
        return NULL;
    }

    char *cache_dir = get_cache_dir();
    if (!cache_dir)
    {
        free(data);
        return NULL;
    }

    if (access(cache_dir, F_OK) != 0)
    {
        if (mkdir(cache_dir, 0755) != 0)
        {
            log_error("Failed to create cache directory: %s", cache_dir);
            free(data);
            free(cache_dir);
            return NULL;
        }
    }

    char path[512];
    snprintf(path,
             sizeof(path),
             "%s/clipboard-upload-%ld-%ld.%s",
             cache_dir,
             (long)getpid(),
             (long)time(NULL),
             ext);

    FILE *f = fopen(path, "wb");
    if (!f)
    {
        log_error("Failed to create temp file: %s", path);
        free(data);
        free(cache_dir);
        return NULL;
    }

    size_t written = fwrite(data, 1, size, f);
    fclose(f);
    free(data);
    free(cache_dir);

    if (written != size)
    {
        log_error("Failed to write clipboard data to %s", path);
        unlink(path);
        return NULL;
    }

    return strdup(path);
}

void
print_version_info(void)
{
    printf("\033[1;36mHOSTMAN %s\033[0m\n\n", HOSTMAN_VERSION);

    printf("\033[1;37mHostman\033[0m - A command-line image host manager\n\n");

    printf("\033[1;33mVersion:\033[0m     v%s\n", HOSTMAN_VERSION);
    printf("\033[1;33mBuilt on:\033[0m    %s\n", HOSTMAN_BUILD_DATE);
    printf("\033[1;33mBuilt at:\033[0m    %s\n", HOSTMAN_BUILD_TIME);

#ifdef __GNUC__
    printf("\033[1;33mCompiler:\033[0m    GCC/G++ %d.%d.%d\n",
           __GNUC__,
           __GNUC_MINOR__,
           __GNUC_PATCHLEVEL__);
#else
    printf("\033[1;33mCompiler:\033[0m    Unknown\n");
#endif

#ifdef __linux__
    printf("\033[1;33mPlatform:\033[0m    Linux\n");
#elif defined(_WIN32) || defined(_WIN64)
    printf("\033[1;33mPlatform:\033[0m    Windows\n");
#elif defined(__APPLE__) && defined(__MACH__)
    printf("\033[1;33mPlatform:\033[0m    macOS\n");
#else
    printf("\033[1;33mPlatform:\033[0m    Unknown\n");
#endif

    printf("\n\033[1;37mMaintainer:\033[0m  %s\n", HOSTMAN_AUTHOR);
    printf("\033[1;37mRepository:\033[0m  %s\n\n", HOSTMAN_HOMEPAGE);

    printf("\033[0;37mLicensed under MIT License.\033[0m\n");
}
