#ifndef HOSTMAN_HOSTS_H
#define HOSTMAN_HOSTS_H

#include "hostman/core/config.h"
#include <stdbool.h>

int
hosts_add_interactive(void);

int
config_edit_interactive(void);

int
host_edit_interactive(const char *host_name);

bool
hosts_add(const char *name,
          const char *api_endpoint,
          const char *auth_type,
          const char *api_key_name,
          const char *api_key,
          const char *request_body_format,
          const char *file_form_field,
          const char *response_url_json_path,
          const char *response_deletion_url_json_path,
          char **static_field_names,
          char **static_field_values,
          int static_field_count);

int
hosts_import_sxcu(const char *file_path);

#endif
